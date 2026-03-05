#include <iostream>
#include <unordered_set>
#include <vector>
#include "core/Scope.hpp"

#include <cassert>



#include "core/node/Node.hpp"
#include "core/node/ParamNode.hpp"


#include "core/types.h"
#include "core/errors.h"
#include "utilities/helper_functions.h"

#include "utilities/debugger.h"

// #include "ast/AstChain.hpp"



#include "core/registry/Context.hpp"
#include "core/registry/FunctionRegistry.hpp"
#include "core/errors.h"
#include "core/callables/functions/Function.hpp"



ScopeCounts Scope::getCounts() {
    return counts;
}

void Scope::refreshVariableLookupCache() const {
    const uint64_t epoch = variableLookupEpoch.load(std::memory_order_relaxed);
    if (variableLookupCacheEpoch != epoch) {
        variableLookupCache.clear();
        variableLookupCacheEpoch = epoch;
    }
}

void Scope::bumpVariableLookupEpoch() {
    variableLookupEpoch.fetch_add(1, std::memory_order_relaxed);
}


const Context& Scope::getAllVariables(Context& callingContext) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    if (!context.getVariables().empty()){
        for (const auto& [varName, varPtr] : context.getVariables()) {
            callingContext.setVariable(varName, varPtr->uniqClone());
        }
    }


    if (auto parent = getParent()){
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

    if (auto parent = getParent()){
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
    MARK_UNUSED_MULTI(callerLabel);
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
        if (child.get() == candidate.get()) { return true; }
    }
    return false;
}


bool Scope::has(const SharedPtr<Scope>& checkScope) {
    if (!checkScope) {
        throw MerkError("ChildScope for checking is null");
    }
    std::unordered_set<const Scope*> visited;
    std::vector<const Scope*> stack;
    stack.push_back(this);

    while (!stack.empty()) {
        const Scope* node = stack.back();
        stack.pop_back();

        if (!visited.insert(node).second) {
            continue;
        }
        if (node == checkScope.get()) {
            return true;
        }

        for (const auto& child : node->childScopes) {
            if (child) {
                stack.push_back(child.get());
            }
        }
    }
    return false;
}

bool Scope::isAncestorOf(const Scope* maybeDesc) const {
    std::unordered_set<const Scope*> visited;
    for (auto p = maybeDesc; p != nullptr; ) {
        if (p == this) {
            return true;
        }
        if (!visited.insert(p).second) {
#if MERK_SCOPE_DIAGNOSTICS
            ancestorParentChainCycleBailouts.fetch_add(1, std::memory_order_relaxed);
            DEBUG_LOG(
                LogLevel::WARNING,
                "[ScopeDiag] isAncestorOf bailed due to parent-chain cycle | target=",
                this,
                " | probe=",
                maybeDesc,
                " | repeated=",
                p
            );
#endif
            return false; // Existing cycle in parent chain.
        }
        auto parent = p->parentScope.lock();
        p = parent.get();
    }
    return false;
}


void Scope::validateNoCycles(SharedPtr<Scope> childScope){
    if (!childScope) throw MerkError("appendChildScope: childScope null");
    if (childScope.get() == this) throw MerkError("appendChildScope: cannot parent self");

    // If it already has a parent, that's a bug unless you have a real detach/reattach protocol.
    if (auto existingParent = childScope->getParent()) {
        if (existingParent.get() == this) {
            // already attached here: avoid duplicates
            if (!hasImmediateChild(childScope)) childScopes.push_back(childScope);
            return;
        }
        std::cout << "appendChildScope conflict: dumping scope trees before throw." << std::endl;
        std::cout << "[current scope]" << std::endl;
        printScopeTree();
        std::cout << "[existing parent scope]" << std::endl;
        existingParent->printScopeTree();
        std::cout << "[child scope]" << std::endl;
        childScope->printScopeTree();
        throw MerkError("appendChildScope: child already has a parent: " +
                        existingParent->metaString() + " -> " + childScope->metaString());
    }

    // Prevent parent-cycles: child cannot be one of my ancestors.
    for (auto p = shared_from_this(); p; p = p->getParent()) {
        if (p.get() == childScope.get()) {
            throw MerkError("appendChildScope: would create cycle (child is ancestor)");
        }
    }
}

void Scope::appendChildScope(SharedPtr<Scope> childScope, bool update) {
    if (!childScope) throw MerkError("appendChildScope: childScope null");
    if (childScope.get() == this) throw MerkError("appendChildScope: cannot parent self");

    // Always block true cycle creation on attach.
    if (childScope->isAncestorOf(this)) {
        throw MerkError("appendChildScope: would create cycle (child is ancestor)");
    }
#if MERK_SCOPE_DIAGNOSTICS
    diagnosticAppend(childScope);
#endif

    // Fast-path in normal runtime: only check immediate children.
    if (hasImmediateChild(childScope)) {
        totalWith += 1;
        return;
    }

#if MERK_SCOPE_DIAGNOSTICS
    // Deeper graph reachability check is diagnostic-only.
    if (this->has(childScope)) {
        DEBUG_LOG(
            LogLevel::WARNING,
            "[ScopeDiag] append target already reachable in subtree (non-immediate) | parent=",
            this,
            " | child=",
            childScope.get()
        );
        totalWith += 1;
        return;
    }
#endif

    childScope->parentScope = shared_from_this();
    childScope->scopeLevel = getScopeLevel() + 1;
    childScopes.push_back(childScope);
    totalWithout += 1;

    childScope->isDetached = false;
    includeMetaData(childScope, false);

    if (update) childScope->updateChildLevelsRecursively();
}

