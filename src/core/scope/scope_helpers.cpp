#include <iostream>
#include <unordered_set>

#include "core/node/Node.hpp"
#include "core/types.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"
#include "core/registry/Context.hpp"
#include "core/registry/ClassRegistry.hpp"
#include "core/registry/FunctionRegistry.hpp"
#include "core/errors.h"
#include "core/Scope.hpp"


String ScopeMeta::metaString() const {
    std::ostringstream oss;
    oss << "isRoot: " << (isRoot ? "true" : "false") << " | ";
    oss << "scopeLevel: " << scopeLevel << " | ";
    oss << "isDetached: " << (isDetached ? "true" : "false") << " | ";
    oss << "isCallable: " << (isCallableScope ? "true" : "false") << " | ";
    oss << "isCloned: " << (isClonedScope ? "true" : "false") << " | ";
    oss << "kind: " << scopeKindToString(kind);
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
    SharedPtr<Scope> newScope;
    if (auto parent = parentScope.lock()) {
        newScope = makeShared<Scope>(parent, globalFunctions, globalClasses, globalTypes, interpretMode);
    } else {
        if (strict) { throw ParentScopeNotFoundError(); }

        newScope = makeShared<Scope>(0, interpretMode);
        newScope->globalClasses = globalClasses;
        newScope->globalFunctions = globalFunctions;
        newScope->globalTypes = globalTypes;
    }

    newScope->context = context.clone();

    newScope->localFunctions = this->localFunctions.clone();
    newScope->localClasses = this->localClasses.clone();
    newScope->localTypes = this->localTypes;

    newScope->isClonedScope  = true;
    includeMetaData(newScope, isDetached);

    return newScope;
}

Vector<SharedPtr<Scope>>& Scope::getChildren() { return childScopes; }

bool Scope::hasChildren() { return childScopes.size() > 0; }

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

    localFunctions.debugPrint();
    localClasses.debugPrint();
    // if (kind == ScopeKind::Root) {
    //     localTypes.debugPrint();
    // }
    
    for (const auto& child : childScopes) {
        if (child.get() != this) {
            child->debugPrint();
        }
    }

    if (scopeLevel == 0) { debugLog(true, highlight("======================== End Scope::debugPrint ======================== \n", Colors::cyan)); }
    DEBUG_FLOW_EXIT();
}

void Scope::printChildScopes(int indentLevel) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    if (scopeLevel == 0) { debugLog(true, highlight("\n\n======================== Start Scope::printChildScopes ========================", Colors::cyan)); }
    auto parent = parentScope.lock();
    auto indent = String(indentLevel+2, ' ');
    int numInstances = 0;

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




std::optional<TypeSignatureId> Scope::lookupTypeSigName(const String& name) {
    // Ensure TSR attached
    if (!localTypes.isAttached()) {
        if (!globalTypeSigs) throw MerkError("Scope::lookupTypeSigName: globalTypeSigs is null");
        localTypes.attach(*globalTypeSigs);
    }

    // 1) Local aliases first (shadowing)
    if (auto local = localTypes.lookupName(name)) return local;

    // 2) If the name is a nominal type in globalTypes, return its nominal signature
    if (globalTypes) {
        TypeId tid = globalTypes->lookupOrInvalid(name);
        if (tid != kInvalidTypeId) {
            return localTypes.nominal(tid); // forwards to manager
        }
    }

    // 3) Parent chain
    if (auto parent = parentScope.lock()) {
        return parent->lookupTypeSigName(name);
    }

    return std::nullopt;
}



TypeSignatureId Scope::inferSigFromNode(const Node& n, TypeSignatureRegistry& reg) {
    // FIRST: instances

    if (n.isNull()) {return reg.any();}
    if (n.isInstance()) {
        auto inst = n.toInstance();
        if (inst) {
            const String& cn = inst->getName();  // whatever yours is
            // Treat list/dict as containers if they are native classes
            if (cn == "List") return reg.container("List", { reg.any() });
            if (cn == "Dict") return reg.container("Dict", { reg.any(), reg.any() });
            return reg.classType(cn);
        }
        return reg.any();
    }

    // SECOND: direct container nodes (if any)
    if (n.isList()) return reg.container("List", { reg.any() });
    if (n.isDict()) return reg.container("Dict", { reg.any(), reg.any() });

    // LAST: primitives by actual runtime node type
    switch (n.getType()) {
        case NodeValueType::Bool:
        case NodeValueType::Int:
        case NodeValueType::Float:
        case NodeValueType::Double:
        case NodeValueType::String:
            return reg.primitive(n.getType());
        default:
            return reg.any();
    }
}