#include "core/types.h"
#include "core/Scope.hpp"

#include "core/callables/functions/Function.hpp"
#include "core/callables/classes/ClassBase.hpp"
#include "core/callables/classes/Method.hpp"
#include "core/registry/TypeRegistry.hpp"





Scope::Scope(int scopeNum, bool interpretMode, bool isRootBool)
  : interpretMode(interpretMode)
{
    isRoot = isRootBool;
    scopeLevel = scopeNum;

    if (isRoot) {
        globalFunctions = makeShared<FunctionRegistry>();
        globalClasses   = makeShared<ClassRegistry>();

        globalTypes = makeShared<TypeRegistry>();                 // nominal TypeId space
        globalTypeSigs = makeShared<TypeSignatureRegistryManager>(*globalTypes); // signature pool

        initRootTypes();
    }

    ++liveScopeCount;
    ++totalScopeCreated;
}




SharedPtr<Scope> Scope::createChildScope() {
    auto c = makeShared<Scope>(shared_from_this(),
                                     globalFunctions,
                                     globalClasses,
                                     globalTypes,
                                     interpretMode);
    c->isRoot = false;
    childScopes.push_back(c);
    Scope::counts.blocks += 1;
    return c;
  }




// Constructor for child
Scope::Scope(SharedPtr<Scope> parent, SharedPtr<FunctionRegistry> globalF, SharedPtr<ClassRegistry> globalC, SharedPtr<TypeRegistry> globalT, bool interpretMode)
: parentScope(parent), 
    globalFunctions(std::move(globalF)),
    globalClasses(std::move(globalC)),
    globalTypes(std::move(globalT)),
    interpretMode(interpretMode)
{
    scopeLevel = parent ? parent->scopeLevel + 1 : 0;

    if (!parent) throw MerkError("Scope ctor: parent is null");
    globalTypeSigs = parent->globalTypeSigs;

    if (!globalTypes)    throw MerkError("Scope ctor: globalTypes is null");
    if (!globalTypeSigs) throw MerkError("Scope ctor: globalTypeSigs is null");

    localTypes.attach(*globalTypeSigs);

    ++liveScopeCount;
    ++totalScopeCreated;

}

Scope::~Scope() {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    if (scopeLevel == 0 && isRoot) {
        if (interpretMode) {
        } else {
            return; // Avoid clearing child scopes in compile mode
        }
    } else {
    
        clear();
        --liveScopeCount;
    }




    DEBUG_FLOW_EXIT();
}

void Scope::clear(bool internalCall) {
    auto weak = weak_from_this();

    // DEBUG_LOG(LogLevel::TRACE,
    //     "Destroying Scope addr=", this,
    //     " owner=", owner,
    //     " parent=", getParent() ? getParent().get() : 0,
    //     " (captured?) children=", childScopes.size(),
    //     " use_count=", weak.use_count()
    // );
    
        
    // if (owner.find("ClassInstance") != std::string::npos) {
    //     String funcs;
    //     for (auto& [funcName, funcSigs] : localFunctions) {(void)funcSigs; funcs += ", " + funcName;}
    //     String classes;
    //     DEBUG_LOG(LogLevel::TRACE,
    //               "Destroying Instance Scope functions=", localFunctions.size(), metaString(), "Scope Address: ", formattedScope(), "Funcs: ", funcs);
    // }
    context.clear();
    if (isRoot && internalCall) {
        // DEBUG_LOG(LogLevel::TRACE, metaString(), "InternalCall: ", (internalCall ? "true": "false"));
        // auto funIt = localFunctions.find("pop");
        // if (funIt != localFunctions.end()) {throw MerkError("Trying To Destroy Scope containing pop Method");}

        // auto it = localClasses.find("List");
        // if (it != localClasses.end()) {throw MerkError("Trying To Destroy Scope containing List class");}
    }
    

    
    
    localFunctions.clear();
    localClasses.clear();
    for (auto& child : childScopes) {
        if (child) {
            if (child.get() != this) {
                child->clear();  // Recursively clear
            }
            
        }
    }
    childScopes.clear();

    // Only for Root Scope
    if (isRoot) {
        if (globalFunctions) globalFunctions->clear();
        if (globalClasses) globalClasses->clear();
        if (globalTypes) globalTypes.reset();
    }
}







