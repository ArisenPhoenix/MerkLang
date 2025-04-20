#include <iostream>
#include <unordered_set>

#include "core/types.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"

#include "core/node.h"

#include "core/functions/param_node.h"

#include "core/context.h"
#include "core/registry/class_registry.h"
#include "core/registry/function_registry.h"

// #include "core/functions/native_functions.h"
#include "core/errors.h"
#include "core/functions/function_node.h"
#include "core/classes/method.h"
#include "core/scope.h"


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
Scope::Scope(int scopeLevel, bool interpretMode) 
    : scopeLevel(scopeLevel), interpretMode(interpretMode) {
        DEBUG_FLOW(FlowLevel::LOW);
    // DEBUG_LOG(LogLevel::INFO, "Initialized Scope with level: ", scopeLevel, " | Memory Loc: ", this, " | Parent Loc: ", parentScope.lock().get());
    // std::cout << "Attempting to allocate globalCallables...\n";

    globalCallables = std::make_shared<std::unordered_map<String, CallableType>>();
    // std::cout << "Success\n";

    DEBUG_LOG(LogLevel::TRACE, 
             "Initialized Root Scope with level: ", scopeLevel, 
             " | Memory Loc: ", this, 
             " | Parent Loc: ", formatPointer(nullptr));
    DEBUG_FLOW_EXIT();
}

// Constructor for child
Scope::Scope(std::weak_ptr<Scope> parentScope, bool interpretMode)
    : parentScope(std::move(parentScope)), interpretMode(interpretMode) {
        DEBUG_FLOW(FlowLevel::LOW);
    // Lock the parent scope to determine the current scope level
    if (auto parent = this->parentScope.lock()) {
        DEBUG_LOG(LogLevel::TRACE, "Parent scope locked at level ", parent->scopeLevel);

        scopeLevel = parent->scopeLevel + 1; // Correctly increment scope level
        DEBUG_LOG(LogLevel::TRACE, "Child Scope Initialized with Parent Scope Level: ", parent->scopeLevel,
                 " | Parent Memory Loc: ", parent.get(), 
                 " | Child Memory Loc: ", this);
        globalCallables = parent->globalCallables;  // Share the same registry!
        DEBUG_LOG(LogLevel::TRACE, "Inherited globalCallables from parent.");


    } else {
        // If no parent is found, this becomes the root scope
        DEBUG_LOG(LogLevel::TRACE, "Parent scope locked at level ", parent->scopeLevel);

        scopeLevel = 0; 
        DEBUG_LOG(LogLevel::TRACE, "Root Scope Initialized | Memory Loc: ", this);
        globalCallables = std::make_shared<std::unordered_map<String, CallableType>>(); // Fallback
        DEBUG_LOG(LogLevel::TRACE, "Inherited globalCallables from parent.");

    }

    // Log after ensuring all members are initialized
    DEBUG_LOG(LogLevel::TRACE, 
            "Finalized Scope Initialization with level: ", scopeLevel, 
            " | Memory Loc: ", static_cast<void*>(this), 
            " | Parent Loc: ", formatPointer(this->parentScope.lock().get()));
    
    DEBUG_FLOW_EXIT();
}

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

        // 1. Explicitly destroy stored variables
        // DEBUG_LOG(LogLevel::INFO, "Still Need to Handle Cleaning up variables in Context Scope Level: ", scopeLevel);

        // 2. Explicitly destroy stored functions
        // DEBUG_LOG(LogLevel::INFO, "Still Need To Handle Cleaning up functions in Registry in Scope Level: ", scopeLevel);

        // Explicitly destroy child scopes in reverse order
        while (!childScopes.empty()) {
            auto child = childScopes.back(); // Get the last child
            DEBUG_LOG(LogLevel::TRACE, "Recursively Destroying Child Scope at Level: ", child->getScopeLevel(),
                 " | Memory Loc: ", child.get());
            child.reset(); // Explicitly reset the shared pointer
            childScopes.pop_back(); // Remove the last child
        }

        childScopes.clear();

        
    DEBUG_LOG(LogLevel::TRACE, "Exiting Scope Destructor for Scope Level: ", scopeLevel, 
             " | Memory Loc: ", this);
    }
    DEBUG_FLOW_EXIT();
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
    if (!functionRegistry.getFunctions().empty()){
        for (const auto& [funcName, funcVec] : functionRegistry.getFunctions()) {
            bool functionAlreadyExists = this->getFunctionRegistry().hasFunction(funcName);

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
    setParent(instanceScope);
    scopeLevel = instanceScope->getScopeLevel() + 1;
    // Optionally append this to the instanceScope's children
    instanceScope->appendChildScope(shared_from_this());
}


// Create a child scope
SharedPtr<Scope> Scope::createChildScope() {
    DEBUG_FLOW(FlowLevel::MED);

    auto childScope = std::make_shared<Scope>(shared_from_this(), interpretMode);
    DEBUG_LOG(LogLevel::TRACE, 
        "Creating child scope with level:", childScope->getScopeLevel(), 
        "| Parent scope level: ", scopeLevel);

    includeMetaData(childScope, isDetached);
    childScopes.push_back(childScope); // Track child scopes

    DEBUG_LOG(LogLevel::TRACE, "Created child scope:", childScope.get(), "| Parent:", this);

    // Additional Debugging
    DEBUG_LOG(LogLevel::TRACE, "ChildScope created with scopeLevel: ", childScope->getScopeLevel(), 
            " | Memory Loc: ", childScope.get(),
            " | Parent Loc: ", this,
            " | Total Children: ", childScopes.size());

    DEBUG_LOG(LogLevel::TRACE, "DEBUG Scope::createChildScope: Created child scope. Child Scope Level: ", 
            childScope->getScopeLevel(), " | Child Scope Loc: ", childScope.get(), 
            " | Parent Scope Loc: ", this);
    DEBUG_FLOW_EXIT();
    return childScope;

    
}

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
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    DEBUG_FLOW_EXIT();
    return scopeLevel;
    
}


