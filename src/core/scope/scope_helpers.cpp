#include <iostream>
#include <unordered_set>

#include "core/types.h"
#include "core/node.h"

#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"

#include "core/functions/param_node.h"
#include "core/context.h"
#include "core/functions/function_node.h"
#include "core/registry/class_registry.h"
#include "core/registry/function_registry.h"

#include "core/functions/native_functions.h"
#include "core/errors.h"
#include "core/scope.h"
 



// this was the original idea for function scoping, but a better one was made, namely detaching, filling, then reattaching the scope using appendChildScope
// The idea may still be useful in the future so keeping the idea and its implementation in-tact
// SharedPtr<Scope> Scope::mergeScope(SharedPtr<Scope> definingScope) {  
//     // DEBUG_FLOW(FlowLevel::VERY_LOW);
//     (void)definingScope;

//     SharedPtr<Scope> callScope = std::make_shared<Scope>(0, interpretMode);
//     // DEBUG_LOG(LogLevel::TRACE, "******************************************************************************************");
//     // DEBUG_LOG(LogLevel::TRACE, "[Scope::mergeScope] Variables: Creating a temporary function call scope.");
//     getAllVariables(context);
//     // context.debugPrint();
    
//     // DEBUG_LOG(LogLevel::TRACE, "[Scope::mergeScope] Functions: Creating a temporary function call scope.");
//     // Step 3: Copy functions from definingScope **only if they aren't already updated in the current scope**
//     getAllFunctions(functionRegistry);


//     // DEBUG_LOG(LogLevel::TRACE, "Scope for mergedScope in Scope::mergeScope");
//     // callScope->debugPrint();

//     // DEBUG_LOG(LogLevel::TRACE, "FunctionRegistry for mergedScope in Scope::mergeScope");
//     // functionRegistry.debugPrint();

//     // DEBUG_LOG(LogLevel::TRACE, "[Scope::mergeScope] Function call scope created successfully.");
//     // callScope->parentScope = nullptr;
//     // DEBUG_LOG(LogLevel::TRACE, "******************************************************************************************");
//     // DEBUG_FLOW_EXIT();
//     return callScope;
// }

UniquePtr<VarNode> cloneVarNode(VarNode* original) {
    return UniquePtr<VarNode>(original);
}

// In Scope class
SharedPtr<Scope> Scope::detachScope(const std::unordered_set<String>& freeVarNames) {
    DEBUG_FLOW(FlowLevel::MED);
    // Create a new scope that is not attached where: (no parent, or parent = nullptr).
    auto detached = std::make_shared<Scope>(0, this->interpretMode);
    detached->isDetached = true;
    includeMetaData(detached, true);
    // For each free variable name, if it exists in this scope, or parent, get it and add to the vector
    for (const auto& name : freeVarNames) {
        if (this->hasVariable(name)) {
            VarNode original = getVariable(name);
            detached->declareVariable(name, cloneVarNode(original.clone()));
        }
        else if (auto parent = this->getParent()) {
            // If not in the current scope, attempt to find it in the parents.
            if (parent->hasVariable(name)) {
                VarNode originalVar = parent->getVariable(name);
                detached->declareVariable(name, cloneVarNode(originalVar.clone()));
            }
        }
    }

    for (const auto& [fname, signatures] : this->functionRegistry.getFunctions()) {
        for (const auto& sig : signatures) {
            detached->getFunctionRegistry().registerFunction(fname, sig);
        }
    }

    // DEBUG_LOG(LogLevel::INFO, highlight("[Context Variables From Detached Scope]:", Colors::yellow));

    DEBUG_FLOW_EXIT();
    return detached;
}

void Scope::includeMetaData(SharedPtr<Scope> newScope, bool thisIsDetached) const {
    newScope->isDetached = thisIsDetached;
    newScope->isClonedScope = newScope->isClonedScope ? newScope->isClonedScope : isClonedScope;
    newScope->isCallableScope = newScope->isCallableScope ? newScope->isCallableScope : isCallableScope;
    newScope->owner = newScope->isDetached ? "detached" : owner;
}


SharedPtr<Scope> Scope::clone() const {
    DEBUG_FLOW(FlowLevel::MED);
    auto newScope = std::make_shared<Scope>(this->scopeLevel, this->interpretMode);
    
    // Deep copy the variables from this scope.
    for (const auto& [name, var] : this->context.getVariables()) {
        newScope->context.setVariable(name, UniquePtr<VarNode>(var->clone()));
    }
    
    newScope->functionRegistry = this->functionRegistry.clone();
    newScope->isClonedScope = true;
    includeMetaData(newScope, isDetached);
    DEBUG_FLOW_EXIT();
    
    return newScope;
}


// Exit the current scope (optional logic if cleanup is needed)
// void Scope::exitScope() {
//     // DEBUG_FLOW(FlowLevel::LOW);
//     // DEBUG_LOG(LogLevel::TRACE, "Exiting scope at level: ", scopeLevel, " with ", context.getVariables().size(), " variables present.");

//     // if (auto parent = parentScope.lock()) {
//     //     std::cout << "Returning to parent scope at level: " << parent->getScopeLevel() << "\n";
//     // } else {
//     //     std::cout << "No parent scope exists; exiting root scope.\n";
//     // }
//     // DEBUG_FLOW_EXIT();
// }

Vector<SharedPtr<Scope>> Scope::getChildren() {
    return childScopes;
}

bool Scope::hasChildren(){
    return childScopes.size() > 0;
}

