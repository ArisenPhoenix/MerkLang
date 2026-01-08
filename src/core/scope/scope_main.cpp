#include <iostream>
#include <unordered_set>
#include "core/scope.h"

#include <cassert>

#ifndef _WIN32
#include <execinfo.h>
#endif

#include "core/node/node.h"
#include "core/node/param_node.h"
#include "core/node/argument_node.h"

#include "core/types.h"
#include "core/errors.h"
#include "utilities/helper_functions.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"

#include "ast/ast_validate.h"
#include "ast/ast_chain.h"
#include "core/callables/callable.h"
#include "core/callables/classes/method.h"



#include "core/context.h"
#include "core/registry/class_registry.h"
#include "core/registry/function_registry.h"
#include "core/errors.h"
#include "core/callables/functions/function.h"
// #include "core/callables/classes/method.h"


int totalWith = 0;
int totalWithout = 0;


// Helper to format a pointer or return a "None" string
String formatPointer(const Scope* ptr) {
    std::ostringstream oss;
    if (ptr) {
        oss << ptr;  // Stream the memory address
    } else {
        oss << "None (Root Scope)";
    }
    return oss.str();
}

// constructor for root.
Scope::Scope(int scopeNum, bool interpretMode, bool isRootBool) 
    : interpretMode(interpretMode) {
        isRoot = isRootBool;
        scopeLevel = scopeNum;
        DEBUG_FLOW(FlowLevel::LOW);

    DEBUG_LOG(LogLevel::DEBUG, "[Ctor(root)] this=", this, " level=", scopeLevel, "\n");

    if (isRoot) {
        globalFunctions = makeShared<FunctionRegistry>();
        globalClasses = makeShared<ClassRegistry>();
    }
    DEBUG_LOG(LogLevel::TRACE, 
             "Initialized Root Scope with level: ", scopeLevel, 
             " | Memory Loc: ", this, 
             " | Parent Loc: ", formatPointer(nullptr));

    ++liveScopeCount;
    ++totalScopeCreated;
    DEBUG_FLOW_EXIT();
}


String Scope::formattedScope() {
    std::ostringstream oss;
    oss << this;
    return oss.str();
}

// Constructor for child
Scope::Scope(WeakPtr<Scope> parentScope, bool interpretMode)
    : parentScope(std::move(parentScope)), interpretMode(interpretMode) {
        DEBUG_FLOW(FlowLevel::LOW);
        if (auto parent = this->parentScope.lock()) {
            scopeLevel = parent->scopeLevel + 1;
            globalFunctions = parent->globalFunctions;
            globalClasses = parent->globalClasses;
        } else { throw MerkError("Scope Initialized Incorrectly"); }

        ++liveScopeCount;
        ++totalScopeCreated;
        DEBUG_FLOW_EXIT();
}

Scope::Scope(SharedPtr<Scope> parent, SharedPtr<FunctionRegistry> globalF, SharedPtr<ClassRegistry> globalC, bool interpretMode)
: parentScope(parent), 
    globalFunctions(std::move(globalF)),
    globalClasses(std::move(globalC)),
    interpretMode(interpretMode)
{
    scopeLevel = parent ? parent->scopeLevel + 1 : 0;
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
    }
}




const Context& Scope::getAllVariables(Context& callingContext) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    if (!context.getVariables().empty()){
        for (const auto& [varName, varPtr] : context.getVariables()) {
            callingContext.setVariable(varName, varPtr->uniqClone());
        }
    }


    if (auto parent = parentScope.lock()){
        if (parent && !parent->getContext().getVariables().empty()){
            parent->getAllVariables(callingContext);
        }

        return callingContext;

    } else {
        DEBUG_LOG(LogLevel::INFO, "parent expired or empty variables");
        return callingContext;
    }
    DEBUG_FLOW_EXIT();
    return callingContext;
}