const FunctionRegistry& Scope::getFunctionRegistry() const { return functionRegistry; }
FunctionRegistry& Scope::getFunctionRegistry() { return functionRegistry; }


bool Scope::hasFunction(const String& name) const {
    return functionRegistry.hasFunction(name);
}

void Scope::registerFunction(const String& name, SharedPtr<UserFunction> function) {
    DEBUG_FLOW(FlowLevel::MED);
    // auto signature = function->toFunctionSignature();
    auto signature = function->toCallableSignature();

    functionRegistry.registerFunction(name, std::move(signature));
    DEBUG_FLOW_EXIT();
}

void Scope::registerFunction(const String& name, SharedPtr<CallableSignature> signature) {
    DEBUG_FLOW(FlowLevel::MED);

    functionRegistry.registerFunction(name, signature);
    DEBUG_FLOW_EXIT();
}


std::optional<std::reference_wrapper<CallableSignature>> Scope::getFunction(const String& name, const Vector<Node>& args) {
    // SharedPtr<FunctionSignature> Scope::getFunction(const String& name, const Vector<Node>& args) {
    // DEBUG_FLOW(FlowLevel::MED);
    if (auto function = functionRegistry.getFunction(name, args)){
        if (function){
            // DEBUG_FLOW_EXIT();
            return function->get();
        }
    }

    if (auto parent = parentScope.lock()){
        // DEBUG_FLOW_EXIT();
        return parent->getFunction(name, args);
    }

    // DEBUG_FLOW_EXIT();
    // debugPrint();
    throw FunctionNotFoundError(name);
}

// For Function Reference
std::optional<std::reference_wrapper<CallableSignature>> Scope::getFunction(const String& name) {
    return functionRegistry.getFunction(name);
}