SharedPtr<Scope> Scope::getParent() const {
    DEBUG_FLOW(FlowLevel::LOW);
    if (auto parent = parentScope.lock()) {
        // DEBUG_LOG(LogLevel::TRACE, "DEBUG Scope::getParent: Accessing Parent Scope at level: ", parent->getScopeLevel(), 
        //          " | Memory Loc: ", parent.get(), 
        //          " | Called from Scope Level: ", scopeLevel, " | Memory Loc: ", this);
        DEBUG_FLOW_EXIT();
        return parent;
    }

    DEBUG_FLOW_EXIT();
    if (kind != ScopeKind::Root) {
        throw ParentScopeNotFoundError();
    }
    return nullptr; // Explicitly return null for root scope
}

void Scope::setVariable(const String& name, UniquePtr<VarNode> value, bool isDeclaration) {
    DEBUG_FLOW(FlowLevel::LOW);
    

    if (isDeclaration) {
        if (context.hasVariable(name)) { throw VariableAlreadyDeclaredError(name + " in Scope::setVariable", shared_from_this()); }
        context.setVariable(name, std::move(value));  // Declare variable

    } else {
        if (context.hasVariable(name)) {
            context.setVariable(name, std::move(value));  // Update in the current scope
        } else if (auto parent = getParent()) {
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
    // debugLog(true,
    //     "DECL x declaredSig=", localTypes.toString(value->getVarFlags().declaredSig),
    //     " inferredSig=", localTypes.toString(value->getVarFlags().inferredSig),
    //     " fullType=", value->getVarFlags().fullType.toString()
    //     );
    context.setVariable(name, std::move(value));
    
    // DEBUG_LOG(LogLevel::PERMISSIVE,
    //       "DECLARE name=", name,
    //       " this=", (void*)this,
    //       " level=", scopeLevel,
    //       " parent=", (void*)getParent().get(),
    //       " owner=", owner,
    //       " kind=", scopeKindToString(kind));
    // context.debugPrint();
    // DEBUG_LOG(LogLevel::PERMISSIVE, "Scope::declareVariable Missed variable ", name, "in The below scope: ");
    // debugPrint();
    DEBUG_FLOW_EXIT();
}

Context& Scope::getContextWith(const String& varName) {
    if (hasVariable(varName)) {
        return getContext();
    }

    if (auto parent = getParent()) {
        if (parent->hasVariable(varName)) {
            return parent->getContext();
        }
    }

    throw VariableNotFoundError(varName, "Scope::getContextWith");
}

void Scope::updateVariable(const String& name, const Node& value) {
    // If variable is in this context, validate and update here
    if (VarNode* var = context.findVariable(name)) {
        DataTypeFlags& vf = var->getVarFlags();

        // 1) const blocks reassignment (binding-level)
        if (vf.isConst) {
            throw MerkError("Cannot assign to const variable '" + name + "'");
        }

        // 2) type enforcement / inference
        auto& tsr = localTypes;
        bool hasDeclared = (vf.declaredSig != kInvalidTypeSignatureId && vf.declaredSig != tsr.any());

        if (hasDeclared) {
            TypeSignatureId rhsSig = tsr.inferFromValue(value);
            auto r = tsr.matchValue(vf.declaredSig, value);
            if (!r.ok) {
                throw MerkError(
                    "Type mismatch for '" + name + "': expected " +
                    tsr.toString(vf.declaredSig) + ", got " + tsr.toString(rhsSig)
                );
            }
        } else {
            // Keep a best-effort inferred signature without paying inference every assignment.
            if (vf.inferredSig == kInvalidTypeSignatureId) {
                vf.inferredSig = tsr.inferFromValue(value);
            }
        }

        // 3) apply value mutability semantics (:= freezes value) only when needed.
        if (value.getFlags().isMutable == vf.isMutable) {
            context.updateVariable(name, value);
            return;
        }
        Node stored = value;
        stored.getFlags().isMutable = vf.isMutable;
        context.updateVariable(name, stored);
        return;
    }

    // Not in this scope -> delegate to parent
    if (auto parent = getParent()) {
        parent->updateVariable(name, value);
        return;
    }

    throw VariableNotFoundError(name, "Scope::updateVariable");
}

VarNode& Scope::getVariable(const String& name) {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    for (Scope* s = this; s != nullptr; ) {
        if (VarNode* variable = s->context.findVariable(name)) {
            return *variable;
        }
        auto parent = s->getParent();
        s = parent.get();
    }
    // DEBUG_LOG(LogLevel::PERMISSIVE, "Scope::getVariable Missed variable ", name, "in The below scope: ");
    // debugPrint();

    if (hasVariable(name)) {throw MerkError("The Variable Lives Here, yet it is not getting pulled");}


    throw VariableNotFoundError(name, "Scope::getVariable");
}

// Check if a variable exists in the current scope or parent scopes
bool Scope::hasVariable(const String& name) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    for (const Scope* s = this; s != nullptr; ) {
        if (s->context.hasVariable(name)) {
            DEBUG_FLOW_EXIT();
            return true;
        }
        auto parent = s->getParent();
        s = parent.get();
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
