#include <iostream>
#include <unordered_set>

#include "core/types.h"
#include "utilities/helper_functions.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"
#include "core/node.h"

#include "ast/ast_validate.h"
#include "ast/ast_class.h"
#include "ast/ast_chain.h"
#include "core/functions/callable.h"
#include "core/classes/method.h"






#include "core/functions/param_node.h"

#include "core/context.h"
#include "core/registry/class_registry.h"
#include "core/registry/function_registry.h"

// #include "core/functions/native_functions.h"
#include "core/errors.h"
#include "core/functions/function_node.h"
#include "core/classes/method.h"
#include "core/scope.h"

#include <cassert>

#include <execinfo.h>

// at top of scope_main.cpp
// static void print_trace() {
//   void*  bt[16];
//   int    cnt = backtrace(bt, 16);
//   char** syms= backtrace_symbols(bt, cnt);
//   std::cerr << "--- Scope constructed from:\n";
//   for(int i=1; i<cnt; ++i)  // skip frame[0] itself
//     std::cerr << "  " << syms[i] << "\n";
//   free(syms);
// }


// static SharedPtr<Scope> makeRoot(int lvl,bool m) { 
//     return makeShared<Scope>(lvl,m);
//   }
//   static SharedPtr<Scope> makeChild(SharedPtr<Scope> parent,bool m) {
//     return makeShared<Scope>(parent,m);
//   }



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
Scope::Scope(int scopeLevel, bool interpretMode, bool isRoot) 
    : scopeLevel(scopeLevel), interpretMode(interpretMode), isRoot(isRoot) {
        DEBUG_FLOW(FlowLevel::LOW);
    // DEBUG_LOG(LogLevel::INFO, "Initialized Scope with level: ", scopeLevel, " | Memory Loc: ", this, " | Parent Loc: ", parentScope.lock().get());
    // std::cout << "Attempting to allocate globalCallables...\n";
    std::cout << "[Ctor(root)] this=" << this << " level=" << scopeLevel << "\n";
    // print_trace();
    // globalCallables = makeShared<std::unordered_map<String, CallableType>>();
    // std::cout << "Success\n";
    if (isRoot) {
        globalFunctions = makeShared<FunctionRegistry>();
        globalClasses = makeShared<ClassRegistry>();
        // if (!globalFunctions) ;
        // if (!globalClasses) ;

    }
    DEBUG_LOG(LogLevel::TRACE, 
             "Initialized Root Scope with level: ", scopeLevel, 
             " | Memory Loc: ", this, 
             " | Parent Loc: ", formatPointer(nullptr));
    DEBUG_FLOW_EXIT();
}

// Constructor for child
Scope::Scope(WeakPtr<Scope> parentScope, bool interpretMode)
    : parentScope(std::move(parentScope)), interpretMode(interpretMode) {
        DEBUG_FLOW(FlowLevel::LOW);

        std::cout << "[Ctor(child)] this=" << this << " parent=" << parentScope.lock().get() << "\n";
        // print_trace();
    // Lock the parent scope to determine the current scope level
    if (auto parent = this->parentScope.lock()) {
        DEBUG_LOG(LogLevel::TRACE, "Parent scope locked at level ", parent->scopeLevel);

        scopeLevel = parent->scopeLevel + 1; // Correctly increment scope level
        DEBUG_LOG(LogLevel::TRACE, "Child Scope Initialized with Parent Scope Level: ", parent->scopeLevel,
                 " | Parent Memory Loc: ", parent.get(), 
                 " | Child Memory Loc: ", this);
        // globalCallables = parent->globalCallables;  // Share the same registry!
        DEBUG_LOG(LogLevel::TRACE, "Inherited globalCallables from parent.");
        globalFunctions = parent->globalFunctions;
        globalClasses = parent->globalClasses;


    } else {
        // // If no parent is found, this becomes the root scope
        // DEBUG_LOG(LogLevel::TRACE, "Parent scope locked at level ", parent->scopeLevel);

        // scopeLevel = 0; 
        // DEBUG_LOG(LogLevel::TRACE, "Root Scope Initialized | Memory Loc: ", this);
        // // globalCallables = makeShared<std::unordered_map<String, CallableType>>(); // Fallback
        // DEBUG_LOG(LogLevel::TRACE, "Inherited globalCallables from parent.");
        throw MerkError("Scope Initialized Incorrectly");

    }

    // Log after ensuring all members are initialized
    DEBUG_LOG(LogLevel::TRACE, 
            "Finalized Scope Initialization with level: ", scopeLevel, 
            " | Memory Loc: ", static_cast<void*>(this), 
            " | Parent Loc: ", formatPointer(this->parentScope.lock().get()));
    
    DEBUG_FLOW_EXIT();
}