void Scope::registerClass(const String& name, SharedPtr<ClassBase> classBase) {
    DEBUG_FLOW(FlowLevel::LOW);
    if (classRegistry.hasClass(name)) {
        throw MerkError("Class '" + name + "' is already defined in this scope.");
    }
    classRegistry.registerClass(name, classBase);
    DEBUG_FLOW_EXIT();
}


void Scope::registerClass(const String& name, SharedPtr<ClassSignature> classSignature) {
    DEBUG_FLOW(FlowLevel::LOW);
    if (classRegistry.hasClass(name)) {
        throw MerkError("Class '" + name + "' is already defined in this scope.");
    }
    classRegistry.registerClass(name, classSignature);
    DEBUG_FLOW_EXIT();
}

void Scope::setParent(SharedPtr<Scope> scope) {
    scope->appendChildScope(shared_from_this());
    parentScope = scope;
    scopeLevel = scope->getScopeLevel() + 1;
}




std::optional<std::reference_wrapper<SharedPtr<CallableSignature>>> Scope::getClass(const String& name) {
    if (auto className = classRegistry.getClass(name)){
        if (className.has_value()){
            // auto callSig = className.value().get();
            // auto classSig = static_cast<ClassSignature>(callSig);
            return className.value().get();
        }
    }
    if (classRegistry.hasClass(name)) {
        return classRegistry.getClass(name);
    }
    auto parent = parentScope.lock();
    if (parent) {
        return parent->getClass(name);
    }
    throw MerkError("Class: " + name + " Does Not Exist.");
}


bool Scope::hasClass(const String& name) const {
    return classRegistry.hasClass(name) || (parentScope.lock() && parentScope.lock()->hasClass(name));
}


ClassRegistry& Scope::getClassRegistry() {return classRegistry;}


Node Scope::lookup(const String& name, IdentifierType type) {
    DEBUG_FLOW(FlowLevel::MED);
    DEBUG_LOG(LogLevel::TRACE, "Scope::lookup => Looking for '", name, "' of type ", identifierTypeToString(type));

    switch (type) {
        case IdentifierType::Variable: {
            if (context.hasVariable(name)) {
                VarNode& var = context.getVariable(name)->get();
                DEBUG_LOG(LogLevel::TRACE, "Found variable in current scope: ", name);
                return var;
            }
            break;
        }

        case IdentifierType::Function: {
            auto funcOpt = functionRegistry.getFunction(name);
            if (funcOpt.has_value()) {
                DEBUG_LOG(LogLevel::TRACE, "Found function in current scope: ", name);
                return FunctionNode(funcOpt.value().get().getCallable());
            }
            break;
        }

        case IdentifierType::Method: {
            // No MethodRegistry, methods treated as functions inside class scopes.
            auto funcOpt = functionRegistry.getFunction(name);
            if (funcOpt.has_value()) {
                DEBUG_LOG(LogLevel::TRACE, "Found method (as function) in current scope: ", name);
                return MethodNode(funcOpt.value().get().getCallable());
            }
            break;
        }

        case IdentifierType::Class: {
            auto classOpt = classRegistry.getClass(name);
            if (classOpt.has_value()) {
                DEBUG_LOG(LogLevel::TRACE, "Found class in current scope: ", name);
                return ClassNode(std::dynamic_pointer_cast<ClassBase>(classOpt.value().get()->getCallable()));
            }
            break;
        }

        default:
            throw MerkError("Unsupported IdentifierType in Scope::lookup for: " + name);
    }

    // If not found, try parent
    if (auto parent = parentScope.lock()) {
        DEBUG_LOG(LogLevel::TRACE, "Identifier '", name, "' not found in current scope. Checking parent...");
        return parent->lookup(name, type);
    }

    // Throw proper error
    throw MerkError("Identifier '" + name + "' of type '" + identifierTypeToString(type) + "' not found in any scope.");
}