const FunctionRegistry& Scope::getAllFunctions(FunctionRegistry& callingRegister) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    if (!globalFunctions->getFunctions().empty()){
        for (const auto& [funcName, funcVec] : globalFunctions->getFunctions()) {
            bool functionAlreadyExists = globalFunctions->hasFunction(funcName);

            for (const auto& funcSig : funcVec) {
                if (funcSig) {
                    if (!functionAlreadyExists) {
                        DEBUG_LOG(LogLevel::INFO, "[Scope::mergeScope] Registering function from definition: ", funcName);
                        callingRegister.registerFunction(funcName, funcSig);
                    } else {
                        DEBUG_LOG(LogLevel::INFO, "[Scope::mergeScope] Skipping function (already updated in current scope): ", funcName);
                    }
                }
            }
        }
    }
    
    DEBUG_LOG(LogLevel::INFO, "Added Own Functions");

    if (auto parent = parentScope.lock()){
        if (parent && !parent->getContext().getVariables().empty()){
            parent->getAllFunctions(callingRegister);
        }
        DEBUG_FLOW_EXIT();
        return callingRegister;

        
    } else {
        DEBUG_LOG(LogLevel::INFO, "parent expired or empty variables");
        DEBUG_FLOW_EXIT();
        return callingRegister;
    }
}

void Scope::appendChildScope(const SharedPtr<Scope>& child, const String& callerLabel, bool update) {
    (void)callerLabel;
    if (!child) {
        DEBUG_LOG(LogLevel::WARNING, "appendChildScope called from [", callerLabel, "] with null scope.");
        return;
    }

    DEBUG_LOG(LogLevel::ERROR, "appendChildScope called from [", callerLabel, "] | Parent Addr: ", this, " | Appending Child Addr: ", child.get());

    // Delegate to original
    appendChildScope(child, update);

}


bool Scope::hasImmediateChild(const SharedPtr<Scope>& candidate) {
    for (auto& child : childScopes) {
        if (child.get() == candidate.get()) return true;
    }
    return false;
}


bool Scope::has(const SharedPtr<Scope>& checkScope) {
    if (!checkScope) {
        throw MerkError("ChildScope for checking is null");
    }
    if (this == checkScope.get()) {
        return true;
    }
    for (auto& child : getChildren()) {
        if (child->has(checkScope)) {
            return true;
        }
    }
    return false;
}


void Scope::appendChildScope(SharedPtr<Scope> childScope, bool update) {
    MARK_UNUSED_MULTI(update);
    DEBUG_FLOW(FlowLevel::LOW);
    auto hasChild = this->has(childScope);
    if (!hasChild) {
        childScope->parentScope = shared_from_this();
        childScope->scopeLevel = getScopeLevel() + 1;
        childScopes.push_back(childScope);
        totalWithout += 1;
    } else {
        totalWith += 1;
    }

    // KEEEEEEEEEEEEP THIS BLOCK IT IS FOR AFTER SCOPING WITHIN CALLABLES IS DETERMINANT
    // childScope->parentScope = shared_from_this();
    // childScope->scopeLevel = getScopeLevel() + 1;
    // childScopes.push_back(childScope);

    childScope->isDetached = false;
    childScope->owner = owner;
    includeMetaData(childScope, false);
    
    if (update) { childScope->updateChildLevelsRecursively(); }
    DEBUG_FLOW_EXIT();
}


SharedPtr<Scope> Scope::createChildScope() {
    auto c = makeShared<Scope>(shared_from_this(),
                                     globalFunctions,
                                     globalClasses,
                                     interpretMode);
    c->isRoot = false;
    childScopes.push_back(c);
    return c;
  }

SharedPtr<Scope> Scope::getParent() const {
    DEBUG_FLOW(FlowLevel::LOW);
    if (parentScope.expired()) {
        DEBUG_FLOW_EXIT();
        return nullptr; // Explicitly return null for root scope
    }
    if (auto parent = parentScope.lock()) {
        // DEBUG_LOG(LogLevel::TRACE, "DEBUG Scope::getParent: Accessing Parent Scope at level: ", parent->getScopeLevel(), 
        //          " | Memory Loc: ", parent.get(), 
        //          " | Called from Scope Level: ", scopeLevel, " | Memory Loc: ", this);
        DEBUG_FLOW_EXIT();
        return parent;
    }

    throw ParentScopeNotFoundError();
}