Scope::Scope(SharedPtr<Scope> parent, SharedPtr<FunctionRegistry> globalF, SharedPtr<ClassRegistry> globalC, bool interpretMode)
: parentScope(parent), scopeLevel( parent ? parent->scopeLevel + 1 : 0 ), 
    globalFunctions(std::move(globalF)),
    globalClasses(std::move(globalC)),
    interpretMode(interpretMode)
{}

Scope::~Scope() {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    DEBUG_LOG(LogLevel::TRACE, "Entering Scope Destructor for Scope Level: ", scopeLevel, 
             " | Memory Loc: ", this);
    if (scopeLevel == 0) {
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
        // functionRegistry.clear();  // <--- Also needed if functions are stored in this scope
        // 1. Explicitly destroy stored variables
        // DEBUG_LOG(LogLevel::INFO, "Still Need to Handle Cleaning up variables in Context Scope Level: ", scopeLevel);

        // 2. Explicitly destroy stored functions
        // DEBUG_LOG(LogLevel::INFO, "Still Need To Handle Cleaning up functions in Registry in Scope Level: ", scopeLevel);

        // Explicitly destroy child scopes in reverse order
 

    clear();
        
    DEBUG_LOG(LogLevel::TRACE, "Exiting Scope Destructor for Scope Level: ", scopeLevel, 
             " | Memory Loc: ", this);
    }


    DEBUG_FLOW_EXIT();
}

void Scope::clear() {
    for (auto it = childScopes.rbegin(); it != childScopes.rend(); ++it) {
        DEBUG_LOG(LogLevel::TRACE,
                  "  -> destroying child level ", (*it)->getScopeLevel(),
                  " @", (*it).get());
        it->reset();  
    }
    
    context.clear();
    childScopes.clear();
    if (isRoot){
        // if (globalClasses->getClasses().size() > 0) {
        //     globalClasses->clear();
        // }
        // if (globalFunctions->getFunctions().size() > 0){
        //     globalFunctions->clear();

        // }
        // classRegistry.clear();          // Safe and sufficient
        // functionRegistry.clear();       // Safe and sufficient
    }

    if (localClasses.size() > 0){
        localClasses.clear();
    }

    if (localFunctions.size() > 0) {
        localFunctions.clear();

    }
    
}



const Context& Scope::getAllVariables(Context& callingContext) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    if (!context.getVariables().empty()){
        DEBUG_LOG(LogLevel::INFO, "passed not empty");
        for (const auto& [varName, varPtr] : context.getVariables()) {
            // debugLog(true, "attempting to add variable:", varName, "Value:", varPtr.get());
            callingContext.setVariable(varName, makeUnique<VarNode>(*varPtr));
        }
    }

    // debugLog(true, "Added Own Variables");

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
                        // callingRegister.registerFunction(funcName, getFunctionRegistry().getFunction(funcName)->get().getFunction()->toFunctionSignature());

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
    
    childScope->parentScope = shared_from_this();
    childScope->scopeLevel = getScopeLevel() + 1;
    childScope->isDetached = false;
    childScope->owner = owner;
    includeMetaData(childScope, false);
    childScopes.push_back(childScope);
    if (update) {
        childScope->updateChildLevelsRecursively();
    }
    DEBUG_FLOW_EXIT();
}
// In Scope.cpp
void Scope::attachToInstanceScope(SharedPtr<Scope> instanceScope) {
    // Set the parent to the instance's scope
    (void)instanceScope;
    throw MerkError("AttachTOInstanceScope should not be used");
    // setParent(instanceScope);
    // scopeLevel = instanceScope->getScopeLevel() + 1;
    // // Optionally append this to the instanceScope's children
    // instanceScope->appendChildScope(shared_from_this());
}




