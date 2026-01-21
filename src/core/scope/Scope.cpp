#include <iostream>
#include <unordered_set>
#include "core/Scope.hpp"

#include <cassert>



#include "core/node/Node.hpp"
#include "core/node/ParamNode.hpp"
#include "core/node/ArgumentNode.hpp"

#include "core/types.h"
#include "core/errors.h"
#include "utilities/helper_functions.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"

// #include "ast/AstChain.hpp"



#include "core/registry/Context.hpp"
#include "core/registry/ClassRegistry.hpp"
#include "core/registry/FunctionRegistry.hpp"
#include "core/errors.h"
#include "core/callables/functions/Function.hpp"



int totalWith = 0;
int totalWithout = 0;


ScopeCounts Scope::getCounts() {
    return counts;
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

bool Scope::isAncestorOf(const Scope* maybeDesc) const {
    for (auto p = maybeDesc; p; ) {
        if (p == this) {return true;}
        if (p->getParent()) {
            p = p->getParent().get();
        }
           // or however you store parent
    }
    return false;
}


void Scope::validateNoCycles(SharedPtr<Scope> childScope){
    if (!childScope) throw MerkError("appendChildScope: childScope null");
    if (childScope.get() == this) throw MerkError("appendChildScope: cannot parent self");

    // If it already has a parent, that's a bug unless you have a real detach/reattach protocol.
    if (auto existingParent = childScope->parentScope.lock()) {
        if (existingParent.get() == this) {
            // already attached here: avoid duplicates
            if (!hasImmediateChild(childScope)) childScopes.push_back(childScope);
            return;
        }
        throw MerkError("appendChildScope: child already has a parent: " +
                        existingParent->metaString() + " -> " + childScope->metaString());
    }

    // Prevent parent-cycles: child cannot be one of my ancestors.
    for (auto p = shared_from_this(); p; p = p->parentScope.lock()) {
        if (p.get() == childScope.get()) {
            throw MerkError("appendChildScope: would create cycle (child is ancestor)");
        }
    }
}

void Scope::appendChildScope(SharedPtr<Scope> childScope, bool update) {
    if (!childScope) throw MerkError("appendChildScope: childScope null");
    if (childScope.get() == this) throw MerkError("appendChildScope: cannot parent self");
    auto hasChild = this->has(childScope);
    if (!hasChild) {
        childScope->parentScope = shared_from_this();
        childScope->scopeLevel = getScopeLevel() + 1;
        childScopes.push_back(childScope);
        totalWithout += 1;
    } else {
        totalWith += 1;
    }

    childScope->isDetached = false;
    childScope->owner = owner;
    includeMetaData(childScope, false);

    if (update) childScope->updateChildLevelsRecursively();
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
    //       " parent=", (void*)parentScope.lock().get(),
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
    if (context.hasVariable(name)) {
        auto varRef = context.getVariable(name);
        if (!varRef.has_value()) {
            throw VariableNotFoundError(name, "Scope::updateVariable -> context.getVariable failed");
        }

        VarNode& var = varRef.value().get();
        DataTypeFlags vf = var.getVarFlags();   // add accessor if you don't have it

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
            vf.inferredSig = tsr.inferFromValue(value);
        }

        // 3) apply value mutability semantics (:= freezes value)
        Node stored = value;
        // stored.getFlags().isMutable = vf.isMutable;
        if (stored.getFlags().isMutable != vf.isMutable) {
            stored.getFlags().isMutable = vf.isMutable;
        }
        // 4) store using existing mechanism (calls VarNode::setValue once)
        context.updateVariable(name, stored);
        return;
    }

    // Not in this scope -> delegate to parent
    if (auto parent = parentScope.lock()) {
        parent->updateVariable(name, value);
        return;
    }

    throw VariableNotFoundError(name, "Scope::updateVariable");
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
    // DEBUG_LOG(LogLevel::PERMISSIVE, "Scope::getVariable Missed variable ", name, "in The below scope: ");
    // debugPrint();

    if (hasVariable(name)) {throw MerkError("The Variable Lives Here, yet it is not getting pulled");}


    throw VariableNotFoundError(name, "Scope::getVariable");
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