void Scope::setVariable(const String& name, UniquePtr<VarNode> value, bool isDeclaration) {
    DEBUG_FLOW(FlowLevel::LOW);
    

    if (isDeclaration) {
        if (context.hasVariable(name)) { throw VariableAlreadyDeclaredError(name + " in Scope::setVariable", shared_from_this()); }
        context.setVariable(name, std::move(value));  // Declare variable

    } else {
        if (context.hasVariable(name)) {
            context.setVariable(name, std::move(value));  // Update in the current scope
        } else if (auto parent = parentScope.lock()) {
            parent->setVariable(name, std::move(value), isDeclaration);  // Delegate to parent scope
        } else {
            
            throw VariableNotFoundError(name + " in Scope::setVariable(2)");
        }
    }
    DEBUG_FLOW_EXIT();
}

void Scope::declareVariable(const String& name, UniquePtr<VarNode> value) {
    DEBUG_FLOW(FlowLevel::LOW);
    DEBUG_LOG(LogLevel::TRACE, "DEBUG Scope::declareVariable: Declaring variable: ", name, " = ", value, " | In Scope Level: ", this->getScopeLevel());

    if (name.empty()) { throw MerkError("Name is an empty string WITH: " + value->toString()); }
    if (context.hasVariable(name)) {
        if (disregardDeclarations) {
            context.setVariable(name, std::move(value));
            return;
        }
        throw VariableAlreadyDeclaredError(name, shared_from_this());
    }

    context.setVariable(name, std::move(value));
    DEBUG_FLOW_EXIT();
}

void Scope::updateVariable(const String& name, const Node& value) {
    if (context.hasVariable(name)) {
        context.updateVariable(name, value);
        return;
    }

    // Delegate to parent scope if the variable is not in the current scope
    if (auto parent = parentScope.lock()) {
        parent->updateVariable(name, value);
        return;
    }

    throw VariableNotFoundError(name);
}

VarNode& Scope::getVariable(const String& name) {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);

    if (auto variable = context.getVariable(name)) {
        return variable.value();
    }

    // Delegate to parent scope if it exists
    if (auto parent = parentScope.lock()) {
        return parent->getVariable(name);
    }

    if (hasVariable(name)) {throw MerkError("The Variable Lives Here, yet it is not getting pulled");}
    throw VariableNotFoundError(name);
}

// Check if a variable exists in the current scope or parent scopes
bool Scope::hasVariable(const String& name) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    if (context.hasVariable(name)) {
        DEBUG_FLOW_EXIT();
        return true;
    }

    // Check the parent scope
    if (auto parent = parentScope.lock()) {
        DEBUG_FLOW_EXIT();
        return parent->hasVariable(name);
    }

    DEBUG_FLOW_EXIT();
    return false;
}

// Get the current scope level
int Scope::getScopeLevel() const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    DEBUG_FLOW_EXIT();
    return scopeLevel;
}

const SharedPtr<FunctionRegistry> Scope::getFunctionRegistry() const {return globalFunctions;}
SharedPtr<FunctionRegistry> Scope::getFunctionRegistry() {return globalFunctions;}

void Scope::registerFunction(const String& name, SharedPtr<UserFunction> function) {
    DEBUG_FLOW(FlowLevel::MED);
    if (function->getCallableType() == CallableType::CLASS) {
        throw MerkError("Cannot Register A Class Into Function Registry");
    }
    auto signature = function->toCallableSignature();
    localFunctions.registerFunction(name, signature);
    DEBUG_FLOW_EXIT();
}

void Scope::registerFunction(const String& name, SharedPtr<Callable> anyCallable) {
    DEBUG_FLOW(FlowLevel::MED);
    if (anyCallable->getCallableType() == CallableType::CLASS) {
        throw MerkError("Cannot Register A Class Into Function Registry");
    }
    auto signature = anyCallable->toCallableSignature();
    localFunctions.registerFunction(name, signature);
    DEBUG_FLOW_EXIT();
}

void Scope::registerFunction(const String& name, SharedPtr<CallableSignature> signature) {
    DEBUG_FLOW(FlowLevel::MED);
    localFunctions.registerFunction(name, signature);
    DEBUG_FLOW_EXIT();
}