SharedPtr<Scope> Scope::createChildScope() {
    auto c = makeShared<Scope>(shared_from_this(),
                                     globalFunctions,
                                     globalClasses,
                                     interpretMode);
    childScopes.push_back(c);
    return c;
  }


SharedPtr<Scope> Scope::makeCallScope() {
    auto c = makeShared<Scope>(shared_from_this(),
                                globalFunctions,
                                globalClasses,
                                interpretMode);
    c->isCallableScope = true;
    c->isDetached = true;
    c->isClonedScope = true;
    c->isRoot = false;
    return c;
}

SharedPtr<Scope> Scope::makeInstanceScope(SharedPtr<Scope> classScope) {
    auto instanceScope = makeCallScope(); // inherits captured env

    // instanceScope->appendChildScope(classScope->getParent(), "InstanceScope.attachClassScope");
    instanceScope->isDetached = true;
    includeMetaData(instanceScope, true);

    instanceScope->owner = generateScopeOwner("ClassInstance", classScope->owner);

    const auto& classVars = classScope->getContext().getVariables();

    for (const auto& [varName, varNodePtr] : classVars) {
        if (varNodePtr) {
            // Clone the VarNode (you might want to make a proper clone method later)
            auto clonedVar = makeUnique<VarNode>(*varNodePtr); 
            instanceScope->declareVariable(varName, std::move(clonedVar));
        }
    }

    // New: Clone methods and patch chains
    const auto& functionRegistry = classScope->getFunctionRegistry()->getFunctions();
    for (const auto& [methodName, methodVec] : functionRegistry) {
        for (const auto& methodSig : methodVec) {
            auto method = std::static_pointer_cast<Method>(methodSig->getCallable());
            if (!method) {
                throw MerkError("Method Not Found: " + methodName);
            }

            // Clone method
            SharedPtr<Method> clonedMethod = makeShared<Method>(*method);
            clonedMethod->setClassScope(instanceScope);

            // Patch Chains
            auto body = clonedMethod->getBody();
            String accessor = clonedMethod->getAccessor();
            ASTUtils::traverse(body->getChildren(), [&](BaseAST* node) {
                if (node->getAstType() == AstType::ChainOperation) {
                    auto* chainOp = static_cast<ChainOperation*>(node);
                    chainOp->setResolutionMethod(1, ResolutionMode::ClassInstance, instanceScope, accessor);
                }
            }, true, false);

            // Register into instanceScope
            instanceScope->registerFunction(methodName, clonedMethod->toCallableSignature());
        }
    }

    return instanceScope;
}





// void Scope::setProtectedMembers(Vector<String> protectedMems) {protectedMembers = protectedMems;}
// Vector<String> Scope::getProtectedMembers() const {return protectedMembers;}



// similarly for lookupClass, lookupVariable, etc…

// registering in *this* scope now only writes into your overlay:
// void Scope::registerFunctionHere(const String& name, SharedPtr<CallableSignature> f) {
//   localFunctions[name] = std::move(f);
// }

// Get the parent scope
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
            throw VariableNotFoundError(name);
        }
    }
    DEBUG_FLOW_EXIT();
}

void Scope::declareVariable(const String& name, UniquePtr<VarNode> value) {
    DEBUG_FLOW(FlowLevel::LOW);
    DEBUG_LOG(LogLevel::TRACE, "DEBUG Scope::declareVariable: Declaring variable: ", name, " = ", value, " | In Scope Level: ", this->getScopeLevel());

    // Set the variable in the current context (not parent or child)
    if (context.hasVariable(name)) {
        throw VariableAlreadyDeclaredError(name);
    }
    context.setVariable(name, std::move(value));
    DEBUG_FLOW_EXIT();
}

