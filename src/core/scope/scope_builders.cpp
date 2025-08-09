#include "core/types.h"
#include "core/scope.h"

#include "core/callables/functions/function.h"
#include "core/callables/classes/class_base.h"
#include "core/callables/classes/method.h"


// Builds a simple shell scope for callables with only globalFunctions and globalClasses
SharedPtr<Scope> Scope::makeCallScope() {
    DEBUG_FLOW(FlowLevel::NONE);
    auto c = makeShared<Scope>(shared_from_this(),
                                globalFunctions,
                                globalClasses,
                                interpretMode);
    c->isCallableScope = true;
    c->isDetached = true;
    c->isClonedScope = true;
    c->isRoot = false;
    DEBUG_FLOW_EXIT();

    return c;
}



// creates a copy of the scope, as if no functions or classes were made previously
SharedPtr<Scope> Scope::detachScope(const std::unordered_set<String>& freeVarNames) {
    DEBUG_FLOW(FlowLevel::MED);

    auto detached = makeShared<Scope>(shared_from_this(), globalFunctions, globalClasses, interpretMode);
    detached->isDetached = true;
    detached->owner = owner+"(detached)";
    includeMetaData(detached, true);
    // For each free variable name, if it exists in this scope, or parent, get it and add to the vector
    for (const auto& name : freeVarNames) {
        if (this->hasVariable(name)) {
            VarNode original = getVariable(name);
            detached->declareVariable(name, original.uniqClone());
        }
    }

    DEBUG_FLOW_EXIT();
    return detached;
}

// creates a standalone scope with only freevariables and local functions/classes
SharedPtr<Scope> Scope::isolateScope(const std::unordered_set<String>& freeVarNames) {
    DEBUG_FLOW(FlowLevel::NONE);
    auto isolated = makeShared<Scope>(0, interpretMode, false);
    includeMetaData(isolated, true);

    for (const auto& name : freeVarNames) {
        if (this->hasVariable(name)) {
            isolated->declareVariable(name, getVariable(name).uniqClone());
        } else if (auto parent = getParent()) {
            if (parent->hasVariable(name)) {
                isolated->declareVariable(name, getVariable(name).uniqClone());
            }
        }
    }

    isolated->localFunctions = this->localFunctions;
    isolated->localClasses = this->localClasses;
    if (!isolated) { throw MerkError("isolated Scope is Null"); }
    DEBUG_FLOW_EXIT();

    return isolated;
}

// to be used on/in the scope calling the function
SharedPtr<Scope> Scope::buildFunctionCallScope(SharedPtr<Function> func, String name) {    
    SharedPtr<Scope> capturedScope = func->getCapturedScope();
    if (!capturedScope) {throw MerkError("FunctionCall::evaluate -> Function " + name + " Does Not Have Valid capturedScope | SubType: " + callableTypeAsString(func->getSubType()) + " MainType: " + callableTypeAsString(func->getCallableType()));}
    
    capturedScope->owner = generateScopeOwner("FuncCallCaptured", name);

    auto callScope = capturedScope->makeCallScope();
    if (!callScope) {throw MerkError("Scope Is Not Valid In UserFunction::execute->function");}
    callScope->owner = generateScopeOwner("FuncCall", name);
    this->appendChildScope(callScope, false);                       // appending for recursion
    return callScope;
}

SharedPtr<Scope> Scope::buildMethodCallScope(SharedPtr<Method> method, String name) {
    SharedPtr<Scope> capturedScope = method->getCapturedScope();
    if (!capturedScope) {throw MerkError("MethodCall::evaluate -> Method " + name + " Does Not Have Valid capturedScope | SubType: " + callableTypeAsString(method->getSubType()) + " MainType: " + callableTypeAsString(method->getCallableType()));}
    
    capturedScope->owner = generateScopeOwner("MethodCallCaptured", name);

    auto callScope = capturedScope->makeCallScope();
    if (!callScope) {throw MerkError("Scope Is Not Valid In UserMethod::execute->Method");}

    callScope->owner = generateScopeOwner("MethodCall", name);
    this->appendChildScope(callScope, false);                       // appending for recursion
    return callScope;
}

