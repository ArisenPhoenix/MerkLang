#include <iostream>
#include <unordered_set>

#include "core/node/node.h"

#include "core/types.h"

#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"

// #include "core/node/param_node.h"
#include "core/context.h"
// #include "core/callables/functions/function.h"
#include "core/registry/class_registry.h"
#include "core/registry/function_registry.h"
#include "core/errors.h"
#include "core/scope.h"


String ScopeMeta::metaString() const {
    std::ostringstream oss;
    oss << "isRoot: " << (isRoot ? "true" : "false") << " | ";
    oss << "scopeLevel: " << scopeLevel << " | ";
    oss << "isDetached: " << (isDetached ? "true" : "false") << " | ";
    oss << "isCallable: " << (isCallableScope ? "true" : "false") << " | ";
    oss << "isCloned: " << (isClonedScope ? "true" : "false");
    return oss.str();
}


SharedPtr<Scope> Scope::getRoot() {
    SharedPtr<Scope> root = shared_from_this();
    std::unordered_set<void*> visited;
    while (root->getParent()) {
        void* addr = root.get();
        if (visited.count(addr)) {
            DEBUG_LOG(LogLevel::ERROR, "Cycle detected in Scope::getRoot at scope=", addr);
            break;
        }
        visited.insert(addr);
        root = root->getParent();
    }
    return root;
}


// In Scope class

SharedPtr<Scope> Scope::clone(bool strict) const {
    (void)strict;
    // DEBUG_FLOW(FlowLevel::MED);

    SharedPtr<Scope> newScope;
    if (auto parent = parentScope.lock()) {
        // use the weak/child constructor
        newScope = makeShared<Scope>(parent, globalFunctions, globalClasses, interpretMode);
    } else {
        if (strict) { throw ParentScopeNotFoundError(); }

        newScope = makeShared<Scope>(0, interpretMode);
        newScope->globalClasses = globalClasses;
        newScope->globalFunctions = globalFunctions;
    }

    newScope->context = context.clone();

    newScope->localFunctions = this->localFunctions.clone();
    newScope->localClasses = this->localClasses.clone();
    newScope->isClonedScope  = true;
    includeMetaData(newScope, isDetached);

    // DEBUG_FLOW_EXIT();
    return newScope;
    // return shared_from_this();
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
    if (scopeLevel == 0) { debugLog(true, highlight("\n\n======================== Start Scope::debugPrint ======================== ", Colors::cyan)); }

    debugLog(true, metaString());
    context.debugPrint();

    bool printed = false;
    if (isRoot && !printed) {
        printed = true;
        globalFunctions->debugPrint();
        globalClasses->debugPrint();
    }

    // if (!isCallableScope) {
        localFunctions.debugPrint();
        localClasses.debugPrint();
    // }
    
    

    for (const auto& child : childScopes) {
        if (child.get() != this) {
            child->debugPrint();
        }
        
    }

    if (scopeLevel == 0) { debugLog(true, highlight("======================== End Scope::debugPrint ======================== \n", Colors::cyan)); }

    DEBUG_FLOW_EXIT();
}

// Recursively print all child scopes and their critical information
void Scope::printChildScopes(int indentLevel) const {
    // Helper to create indentation for nested scopes
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    if (scopeLevel == 0) { debugLog(true, highlight("\n\n======================== Start Scope::printChildScopes ========================", Colors::cyan)); }
    auto parent = parentScope.lock();
    auto indent = String(indentLevel+2, ' ');
    int numInstances = 0;
    // for (auto& [varName, var] : context.getVariables()) {
    //     if (var->isInstance()) {
    //         numInstances += 1;
    //     }
    // }

    // Print information about the current scope
    debugLog(true, indent,
        "Scope Level:", scopeLevel, 
        "| Memory Loc:", this, 
        "| Parent Loc:", parentScope.lock() ? parentScope.lock() : nullptr,
        "| Number of Variables:", context.getVariables().size() - numInstances,
        "| Number of Children:", childScopes.size(),
        // "| Num Functions:", localFunctions.size(),
        // "| Num Classes:", localClasses.size(),
        "| Num Instance: ", numInstances,
        "| Owner:", !owner.empty() ? owner : "<None Provided>>"
        );
    
    // if (numInstances > 0) {
    //     debugLog(true, indent, "=================== GO INSTANCE LOG ===================");
    //     for (auto& [varName, var] : context.getVariables()) {
    //         if (var->isInstance()) {
    //             auto instance = std::get<SharedPtr<ClassInstance>>(var->getValue());
    //             instance->getInstanceScope()->printChildScopes(indentLevel+2);
    //             instance->getCapturedScope()->printChildScopes(indentLevel+2);
    //         }
    //     }
    //     debugLog(true, indent, "=================== END INSTANCE LOG ===================");

    // }
    // Recursively print each child scope
    // for (const auto& child : childScopes) {
    //     child->printChildScopes(indentLevel+2); // Increase indentation for child
    // }
    if (scopeLevel == 0){
        debugLog(true, highlight("======================== End Scope::printChildScopes ========================\n", Colors::cyan));
    }

    DEBUG_FLOW_EXIT();
}

void Scope::printContext(int depth) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    String indent(depth * 2, ' '); // Indentation based on depth
    std::cout << indent << "Scope Level: " << scopeLevel
              << " | Memory Loc: " << this
              << " | Parent Loc: " << (parentScope.lock() ? parentScope.lock().get() : nullptr)
              << " | Number of Variables: " << context.getVariables().size()
              << " | Number of Children: " << childScopes.size() << std::endl;

    // Print variables in the current context
    // for (const auto& [name, value] : context.getVariables()) {
    //     if (value) {
    //         std::cout << indent << "  Variable: " << name
    //                   << " = " << *value << std::endl;  // Dereference unique_ptr
    //     } else {
    //         std::cout << indent << "  Variable: " << name << " = [null]" << std::endl;
    //     }
    // }

    // Recursively print the parent scope, if any
    if (auto parent = parentScope.lock()) {
        parent->printContext(depth + 1);
    }
    DEBUG_FLOW_EXIT();
}

void Scope::addMember(String& varName) {
    classMembers.emplace(varName, varName);

}
void Scope::addMember(String& varName, String& var) {
    classMembers.emplace(varName, var);
}

ClassMembers Scope::getClassMembers() const {
    return classMembers;
}

bool Scope::hasMember(String& varName) {
    for (auto& [var, data] : classMembers) {
        if (var == varName) {
            return true;
        }
    }
    return false;
}

void Scope::setClassMembers(ClassMembers members) {
    classMembers = members;
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