bool Scope::hasFunction(const String& name) const {
    if (auto func = lookupFunction(name)){
        return true;
    }
    if (auto parent = getParent()){
        return getParent()->hasFunction(name);
    } else {
    }

    return globalFunctions->hasFunction(name);
}


std::optional<Vector<SharedPtr<CallableSignature>>> Scope::lookupFunction(const String& name) const {
    if (globalFunctions && globalFunctions->hasFunction(name)) {
        auto sig = globalFunctions->getFunction(name);
        if (sig.has_value()) return sig;
    }

    if (auto funcs = localFunctions.getFunction(name)) { return funcs; }
    if (auto parent = parentScope.lock()) { return parent->lookupFunction(name); }
    
    return std::nullopt;
  }


std::optional<SharedPtr<CallableSignature>> Scope::lookupFunction(const String& name, const ArgResultType& args) const {
    if (globalFunctions && globalFunctions->hasFunction(name)) {
        auto sig = globalFunctions->getFunction(name, args);
        if (sig.has_value()) return sig;
    }

    if (auto sig = localFunctions.getFunction(name, args)) {
        if (sig.has_value()) {
            return sig;
        }
    }

    if (auto parent = parentScope.lock()) {
        return parent->lookupFunction(name, args);
    }
    
    return std::nullopt;
}

std::optional<SharedPtr<CallableSignature>> Scope::getFunction(const String& name, const ArgResultType& args) {
    DEBUG_FLOW(FlowLevel::MED);
    if (globalFunctions) {
        auto globalFunc = globalFunctions->getFunction(name, args);
        if (globalFunc.has_value()) {
            
            return globalFunc.value();
        }
    }

    if (auto func = lookupFunction(name, args)) {
        return func.value();
    }

    if (auto parent = parentScope.lock()) {
        return parent->getFunction(name, args);
    }

    DEBUG_FLOW_EXIT();
    return std::nullopt;
}

// For Function Reference
std::optional<Vector<SharedPtr<CallableSignature>>> Scope::getFunction(const String& name) {
    if (auto funcs = lookupFunction(name)) return funcs.value();
    if (auto parent = parentScope.lock()) return parent->getFunction(name);
    if (globalFunctions) {
        auto globals = globalFunctions->getFunction(name);
        if (globals) return globals.value();
    }

    return std::nullopt;
}





std::optional<SharedPtr<ClassSignature>> Scope::lookupClass(const String& name) const {
    if (auto local = localClasses.getClass(name)) {
        return local;
    }

    if (auto parent = parentScope.lock()) {
        return parent->lookupClass(name);
    }

    if (globalClasses && globalClasses->hasClass(name)) {
        return globalClasses->getClass(name);
    }

    return std::nullopt;
  }


void Scope::registerClass(const String& name, SharedPtr<ClassBase> classBase) {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    if (!classBase) {throw MerkError("registerClass called with null ClassBase for: " + name);}
    if (auto clsOpt = lookupClass(name); clsOpt && clsOpt.value()) {throw MerkError("Class '" + name + "' is already defined in this scope.");}
    if (!classBase->getCapturedScope()) {throw MerkError("No Captured Scope was set in class: " + name);}

    auto sig = std::static_pointer_cast<ClassSignature>(classBase->toCallableSignature());
    
    if (!sig) {throw MerkError("ClassSignature is null when registering: " + name);}

    localClasses.registerClass(name, std::move(sig));
    DEBUG_FLOW_EXIT();
}


std::optional<SharedPtr<ClassSignature>> Scope::getClass(const String& name) {
    if (auto local = localClasses.getClass(name)) {
        return local;
    }

    if (auto parent = parentScope.lock()) {
        return parent->getClass(name);
    }

    if (globalClasses) {
        return globalClasses->getClass(name);
    }

    return std::nullopt;
}


bool Scope::hasClass(const String& name) const {
    if (localClasses.hasClass(name)) {
        return true;
    }
    if (auto parent = parentScope.lock()) {
        return parent->hasClass(name);
    }
    return globalClasses && globalClasses->hasClass(name);
}

SharedPtr<ClassRegistry> Scope::getClassRegistry() {return globalClasses;}