SharedPtr<Scope> Scope::makeInstanceScope(SharedPtr<Scope> classScope) {
    auto instanceScope = makeCallScope();
    includeMetaData(instanceScope, true);
    instanceScope->owner = generateScopeOwner("ClassInstance", classScope->owner);

    for (const auto& [varName, varPtr] : classScope->getContext().getVariables()) {
        if (varPtr) {
            if (!instanceScope->getContext().hasVariable(varName)) {
                instanceScope->getContext().setVariable(varName, varPtr->uniqClone());
            } else {
                DEBUG_LOG(LogLevel::WARNING, "Variable already exists in instanceScope: ", varName);
            }
        }
    }

    if (classScope->localFunctions.size() == 0) { throw MerkError("There Are No Functions in Scope " + owner); }

    for (const auto& [funcName, funcSigVec] : classScope->localFunctions) {
        for (auto& funcSig : funcSigVec) {
            instanceScope->localFunctions.registerFunction(funcName, funcSig);
        }
    }
    
    if (instanceScope->localFunctions.size() == 0) {throw MerkError("No Methods added to instanceScope");}

    return instanceScope;
}

// to be used on the ClassBase directly as it should contain all the necessary data
SharedPtr<Scope> Scope::buildInstanceScope(SharedPtr<ClassBase> classTemplate, String className) {
    auto capturedScope = classTemplate->getCapturedScope();
    auto capturedClone = capturedScope->clone(true);  // clone it safely
    auto classScope = classTemplate->getClassScope();

    SharedPtr<Scope> instanceScope = makeInstanceScope(classScope);
    if (!instanceScope){throw MerkError("InstanceScope creation failed in ClassCall::evaluate()");}

    classScope->appendChildScope(capturedClone);
    
    instanceScope->owner = generateScopeOwner("ClassInstance", className);
    auto captured = instanceScope->getParent();
    capturedClone->owner = generateScopeOwner("InstanceCaptured", className);
    if (capturedClone->has(instanceScope)) { throw MerkError("Captured Scope Already Contains InstanceScope"); }
    capturedClone->appendChildScope(instanceScope);

    return instanceScope;
}

// This one requires that the calling class pulls the parent in order to store the captured scope
SharedPtr<Scope> Scope::buildClassScope(FreeVars freeVarNames, String className) {

    SharedPtr<Scope> classDefCapturedScope = this->detachScope(freeVarNames);
    SharedPtr<Scope> classScope = classDefCapturedScope->makeCallScope();
    classScope->isDetached = true; // detached until ClassBase owns it
    classScope->owner = generateScopeOwner("ClassMainScopeClass", className);
    this->appendChildScope(classDefCapturedScope, "ClassDef::evaluate");    
    classDefCapturedScope->owner = generateScopeOwner("ClassDef--InitialCaptured", className);
    classDefCapturedScope->appendChildScope(classScope);  // cls->getClassScope // place classScope inside of classDefCapturedScope

    return classScope;
}

// helper for scope meta data
void Scope::includeMetaData(SharedPtr<Scope> newScope, bool thisIsDetached) const {
    newScope->isDetached = thisIsDetached;
    newScope->isClonedScope = newScope->isClonedScope ? newScope->isClonedScope : isClonedScope;
    newScope->isCallableScope = newScope->isCallableScope ? newScope->isCallableScope : isCallableScope;
    if (newScope->isDetached) {
        if (newScope->owner.empty()) {
            newScope->owner = owner;
        } else {
            newScope->owner = owner + "(detached)";
        }
    } else {
        newScope->owner = owner;
    }
}



SharedPtr<Scope> Scope::buildFunctionDefScope(const FreeVars& freeVars, const String& funcName) {
    auto defScope = this->isolateScope(freeVars);
    defScope->owner = generateScopeOwner("FunctionDef", funcName);
    return defScope;
}

SharedPtr<Scope> Scope::buildClassDefScope(const FreeVars& freeVars, const String& className) {
    auto classCaptured = this->detachScope(freeVars);
    auto classScope = classCaptured->makeCallScope();
    classCaptured->owner = generateScopeOwner("ClassDefCaptured", className);
    classScope->owner = generateScopeOwner("ClassDefScope", className);
    classCaptured->appendChildScope(classScope);
    this->appendChildScope(classCaptured);
    return classScope;
}
