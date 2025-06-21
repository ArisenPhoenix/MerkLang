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
 

UniquePtr<VarNode> cloneVarNode(VarNode* original) {
    return UniquePtr<VarNode>(original);
}

// In Scope class
SharedPtr<Scope> Scope::detachScope(const std::unordered_set<String>& freeVarNames) {
    DEBUG_FLOW(FlowLevel::MED);
    // Create a new scope that is not attached where: (no parent, or parent = nullptr).
    // explicit Scope(SharedPtr<Scope> parentScope, SharedPtr<FunctionRegistry> globalF, SharedPtr<ClassRegistry> globalC, bool interpreMode);

    auto detached = std::make_shared<Scope>(shared_from_this(), globalFunctions, globalClasses, interpretMode);
    // detached->setParent(nullptr);
    // detached->setScopeLevel(0);
    // Scope()
    detached->isDetached = true;
    detached->owner = owner+"(detached)";
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

    // DEBUG_LOG(LogLevel::INFO, highlight("[Context Variables From Detached Scope]:", Colors::yellow));

    DEBUG_FLOW_EXIT();
    return detached;
}


SharedPtr<Scope> Scope::isolateScope(const std::unordered_set<String>& freeVarNames) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    auto isolated = std::make_shared<Scope>(0, interpretMode, false);
    includeMetaData(isolated, true);

    for (const auto& name : freeVarNames) {
        if (this->hasVariable(name)) {
            isolated->declareVariable(name, cloneVarNode(getVariable(name).clone()));
        } else if (auto parent = getParent()) {
            if (parent->hasVariable(name)) {
                isolated->declareVariable(name, cloneVarNode(parent->getVariable(name).clone()));
            }
        }
    }

    isolated->localFunctions = this->localFunctions;
    isolated->localClasses = this->localClasses;
    DEBUG_FLOW_EXIT();
    return isolated;
}


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
    // newScope->owner = newScope->isDetached ? newScope->owner.empty() ? owner + "(detached)" : owner : newScope->owner == "None" ? owner : owner;
}


// SharedPtr<Scope> Scope::clone(bool strict) const {
//     DEBUG_FLOW(FlowLevel::MED);
//     if (parentScope.expired()){
//         if (strict) {
//             throw ParentScopeNotFoundError();
//         }
//     }
//     auto newScope = std::make_shared<Scope>(getParent(), this->interpretMode);
    
//     // Deep copy the variables from this scope.
//     for (const auto& [name, var] : this->context.getVariables()) {
//         newScope->context.setVariable(name, UniquePtr<VarNode>(var->clone()));
//     }
    
//     newScope->functionRegistry = this->functionRegistry.clone();
//     newScope->classRegistry = this->classRegistry.clone();
//     newScope->isClonedScope = true;
//     includeMetaData(newScope, isDetached);
//     DEBUG_FLOW_EXIT();
    
//     return newScope;
// }

SharedPtr<Scope> Scope::clone(bool strict) const {
    DEBUG_FLOW(FlowLevel::MED);

    SharedPtr<Scope> newScope;
    if (auto parent = parentScope.lock()) {
        // use the weak/child constructor
        // Scope(parent, globalFunctions, globalClasses, interpretMode);
        newScope = std::make_shared<Scope>(parent, globalFunctions, globalClasses, interpretMode);
    } else {
        if (strict){
            throw ParentScopeNotFoundError();
        }
        // it really was a root, so use the int‐ctor
        newScope = std::make_shared<Scope>(0, interpretMode);
    }

    // now deep‐copy your context, registries, metadata…
    for (const auto& [name,var] : this->context.getVariables())
        newScope->context.setVariable(name, UniquePtr<VarNode>(var->clone()));

    // for (const auto& [funcName, funcVect] : localFunctions) {
    //     for (const auto& funcSig : funcVect) {
    //         if (funcSig){
    //             newScope->registerFunction(funcName, funcSig);
    //         }
    //         else {
    //             throw MerkError("Clone Failed at Function: " + funcName);
    //         }
    //     }
    // }

    // for (auto& [className, classSig] : localClasses) {

    //     if (classSig){
    //         newScope->registerClass(className, classSig);
    //     }
    //     else {
    //         throw MerkError("Clone Failed at Function: " + className);
    //     }

    // }

    newScope->localFunctions = this->localFunctions;
    newScope->localClasses = this->localClasses;
    newScope->isClonedScope    = true;
    includeMetaData(newScope, isDetached);

    DEBUG_FLOW_EXIT();
    return newScope;
}



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
        "Owner:", !owner.empty() ? owner : "<None Provided>>" 
    );
    context.debugPrint();
    // globalFunctions->debugPrint();
    for (auto& funcVec : localFunctions){
        auto& funcName = funcVec.first;
        debugLog(true, funcName, funcVec);
    }
    
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
    auto parent = parentScope.lock();
    // auto newIndent = indentLevel * 2;
    auto indent = String(indentLevel+2, ' ');
    int numInstances = 0;
    for (auto& [varName, var] : context.getVariables()) {
        if (var->isClassInstance()) {
            numInstances += 1;
        }
    }

    // Print information about the current scope
    debugLog(true, indent,
        "Scope Level:", scopeLevel, 
        "| Memory Loc:", this, 
        "| Parent Loc:", parentScope.lock() ? parentScope.lock() : nullptr,
        "| Number of Variabls:", context.getVariables().size() - numInstances,
        "| Number of Children:", childScopes.size(),
        "| Num Functions:", localFunctions.size(),
        "| Num Classes:", localClasses.size(),
        "| Num Instance: ", numInstances,
        "| Owner:", !owner.empty() ? owner : "<None Provided>>"
        );
    
    if (numInstances > 0) {
        debugLog(true, indent, "=================== GO INSTANCE LOG ===================");
        for (auto& [varName, var] : context.getVariables()) {
            if (var->isClassInstance()) {
                auto instance = std::get<SharedPtr<ClassInstance>>(var->getValue());
                // auto instance = std::static_pointer_cast<ClassInstance>(callable);
                instance->getInstanceScope()->printChildScopes(indentLevel+2);
                instance->getCapturedScope()->printChildScopes(indentLevel+2);
            }
        }
        debugLog(true, indent, "=================== END INSTANCE LOG ===================");

    }
    // Recursively print each child scope
    for (const auto& child : childScopes) {
        child->printChildScopes(indentLevel+2); // Increase indentation for child
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





void Scope::setScopeLevel(int newLevel) {
    scopeLevel = newLevel;
}
void Scope::updateChildLevelsRecursively() {
    for (auto& child : childScopes){
        child->setScopeLevel(scopeLevel+1);
        includeMetaData(child);
        child->updateChildLevelsRecursively();
    }
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



void Scope::setParent(SharedPtr<Scope> scope) {
    if (scope){
        scope->appendChildScope(shared_from_this());
        scopeLevel = scope->getScopeLevel() + 1;
    }
    parentScope = scope;
    
}



bool Scope::has(const SharedPtr<Scope>& checkScope) {
    if (!checkScope) {
        throw MerkError("ChildScope for checking is null");
    }
    if (this == checkScope.get()) {
        // DEBUG_LOG(LogLevel::PERMISSIVE, "Scopes Match at: ", checkScope.get(), 
        //           "ScopeLevel: ", checkScope->getScopeLevel(), "Owner: ", owner);
        return true;
    }
    for (auto& child : getChildren()) {
        if (child->has(checkScope)) {
            return true;
        }
    }
    return false;
}