bool Scope::parentIsValid() {
    auto parent = parentScope.lock();
    if (parent){
        return true;
    }

    return false;

}

// Debugging
void Scope::debugPrint() const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    if (scopeLevel == 0){
        debugLog(true, highlight("\n\n======================== Start Scope::debugPrint ======================== ", Colors::cyan));
    }

    debugLog(true, 
        "Scope Level: ", scopeLevel, 
        "isDetached:", isDetached ? "true" : "false", 
        "isCloned:", isClonedScope ? "true" : "false", 
        "isCallableScope:", isCallableScope ? "true" : "false",
        "Owner:", !owner.empty() ? owner : "None" 
    );
    context.debugPrint();
    functionRegistry.debugPrint();
    
    for (const auto& child : childScopes) {
        child->debugPrint();
    }

    if (scopeLevel == 0){
        debugLog(true, highlight("======================== End Scope::debugPrint ======================== \n", Colors::cyan));
    }

    DEBUG_FLOW_EXIT();
}

// Recursively print all child scopes and their critical information
void Scope::printChildScopes(int indentLevel) const {
    // Helper to create indentation for nested scopes
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    if (scopeLevel == 0){
        debugLog(true, highlight("\n\n======================== Start Scope::printChildScopes ========================", Colors::cyan));
    }
    auto indent = String(indentLevel * 2, ' ');

    // Print information about the current scope
    debugLog(true, indent,
        "Scope Level:", scopeLevel, 
        "| Memory Loc:", this, 
        "| Parent Loc:", parentScope.lock() ? parentScope.lock() : nullptr,
        "| Number of Variabls:", context.getVariables().size(),
        "| Number of Children:", childScopes.size(),
        "| Num Functions:", functionRegistry.getFunctions().size(),
        "| Num Classes:", classRegistry.getClasses().size(),
        "| Owner:", !owner.empty() ? owner : "None"
        );

    // Recursively print each child scope
    for (const auto& child : childScopes) {
        child->printChildScopes(indentLevel + 1); // Increase indentation for child
    }
    if (scopeLevel == 0){
        debugLog(true, highlight("======================== End Scope::printChildScopes ========================\n", Colors::cyan));
    }

    DEBUG_FLOW_EXIT();
}

void Scope::printContext(int depth) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    // Print current scope details
    String indent(depth * 2, ' '); // Indentation based on depth
    std::cout << indent << "Scope Level: " << scopeLevel
              << " | Memory Loc: " << this
              << " | Parent Loc: " << (parentScope.lock() ? parentScope.lock().get() : nullptr)
              << " | Number of Variables: " << context.getVariables().size()
              << " | Number of Children: " << childScopes.size() << std::endl;

    // Print variables in the current context
    for (const auto& [name, value] : context.getVariables()) {
        if (value) {
            std::cout << indent << "  Variable: " << name
                      << " = " << *value << std::endl;  // Dereference unique_ptr
        } else {
            std::cout << indent << "  Variable: " << name << " = [null]" << std::endl;
        }
    }

    // Recursively print the parent scope, if any
    if (auto parent = parentScope.lock()) {
        parent->printContext(depth + 1);
    }
    DEBUG_FLOW_EXIT();
}


void Scope::registerCallableType(const String& name, CallableType type) {
    (*globalCallables)[name] = type;
}

std::optional<CallableType> Scope::getCallableType(const String& name) const {
    auto it = globalCallables->find(name);
    if (it != globalCallables->end()) {
        return it->second;
    }
    return std::nullopt;
}

void Scope::setScopeLevel(int newLevel) {
    scopeLevel = newLevel;
}
void Scope::updateChildLevelsRecursively() {
    for (auto& child : childScopes){
        child->setScopeLevel(scopeLevel+1);
        child->updateChildLevelsRecursively();
    }
}

Node Scope::resolveCallable(const String& name, const Vector<Node>& args) {
    auto typeOpt = getCallableType(name);
    if (!typeOpt.has_value()) {
        throw MerkError("Callable '" + name + "' not found in global callables.");
    }

    CallableType kind = *typeOpt;

    switch (kind) {
        case CallableType::FUNCTION:
        case CallableType::DEF:
        case CallableType::LAMBDA:
        case CallableType::NATIVE: {
            auto funcOpt = getFunction(name, args);
            if (!funcOpt) throw FunctionNotFoundError(name);
            return FunctionNode(funcOpt->get().getCallable());
        }

        case CallableType::CLASS: {
            auto classOpt = getClass(name);
            if (!classOpt.has_value()) {
                throw MerkError("Class '" + name + "' not found.");
            }
            return ClassNode(std::dynamic_pointer_cast<ClassBase>(classOpt->get()->getCallable()));
        }

        case CallableType::METHOD:
        case CallableType::INSTANCE:
        case CallableType::CALLABLE:
            throw MerkError("Cannot resolve callable of unsupported type directly: " + name);
    }

    throw MerkError("Unrecognized callable type for '" + name + "'.");
}



bool Scope::removeChildScope(const SharedPtr<Scope>& target) {
    DEBUG_FLOW(FlowLevel::LOW);
    auto it = std::find(childScopes.begin(), childScopes.end(), target);
    if (it != childScopes.end()) {
        childScopes.erase(it);
        DEBUG_LOG(LogLevel::DEBUG, "Removed child scope at memory location: ", target.get());
        return true;
    }
    DEBUG_LOG(LogLevel::DEBUG, "Failed to remove child scope: not found.");
    return false;
}