// Builds a simple shell scope for callables with only globalFunctions and globalClasses
SharedPtr<Scope> Scope::makeCallScope() {
    DEBUG_FLOW(FlowLevel::NONE);
    auto c = makeShared<Scope>(shared_from_this(),
                                globalFunctions,
                                globalClasses,
                                globalTypes,
                                interpretMode);
    c->globalTypes    = globalTypes;
    c->globalTypeSigs = globalTypeSigs;
    c->localTypes.attach(*c->globalTypeSigs);

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

    auto detached = makeShared<Scope>(shared_from_this(), globalFunctions, globalClasses, globalTypes, interpretMode);
    detached->globalTypes    = globalTypes;
    detached->globalTypeSigs = globalTypeSigs;
    detached->localTypes.attach(*detached->globalTypeSigs);

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
    detached->kind = ScopeKind::Detached;
    DEBUG_FLOW_EXIT();
    return detached;
}

// creates a standalone scope with only freevariables and local functions/classes
SharedPtr<Scope> Scope::isolateScope(const std::unordered_set<String>& freeVarNames) {
    DEBUG_FLOW(FlowLevel::NONE);
    auto isolated = makeShared<Scope>(0, interpretMode, false);
    isolated->globalTypes    = globalTypes;
    isolated->globalTypeSigs = globalTypeSigs;
    isolated->localTypes.attach(*isolated->globalTypeSigs);

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
    isolated->globalTypes = globalTypes;

    isolated->localFunctions = this->localFunctions;
    isolated->localClasses = this->localClasses;
    isolated->kind = ScopeKind::Isolated;
    if (!isolated) { throw MerkError("isolated Scope is Null"); }
    DEBUG_FLOW_EXIT();

    return isolated;
}

// to be used on/in the scope calling the function
SharedPtr<Scope> Scope::buildFunctionCallScope(SharedPtr<Function> func, String name) {    
    SharedPtr<Scope> capturedScope = func->getCapturedScope();
    if (!capturedScope) {throw MerkError("FunctionCall::evaluate -> Function " + name + " Does Not Have Valid capturedScope | SubType: " + callableTypeAsString(func->getSubType()) + " MainType: " + callableTypeAsString(func->getCallableType()));}
    
    capturedScope->owner = generateScopeOwner("FuncCallCaptured", name);
    capturedScope->kind = ScopeKind::Captured;
    auto callScope = capturedScope->makeCallScope();
    if (!callScope) {throw MerkError("Scope Is Not Valid In UserFunction::execute->function");}
    callScope->owner = generateScopeOwner("FuncCall", name);
    this->appendChildScope(callScope, false);                       // appending for recursion
    callScope->kind = ScopeKind::FunctionCall;
    Scope::counts.functionCalls += 1;
    return callScope;
}

SharedPtr<Scope> Scope::buildMethodCallScope(SharedPtr<Method> method, String name) {
    SharedPtr<Scope> capturedScope = method->getCapturedScope();
    if (!capturedScope) {throw MerkError("MethodCall::evaluate -> Method " + name + " Does Not Have Valid capturedScope | SubType: " + callableTypeAsString(method->getSubType()) + " MainType: " + callableTypeAsString(method->getCallableType()));}

    capturedScope->owner = generateScopeOwner("MethodCallCaptured", name);
    capturedScope->kind = ScopeKind::Captured;

    auto callScope = capturedScope->makeCallScope();
    if (!callScope) {throw MerkError("Scope Is Not Valid In UserMethod::execute->Method");}
    method->setCapturedScope(callScope);
    callScope->owner = generateScopeOwner("MethodCall", name);
    this->appendChildScope(callScope, false);                       // appending for recursion
    callScope->kind = ScopeKind::MethodCall;
    Scope::counts.methodCalls += 1;
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
    instanceScope->kind = ScopeKind::Instance;

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
    instanceScope->kind = ScopeKind::Instance;
    Scope::counts.instanceCalls += 1;
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
    classScope->kind = ScopeKind::Instance;
    Scope::counts.classCalls += 1;
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