void Scope::updateVariable(const String& name, const Node& value) {
    // DEBUG_FLOW(FlowLevel::LOW);
    // DEBUG_LOG(LogLevel::TRACE, "Attempting to update variable: ", name, " in scope level: ", scopeLevel);

    // Check if the variable exists in the current scope
    if (context.hasVariable(name)) {
        // DEBUG_LOG(LogLevel::TRACE, "Variable found in current scope. Updating: ", name, " to value: ", value);
        context.updateVariable(name, value);
        // DEBUG_FLOW_EXIT();
        return;
    }

    // Delegate to parent scope if the variable is not in the current scope
    if (auto parent = parentScope.lock()) {
        // DEBUG_LOG(LogLevel::TRACE, "Variable not found in current scope. Delegating update to parent scope.");
        parent->updateVariable(name, value);
        // DEBUG_FLOW_EXIT();
        return;
    }

    // If the variable is not found in any scope, throw an error
    // DEBUG_LOG(LogLevel::TRACE, "Variable not found in any scope. Throwing error for: ", name);
    // DEBUG_FLOW_EXIT();

    DEBUG_LOG(LogLevel::DEBUG, highlight("Scope Data Before Error", Colors::pink));
    // debugPrint();
    throw VariableNotFoundError(name);
}

VarNode& Scope::getVariable(const String& name) {
    // DEBUG_FLOW(FlowLevel::LOW);
    // DEBUG_LOG(LogLevel::DEBUG, "[Scope::getVariable] Looking for variable '", name, "' in scope level: ", getScopeLevel());

    if (auto variable = context.getVariable(name)) {
        // DEBUG_LOG(LogLevel::TRACE, highlight("[Scope::getVariable]", Colors::orange), "Found variable '", name, "' in scope level: ", getScopeLevel());
        // if (variable) {
        //     DEBUG_LOG(LogLevel::TRACE, highlight("[Scope::getVariable]", Colors::orange), 
        //     "Found variable '", name, "= ", *variable,
        //     "' in scope level: ", getScopeLevel());  
        // }

        // DEBUG_FLOW_EXIT();
        return variable->get();
    }

    // Delegate to parent scope if it exists
    if (auto parent = parentScope.lock()) {
        if (parent.get() == this) { 
            throw MerkError("Scope::getVariable: Parent scope is same as current scope (cyclic reference).");
        }
        DEBUG_LOG(LogLevel::TRACE, "[Scope::getVariable] Variable '", name, "' not found in scope level ", 
                 getScopeLevel(), ". Checking parent scope.");
        // DEBUG_FLOW_EXIT();
        return parent->getVariable(name);
    }

    // DEBUG_LOG(LogLevel::DEBUG, highlight("Scope Context Before Error:", Colors::pink));
    // context.debugPrint();
    // DEBUG_FLOW_EXIT();
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
    // if (!this){
    //     throw MerkError("Scope is Null");
    // }
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    DEBUG_FLOW_EXIT();
    return scopeLevel;
}


// const FunctionRegistry Scope::getFunctionRegistry() const { return globalClasses; }
// FunctionRegistry Scope::getFunctionRegistry() { return functionRegistry; }

const SharedPtr<FunctionRegistry> Scope::getFunctionRegistry() const {return globalFunctions;}
SharedPtr<FunctionRegistry> Scope::getFunctionRegistry() {return globalFunctions;}


void Scope::registerFunction(const String& name, SharedPtr<UserFunction> function) {
    DEBUG_FLOW(FlowLevel::MED);
    // auto signature = function->toFunctionSignature();
    auto signature = function->toCallableSignature();

    // functionRegistry.registerFunction(name, std::move(signature));
    localFunctions[name].emplace_back(signature);

    DEBUG_FLOW_EXIT();
}

void Scope::registerFunction(const String& name, SharedPtr<CallableSignature> signature) {
    DEBUG_FLOW(FlowLevel::MED);

    // functionRegistry.registerFunction(name, signature);
    localFunctions[name].emplace_back(signature);
    DEBUG_FLOW_EXIT();
}

