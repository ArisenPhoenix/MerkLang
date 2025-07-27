#include <iostream>
#include <unordered_set>

#include "core/types.h"
#include "core/errors.h"
#include "utilities/helper_functions.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"
#include "core/node.h"

#include "ast/ast_validate.h"
#include "ast/ast_class.h"
#include "ast/ast_chain.h"
#include "core/callables/callable.h"
#include "core/callables/classes/method.h"
#include "core/callables/param_node.h"

#include "core/context.h"
#include "core/registry/class_registry.h"
#include "core/registry/function_registry.h"
#include "core/errors.h"
#include "core/callables/functions/function.h"
#include "core/callables/classes/method.h"
#include "core/scope.h"
#include "core/callables/argument_node.h"

#include <cassert>

#include <execinfo.h>


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

    std::cout << "[Ctor(root)] this=" << this << " level=" << scopeLevel << "\n";

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

        std::cout << "[Ctor(child)] this=" << this << " parent=" << parentScope.lock().get() << "\n";
        if (auto parent = this->parentScope.lock()) {
            DEBUG_LOG(LogLevel::TRACE, "Parent scope locked at level ", parent->scopeLevel);

            scopeLevel = parent->scopeLevel + 1; // Correctly increment scope level
            DEBUG_LOG(LogLevel::TRACE, "Child Scope Initialized with Parent Scope Level: ", parent->scopeLevel,
                    " | Parent Memory Loc: ", parent.get(), 
                    " | Child Memory Loc: ", this);
            DEBUG_LOG(LogLevel::TRACE, "Inherited globalCallables from parent.");
            globalFunctions = parent->globalFunctions;
            globalClasses = parent->globalClasses;


        } else {
            throw MerkError("Scope Initialized Incorrectly");

        }

        // Log after ensuring all members are initialized
        DEBUG_LOG(LogLevel::TRACE, 
                "Finalized Scope Initialization with level: ", scopeLevel, 
                " | Memory Loc: ", static_cast<void*>(this), 
                " | Parent Loc: ", formatPointer(this->parentScope.lock().get()));
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
            DEBUG_LOG(LogLevel::TRACE, "Destroying Root Scope.");
        } else {
            DEBUG_LOG(LogLevel::TRACE, "Preserving Root Scope for compilation.");

            return; // Avoid clearing child scopes in compile mode
        }
    } else {
        DEBUG_LOG(LogLevel::TRACE, "Destroying Scope at level: ", scopeLevel, 
                 " | Memory Loc: ", this, 
                 " | Parent Loc: ", parentScope.lock().get());
 
    
    clear();
    --liveScopeCount;
    DEBUG_LOG(LogLevel::TRACE, "[Scope Destructor] Remaining Scopes: ", liveScopeCount);
    DEBUG_LOG(LogLevel::TRACE, "Exiting Scope Destructor for Scope Level: ", scopeLevel, 
             " | Memory Loc: ", this);
    }




    DEBUG_FLOW_EXIT();
}