bool Scope::hasFunction(const String& name) const {
    // auto func = lookupFunction(name);

    if (auto func = lookupFunction(name)){
        // DEBUG_LOG(LogLevel::PERMISSIVE, "FOUND FUNCTION THROUGH Lookup");
        return true;
    } else {
        // DEBUG_LOG(LogLevel::PERMISSIVE, "DID NOT FIND FUNCTION THROUGH Lookup");

    }
    if (auto parent = getParent()){
        // DEBUG_LOG(LogLevel::PERMISSIVE, "FOUND FUNCTION THROUGH Parent Lookup");

        return getParent()->hasFunction(name);
    } else {
        // DEBUG_LOG(LogLevel::PERMISSIVE, "DID NOT FIND FUNCTION THROUGH Parent Lookup");
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


// lookup a function: first in your local overlay
std::optional<SharedPtr<CallableSignature>> Scope::lookupFunction(const String& name, const Vector<Node>& args) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    auto it = localFunctions.find(name);
    Vector<NodeValueType> argTypes;

    if (it != localFunctions.end()) {
        DEBUG_LOG(LogLevel::TRACE, "FOUND FUNCTION In lookupFunction, args not checked yet");
        for (const auto &arg : args) {
            argTypes.push_back(arg.getType());
            DEBUG_LOG(LogLevel::TRACE, "ArgType: ", nodeTypeToString(arg.getType()));
        }

        for (auto candidate : it->second) {
            // DEBUG_LOG(LogLevel::ERROR, "Checking Function Candidate", name, candidate->getCallable()->parameters.toString());
            // DEBUG_LOG(LogLevel::ERROR, "Function Sub-Type: ", callableTypeAsString(candidate->getSubType()));
            if (candidate->getSubType() == CallableType::DEF) {
                if (candidate->matches(argTypes)){
                    // return std::optional<SharedPtr<CallableSignature>>(candidate);
                    return candidate;
                } else {
                    // DEBUG_LOG(LogLevel::ERROR, "This is actually an Argument mismatch error");
                    throw FunctionNotFoundError(name);
                }
            } 

            else if (candidate->getSubType() == CallableType::FUNCTION) {
                if (candidate->matches(argTypes)){
                    // DEBUG_LOG(LogLevel::ERROR, "FOUND AND RETURNING MATCHING FUNCTION");
                    DEBUG_FLOW_EXIT();
                    return candidate;
                } else {
                    DEBUG_LOG(LogLevel::ERROR, "This is actually an Argument mismatch error");
                    throw FunctionNotFoundError(name);
                }
                // DEBUG_LOG(LogLevel::DEBUG, highlight("Function:", Colors::bold_blue), name, "args didn't match");
            }
        }
    }

    if (auto parent = parentScope.lock()){
        return parent->lookupFunction(name, args);
    }


    return nullptr;
}

SharedPtr<CallableSignature> Scope::getFunction(const String& name, const Vector<Node>& args) {
    DEBUG_FLOW(FlowLevel::MED);
    if (auto func = lookupFunction(name, args)) {
        return func.value();
    }

    if (auto parent = parentScope.lock()) {
        // DEBUG_FLOW_EXIT();
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
    if (auto functions = globalFunctions->getFunction(name)){
        // return functions;
        return functions.value();
    }

    if (auto parent = parentScope.lock()){
        // DEBUG_FLOW_EXIT();
        return parent->getFunction(name);
    }
    throw FunctionNotFoundError(name);
    // return functionRegistry.getFunction(name);
}





std::optional<SharedPtr<ClassSignature>> Scope::lookupClass(const String& name) const {
    // 1) check the local overlay
    auto it = localClasses.find(name);
    if (it != localClasses.end())
      return it->second;
  
    // 2) recurse up the lexical parent chain
    if (auto p = parentScope.lock()) {
      if (auto found = p->lookupClass(name))
        return found;
    }
  
    // 3) finally fall back to the one global registry
  //   return globalClasses->getClass(name);
      auto cls = globalClasses->getClass(name);
      if (cls) {
          return cls.value();
      }
      return nullptr;
  //   throw FunctionNotFoundError(name);
  }


// void Scope::registerClass(const String& name, SharedPtr<ClassBase> classBase) {
//     DEBUG_FLOW(FlowLevel::LOW);
//     if (auto cls = lookupClass(name)) {
//         if (cls){
//             DEBUG_LOG(LogLevel::ERROR, callableTypeAsString(cls.value()->getCallableType()), "is already defined");
//             DEBUG_LOG(LogLevel::ERROR, "SharedPtr<ClassBase>");
//             throw MerkError("Class '" + name + "' is already defined in this scope.");

//         }


//     }
//     // classRegistry.registerClass(name, classBase);

//     localClasses[name] = std::move(std::static_pointer_cast<ClassSignature>(classBase->toCallableSignature()));
//     DEBUG_FLOW_EXIT();
// }


// void Scope::registerClass(const String& name, SharedPtr<ClassSignature> classSignature) {
//     DEBUG_FLOW(FlowLevel::LOW);
//     if (auto cls = lookupClass(name)) {
//         // DEBUG_LOG(LogLevel::ERROR, callableTypeAsString(cls->get()->getCallableType()), "is already defined");
//         DEBUG_LOG(LogLevel::ERROR, "SharedPtr<ClassSignature>");
//         throw MerkError("Class '" + name + "' is already defined in this scope.");
//     }
//     // classRegistry.registerClass(name, classSignature);
//     localClasses[name] = std::move(classSignature);
//     DEBUG_FLOW_EXIT();
// }


// void Scope::registerClass(const String& name, SharedPtr<ClassBase> classBase) {
//     DEBUG_FLOW(FlowLevel::LOW);
//     auto clsOpt = lookupClass(name);
//     if (clsOpt && clsOpt.value()) {
//         auto& cls = clsOpt.value();
//         DEBUG_LOG(LogLevel::ERROR, callableTypeAsString(cls->getCallableType()), "Class: ", name, "is already defined");
//         throw MerkError("Class '" + name + "' is already defined in this scope.");
//     }
//     // classRegistry.registerClass(name, classBase);

//     localClasses[name] = std::move(std::static_pointer_cast<ClassSignature>(classBase->toCallableSignature()));
//     DEBUG_FLOW_EXIT();
// }

void Scope::registerClass(const String& name, SharedPtr<ClassBase> classBase) {
    DEBUG_FLOW(FlowLevel::LOW);

    if (!classBase) {
        throw MerkError("registerClass called with null ClassBase for: " + name);
    }

    if (auto clsOpt = lookupClass(name); clsOpt && clsOpt.value()) {
        throw MerkError("Class '" + name + "' is already defined in this scope.");
    }
    auto sig = std::static_pointer_cast<ClassSignature>(classBase->toCallableSignature());

    if (!sig) {
        throw MerkError("ClassSignature is null when registering: " + name);
    }

    

    localClasses[name] = std::move(sig);
    DEBUG_FLOW_EXIT();
}






std::optional<SharedPtr<ClassSignature>> Scope::getClass(const String& name) {

    if (auto className = lookupClass(name)){
        return className;
    }
    if (auto className = globalClasses->getClass(name)){
        return className;
        // if (className.has_value()){
        //     // auto callSig = className.value().get();
        //     // auto classSig = static_cast<ClassSignature>(callSig);
        //     return className;
        // }
    }
    if (globalClasses->hasClass(name)) {
        return globalClasses->getClass(name);
    }
    auto parent = parentScope.lock();
    if (parent) {
        return parent->getClass(name);
    }
    throw MerkError("Class: " + name + " Does Not Exist.");
}


bool Scope::hasClass(const String& name) const {

    // if (auto func = lookupFunction(name)){
    //     DEBUG_LOG(LogLevel::TRACE, "FOUND FUNCTION THROUGH Lookup");
    //     return true;

    // }
    // if (auto parent = getParent()){
    //     DEBUG_LOG(LogLevel::TRACE, "FOUND FUNCTION THROUGH Parent Lookup");

    //     return getParent()->hasFunction(name);
    // }

    // return globalFunctions->hasFunction(name);
    if (auto cls = lookupClass(name)){
        return true;
    }

    if (auto parent = getParent()){
        return parent->hasClass(name);
    }

    return globalClasses->hasClass(name);
    // auto it = localClasses.find(name);
    // if (it != localClasses.end()){
    //     return true;
    // }
    // if (auto p = parentScope.lock()){
    //     if (auto r = p->lookupFunction(name)) {
    //         return true;
    //     }       
    // }
    // else {
    //     return (parentScope.lock() && parentScope.lock()->hasClass(name)) || globalClasses->hasClass(name);

    // }

    // return false;
}

SharedPtr<ClassRegistry> Scope::getClassRegistry() {return globalClasses;}


// ClassRegistry& Scope::getClassRegistry() {return classRegistry;}





// Create a child scope
// SharedPtr<Scope> Scope::createChildScope() {
//     DEBUG_FLOW(FlowLevel::MED);

//     auto childScope = makeShared<Scope>(shared_from_this(), interpretMode);
//     DEBUG_LOG(LogLevel::TRACE, 
//         "Creating child scope with level:", childScope->getScopeLevel(), 
//         "| Parent scope level: ", scopeLevel);
//     if (shared_from_this().use_count() == 0) {
//         throw MerkError("Invalid use of shared_from_this(): Scope not managed by shared_ptr.");
//     }
//     includeMetaData(childScope, isDetached);
//     childScopes.push_back(childScope); // Track child scopes

//     DEBUG_LOG(LogLevel::TRACE, "Created child scope:", childScope.get(), "| Parent:", this);

//     // Additional Debugging
//     DEBUG_LOG(LogLevel::TRACE, "ChildScope created with scopeLevel: ", childScope->getScopeLevel(), 
//             " | Memory Loc: ", childScope.get(),
//             " | Parent Loc: ", this,
//             " | Total Children: ", childScopes.size());

//     DEBUG_LOG(LogLevel::TRACE, "DEBUG Scope::createChildScope: Created child scope. Child Scope Level: ", 
//             childScope->getScopeLevel(), " | Child Scope Loc: ", childScope.get(), 
//             " | Parent Scope Loc: ", this);
//     DEBUG_FLOW_EXIT();
//     return childScope;

    
// }


// SharedPtr<Scope> Scope::makeCallScope() {
//     auto s = makeShared<Scope>( this->shared_from_this(), interpretMode );
//     includeMetaData(s, /*detached=*/false);
//     s->scopeLevel = this->scopeLevel + 1;
//     return s;   // ← crucially: do *not* push it into childScopes
//   }


// Node Scope::lookup(const String& name, IdentifierType type) {
//     DEBUG_FLOW(FlowLevel::MED);
//     DEBUG_LOG(LogLevel::TRACE, "Scope::lookup => Looking for '", name, "' of type ", identifierTypeToString(type));

//     switch (type) {
//         case IdentifierType::Variable: {
//             if (context.hasVariable(name)) {
//                 VarNode& var = context.getVariable(name)->get();
//                 DEBUG_LOG(LogLevel::TRACE, "Found variable in current scope: ", name);
//                 return var;
//             }
//             break;
//         }

//         case IdentifierType::Function: {
//             auto funcOpt = functionRegistry.getFunction(name);
//             if (funcOpt.has_value()) {
//                 DEBUG_LOG(LogLevel::TRACE, "Found function in current scope: ", name);
//                 return FunctionNode(funcOpt.value().get().getCallable());
//             }
//             break;
//         }

//         case IdentifierType::Method: {
//             // No MethodRegistry, methods treated as functions inside class scopes.
//             auto funcOpt = functionRegistry.getFunction(name);
//             if (funcOpt.has_value()) {
//                 DEBUG_LOG(LogLevel::TRACE, "Found method (as function) in current scope: ", name);
//                 return MethodNode(funcOpt.value().get().getCallable());
//             }
//             break;
//         }

//         case IdentifierType::Class: {
//             auto classOpt = classRegistry.getClass(name);
//             if (classOpt.has_value()) {
//                 DEBUG_LOG(LogLevel::TRACE, "Found class in current scope: ", name);
//                 return ClassNode(std::dynamic_pointer_cast<ClassBase>(classOpt.value().get()->getCallable()));
//             }
//             break;
//         }

//         default:
//             throw MerkError("Unsupported IdentifierType in Scope::lookup for: " + name);
//     }

//     // If not found, try parent
//     if (auto parent = parentScope.lock()) {
//         DEBUG_LOG(LogLevel::TRACE, "Identifier '", name, "' not found in current scope. Checking parent...");
//         return parent->lookup(name, type);
//     }

//     // Throw proper error
//     throw MerkError("Identifier '" + name + "' of type '" + identifierTypeToString(type) + "' not found in any scope.");
// }