void Scope::clear(bool internalCall) {
    auto weak = weak_from_this();

    DEBUG_LOG(LogLevel::TRACE,
        "Destroying Scope addr=", this,
        " owner=", owner,
        " parent=", getParent() ? getParent().get() : 0,
        " (captured?) children=", childScopes.size(),
        " use_count=", weak.use_count()
    );
    
        
    if (owner.find("ClassInstance") != std::string::npos) {
        String funcs;
        for (auto& [funcName, funcSigs] : localFunctions) {(void)funcSigs; funcs += ", " + funcName;}
        String classes;
        DEBUG_LOG(LogLevel::TRACE,
                  "Destroying Instance Scope functions=", localFunctions.size(), metaString(), "Scope Address: ", formattedScope(), "Funcs: ", funcs);
    }
    context.clear();
    if (isRoot && internalCall) {
        DEBUG_LOG(LogLevel::TRACE, metaString(), "InternalCall: ", (internalCall ? "true": "false"));
        auto funIt = localFunctions.find("pop");
        if (funIt != localFunctions.end()) {throw MerkError("Trying To Destroy Scope containing pop Method");}

        auto it = localClasses.find("List");
        if (it != localClasses.end()) {throw MerkError("Trying To Destroy Scope containing List class");}
    }
    

    
    
    localFunctions.clear();
    localClasses.clear();

    for (auto& child : childScopes) {
        if (child) {
            child->clear();  // Recursively clear
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
        DEBUG_LOG(LogLevel::INFO, "passed not empty");
        for (const auto& [varName, varPtr] : context.getVariables()) {
            callingContext.setVariable(varName, makeUnique<VarNode>(*varPtr));
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
    if (!child) {
        DEBUG_LOG(LogLevel::WARNING, "appendChildScope called from [", callerLabel, "] with null scope.");
        return;
    }

    DEBUG_LOG(LogLevel::ERROR, "appendChildScope called from [", callerLabel, "] | Parent Addr: ", this, " | Appending Child Addr: ", child.get());

    // Delegate to the original
    appendChildScope(child, update);

}


void Scope::appendChildScope(SharedPtr<Scope> childScope, bool update) {
    DEBUG_FLOW(FlowLevel::LOW);
    if (shared_from_this().use_count() == 0)
    throw std::runtime_error("Oops: calling shared_from_this() on a non‑shared object!");
    auto hasChild = this->has(childScope);
    if (!hasChild){
        childScope->parentScope = shared_from_this();
        childScope->scopeLevel = getScopeLevel() + 1;
        childScopes.push_back(childScope);
    } 
    // else {
    //     throw MerkError("Circular dependency found for " + childScope->formattedScope());
    // }

    childScope->isDetached = false;
    childScope->owner = owner;
    includeMetaData(childScope, false);
    
    if (update) {
        childScope->updateChildLevelsRecursively();
    }
    DEBUG_FLOW_EXIT();
}


// void Scope::appendChildScope(SharedPtr<Scope> childScope, bool update) {
//     DEBUG_FLOW(FlowLevel::LOW);
//     if (shared_from_this().use_count() == 0)
//         throw std::runtime_error("Oops: calling shared_from_this() on a non‑shared object!");

//     // Only check immediate children
//     for (auto& child : childScopes) {
//         if (child && child == childScope) {
//             DEBUG_LOG(LogLevel::PERMISSIVE, "appendChildScope: already immediate child ", child.get());
//             return;
//         }
//     }

//     if (auto oldParent = childScope->getParent()) {
//         auto& siblings = oldParent->childScopes;
//         siblings.erase(
//             std::remove_if(siblings.begin(), siblings.end(),
//                         [&](auto& c) { return c.get() == childScope.get(); }),
//             siblings.end()
//         );
//     }

//     childScope->parentScope = shared_from_this();
//     childScope->scopeLevel = getScopeLevel() + 1;
//     childScopes.push_back(childScope);

//     childScope->isDetached = false;
//     childScope->owner = owner;
//     includeMetaData(childScope, false);

//     if (update) {
//         childScope->updateChildLevelsRecursively();
//     }
//     DEBUG_FLOW_EXIT();
// }


SharedPtr<Scope> Scope::createChildScope() {
    auto c = makeShared<Scope>(shared_from_this(),
                                     globalFunctions,
                                     globalClasses,
                                     interpretMode);
    childScopes.push_back(c);
    return c;
  }


// UniquePtr<VarNode> cloneVar(VarNode* original) {
//     return UniquePtr<VarNode>(original);
// }





SharedPtr<Scope> Scope::getParent() const {
    DEBUG_FLOW(FlowLevel::LOW);
    if (parentScope.expired()) {
        DEBUG_LOG(LogLevel::TRACE, "DEBUG Scope::getParent: No Parent for Root Scope.");
        DEBUG_FLOW_EXIT();
        return nullptr; // Explicitly return null for root scope
    }
    if (auto parent = parentScope.lock()) {
        DEBUG_LOG(LogLevel::TRACE, "DEBUG Scope::getParent: Accessing Parent Scope at level: ", parent->getScopeLevel(), 
                 " | Memory Loc: ", parent.get(), 
                 " | Called from Scope Level: ", scopeLevel, " | Memory Loc: ", this);
        DEBUG_FLOW_EXIT();
        return parent;
    }

    DEBUG_LOG(LogLevel::TRACE, "DEBUG Scope: No Parent Scope Found for Scope Level: ", scopeLevel, " | Memory Loc: ", this);
    throw ParentScopeNotFoundError();
}

void Scope::setVariable(const String& name, UniquePtr<VarNode> value, bool isDeclaration) {
    DEBUG_FLOW(FlowLevel::LOW);
    

    if (isDeclaration) {
        if (context.hasVariable(name)) {
            throw MerkError("DEBUG Scope::setVariable:  Variable '" + name + "' is already declared in this scope.");
        }
        context.setVariable(name, std::move(value));  // Declare variable

    } else {
        if (context.hasVariable(name)) {
            context.setVariable(name, std::move(value));  // Update in the current scope
        } else if (auto parent = parentScope.lock()) {
            parent->setVariable(name, std::move(value), isDeclaration);  // Delegate to parent scope
        } else {
            DEBUG_LOG(LogLevel::ERROR, "From Scope::setVariable");
            
            throw VariableNotFoundError(name);
        }
    }
    DEBUG_FLOW_EXIT();
}

void Scope::declareVariable(const String& name, UniquePtr<VarNode> value) {
    DEBUG_FLOW(FlowLevel::LOW);
    DEBUG_LOG(LogLevel::TRACE, "DEBUG Scope::declareVariable: Declaring variable: ", name, " = ", value, " | In Scope Level: ", this->getScopeLevel());
    // if (owner == "ClassBase(Square)" && name != "x" && name != "y" && scopeLevel == 3) {throw MerkError("ClassBase(Square) attempted to allow in a non instance variable: " + name);}
    // Set the variable in the current context (not parent or child)
    if (context.hasVariable(name)) {
        DEBUG_LOG(LogLevel::DEBUG, "SCOPE: ", owner, "SCOPE LEVEL: ", scopeLevel);
        if (disregardDeclarations) {
            context.setVariable(name, std::move(value));
            return;
        }
        throw VariableAlreadyDeclaredError(name, shared_from_this());
    }
    // DEBUG_LOG(LogLevel::PERMISSIVE, "VarNode is: ", value->toString());
    // DEBUG_LOG(LogLevel::PERMISSIVE, "VarNode FullType: ", value->getFullType().toString());
    // throw MerkError("See Above");
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

    DEBUG_LOG(LogLevel::ERROR, "From Scope::updateVariable");
    throw VariableNotFoundError(name);
}

VarNode& Scope::getVariable(const String& name) {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    DEBUG_LOG(LogLevel::PERMISSIVE, "in scope: ", getScopeLevel(), "DEBUG DATA BELOW:");
    context.debugPrint();

    if (auto variable = context.getVariable(name)) {
        return variable.value();
    }

    // Delegate to parent scope if it exists
    if (auto parent = parentScope.lock()) {
        return parent->getVariable(name);
    }

    DEBUG_LOG(LogLevel::ERROR, "From Scope::getVariable");
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

void Scope::handleFunctionRegistration(String funcMethName, SharedPtr<CallableSignature> funcMethSig) {
    auto primaryType = callableTypeAsString(funcMethSig->getCallableType());
    auto subType = callableTypeAsString(funcMethSig->getSubType());

    // DEBUG_LOG(LogLevel::PERMISSIVE, "Current ", primaryType, " Being Registered: ", funcMethName, " Type: ", primaryType, " SubType: ", subType);
    if (!funcMethSig){throw MerkError("Not a Method Signature");}

    DEBUG_LOG(LogLevel::TRACE, "FUNC TYPE: ", primaryType, " FUNC SUB_TYPE: ", subType, " FUNC NAME: ", funcMethName);

    if (funcMethSig->getSubType() == CallableType::NATIVE) {
        auto previousOpt = lookupFunction(funcMethName);
        if (previousOpt){
            auto& previous = previousOpt.value();
            bool replaced = false;
            for (auto& prev: previous) {
                // DEBUG_LOG(LogLevel::PERMISSIVE, "Checking Parameter Matches");
                if (prev->matches(funcMethSig->getParameterTypes())){
                    
                    prev = funcMethSig;
                    replaced = true;
                    break;
                }
            }
            if (!replaced) {
                previous.push_back(funcMethSig);  // Append if not found
            } 
        }
        else {
            localFunctions[funcMethName] = {funcMethSig};  // First time
        }
        return;
    }
    
    if (funcMethSig->getSubType() == CallableType::DEF) {
        auto previousOpt = lookupFunction(funcMethName);
        if (previousOpt) {
            auto& previous = previousOpt.value();
            // localFunctions[funcMethName].emplace_back(funcMethSig);
            bool replaced = false;
            for (auto& prev : previous) {
                if (prev->getSubType() == CallableType::DEF) {
                    prev = funcMethSig;  // Replace just the DEF
                    replaced = true;
                    break;
                }
            }
            if (!replaced) {
                previous.push_back(funcMethSig);  // Append if not found
            } 
        } else {
            localFunctions[funcMethName] = {funcMethSig};  // First time
        }
    return;
    }

    else if (funcMethSig->getSubType() == CallableType::FUNCTION) {
        auto previousOpt = lookupFunction(funcMethName);
        if (previousOpt){
            auto& previous = previousOpt.value();
            for (auto& prev: previous) {
            if (prev->matches(funcMethSig->getParameterTypes())){
                throw FunctionRedefinitionError(funcMethName);
            }
        }
        }
        
        localFunctions[funcMethName].emplace_back(funcMethSig);
        return;
    }


    throw MerkError("Not A Function: " + funcMethName);
}



void Scope::registerFunction(const String& name, SharedPtr<UserFunction> function) {
    DEBUG_FLOW(FlowLevel::MED);
    if (function->getCallableType() == CallableType::CLASS) {
        throw MerkError("Cannot Register A Class Into Function Registry");
    }
    auto signature = function->toCallableSignature();
    handleFunctionRegistration(name, signature);
    
    DEBUG_FLOW_EXIT();
}

void Scope::registerFunction(const String& name, SharedPtr<Callable> anyCallable) {
    DEBUG_FLOW(FlowLevel::MED);
    if (anyCallable->getCallableType() == CallableType::CLASS) {throw MerkError("Cannot Register A Class Into Function Registry");}
    DEBUG_LOG(LogLevel::TRACE, "anyCallType: ", callableTypeAsString(anyCallable->getCallableType()), "anySubType: ", callableTypeAsString(anyCallable->getSubType()));
    // throw MerkError("Got AnyCallable Meta");
    auto signature = anyCallable->toCallableSignature();
    auto other = signature->getCallable();
    DEBUG_LOG(LogLevel::TRACE, "otherCallType: ", callableTypeAsString(other->getCallableType()), "otherSubType: ", callableTypeAsString(other->getSubType()));
    if (other->getCallableType() != anyCallable->getCallableType() || other->getSubType() != anyCallable->getSubType()) {throw MerkError("starting method and signature held method meta don't match");}
    handleFunctionRegistration(name, signature);

    DEBUG_FLOW_EXIT();
}

void Scope::registerFunction(const String& name, SharedPtr<CallableSignature> signature) {
    DEBUG_FLOW(FlowLevel::MED);
    handleFunctionRegistration(name, signature);
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
    DEBUG_FLOW(FlowLevel::MED);
    auto it = localFunctions.find(name);
    if (it != localFunctions.end() && !it->second.empty()) {
        
        DEBUG_FLOW_EXIT();
        return std::optional<Vector<SharedPtr<CallableSignature>>>(it->second);
    }

    DEBUG_FLOW_EXIT();
    return std::nullopt;
  }


std::optional<SharedPtr<CallableSignature>> Scope::lookupFunction(const String& name, const ArgResultType& args) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    auto it = localFunctions.find(name);
    Vector<NodeValueType> argTypes;

    if (it != localFunctions.end()) {
        DEBUG_LOG(LogLevel::TRACE, "FOUND FUNCTION In lookupFunction, args not checked yet");
        for (auto &arg : args) {
            argTypes.push_back(arg.getType());
            DEBUG_LOG(LogLevel::TRACE, "ArgType: ", nodeTypeToString(arg.getType()));
        }

        for (auto candidate : it->second) {
            return candidate;
            // if (candidate->matches(argTypes)){
            //     DEBUG_FLOW_EXIT();
            //     return candidate;
            // } 
        }
        
    }

    if (auto parent = parentScope.lock()){
        return parent->lookupFunction(name, args);
    }

    DEBUG_LOG(LogLevel::ERROR, "This is actually an Argument mismatch error");
    DEBUG_FLOW_EXIT();
    throw FunctionNotFoundError(name);
    return nullptr;
}

SharedPtr<CallableSignature> Scope::getFunction(const String& name, const ArgResultType& args) {
    DEBUG_FLOW(FlowLevel::MED);
    if (auto func = lookupFunction(name, args)) {
        return func.value();
    }

    if (auto parent = parentScope.lock()) {
        return parent->getFunction(name, args);
    }

    if (auto function = globalFunctions->getFunction(name, args)) { // returns std::optional<std::reference_wrapper<SharedPtr<CallableSignature>>>
        return function.value();
    }

    DEBUG_FLOW_EXIT();
    throw FunctionNotFoundError(name);
}

// For Function Reference
Vector<SharedPtr<CallableSignature>> Scope::getFunction(const String& name) {
    if (auto func = lookupFunction(name)){
        return func.value();
    }


    if (auto parent = parentScope.lock()){
        return parent->getFunction(name);
    }

    if (auto functions = globalFunctions->getFunction(name)){
        return functions.value();
    }
    throw FunctionNotFoundError(name);
}





std::optional<SharedPtr<ClassSignature>> Scope::lookupClass(const String& name) const {
    if (globalClasses->hasClass(name) ) { return globalClasses->getClass(name);}

    auto it = localClasses.find(name);
    if (it != localClasses.end())
      return it->second;
  
    if (auto p = parentScope.lock()) {
      if (auto found = p->lookupClass(name))
        return found;
    }
  
    // auto cls = globalClasses->getClass(name);
    // if (cls) {
    //     return cls.value();
    // }
    return std::nullopt;
  }


void Scope::registerClass(const String& name, SharedPtr<ClassBase> classBase) {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    DEBUG_LOG(LogLevel::PERMISSIVE, "Registering Class: ", name);
    // throw MerkError("Registering class " + name);
    if (!classBase) {throw MerkError("registerClass called with null ClassBase for: " + name);}
    if (auto clsOpt = lookupClass(name); clsOpt && clsOpt.value()) {throw MerkError("Class '" + name + "' is already defined in this scope.");}
    if (!classBase->getCapturedScope()) {throw MerkError("No Captured Scope was set in class: " + name);}
    if (!classBase->getCapturedScope()->has(classBase->getClassScope())) {throw MerkError("Class Scope is Not A CHILD OF CAPTURED SCOPE");}

    auto sig = std::static_pointer_cast<ClassSignature>(classBase->toCallableSignature());
    
    if (!sig) {throw MerkError("ClassSignature is null when registering: " + name);}

    

    localClasses[name] = std::move(sig);
    // DEBUG_LOG(LogLevel::PERMISSIVE, "Added class to localClasses");
    DEBUG_FLOW_EXIT();
}


std::optional<SharedPtr<ClassSignature>> Scope::getClass(const String& name) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (name.empty()) {throw MerkError("String name for getClass Is Empty");}

    if (globalClasses) {
        auto globalClass = globalClasses->getClass(name);
        if (globalClass) {
            DEBUG_FLOW_EXIT();
            if (!globalClass.has_value() || !globalClass.value()) {throw MerkError("Global Class is Null");}
            return globalClass;
        };
    }
    
    auto it = localClasses.find(name);
    if (it != localClasses.end()) {return it->second;}
    

    auto parent = parentScope.lock();
    if (parent) {
        
        auto cls = parent->getClass(name);
        if (!cls) {throw MerkError("cls itself is null");}
        if (!cls.has_value() || !cls.value()) {throw MerkError("cls is null");}
        DEBUG_FLOW_EXIT();
        return cls;
    }

    DEBUG_FLOW_EXIT();
    return std::nullopt;
}


bool Scope::hasClass(const String& name) const {
    if (auto cls = lookupClass(name)){
        return true;
    }

    if (auto parent = getParent()){
        return parent->hasClass(name);
    }

    return globalClasses->hasClass(name);
}

SharedPtr<ClassRegistry> Scope::getClassRegistry() {return globalClasses;}
