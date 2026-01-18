#include <iostream>
#include <unordered_set>
#include "core/Scope.hpp"

#include <cassert>

#ifndef _WIN32
#include <execinfo.h>
#endif

#include "core/node/Node.hpp"
#include "core/node/ParamNode.hpp"
#include "core/node/ArgumentNode.hpp"

#include "core/types.h"
#include "core/errors.h"
#include "utilities/helper_functions.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"

#include "ast/ast_validate.h"
#include "ast/AstChain.hpp"
#include "core/callables/Callable.hpp"
#include "core/callables/classes/Method.hpp"



#include "core/registry/Context.hpp"
#include "core/registry/ClassRegistry.hpp"
#include "core/registry/FunctionRegistry.hpp"
#include "core/errors.h"
#include "core/callables/functions/Function.hpp"
// #include "core/callables/classes/Method.hpp"


int totalWith = 0;
int totalWithout = 0;

String ScopeCounts::toString() {
    String out;
    out += "FunctionCalls: " + std::to_string(functionCalls) + "\n";
    out += "MethodCalls: " + std::to_string(methodCalls) + "\n";
    out += "InstanceCalls: " + std::to_string(instanceCalls) + "\n";
    out += "ClassCalls: " + std::to_string(classCalls) + "\n";
    out += "Blocks: " + std::to_string(blocks) + "\n";
    return out;
}


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

String scopeKindToString(ScopeKind kind) {
    switch (kind)
    {
    case ScopeKind::Root: return "Root";
    case ScopeKind::Block: return "Block";
    case ScopeKind::FunctionDef: return "FunctionDef";
    case ScopeKind::FunctionCall: return "FunctionCall";
    case ScopeKind::ClassDef: return "ClassDef";
    case ScopeKind::ClassScope: return "ClassScope";
    case ScopeKind::Instance: return "Instance";
    case ScopeKind::Captured: return "Captured";
    case ScopeKind::Detached: return "Detached";
    case ScopeKind::Isolated: return "Isolated";
    case ScopeKind::MethodDef: return "MethodDef";
    case ScopeKind::MethodCall: return "MethodCall";

    default:
        return "UNKNOWN";
    }
};


void Scope::linkTypes() {
    if (auto parent = this->parentScope.lock()) {
        scopeLevel = parent->scopeLevel + 1;
        globalFunctions = parent->globalFunctions;
        globalClasses   = parent->globalClasses;
        globalTypes     = parent->globalTypes;
        globalTypeSigs  = parent->globalTypeSigs;

        if (!globalTypes)    throw MerkError("Child Scope: globalTypes is null");
        if (!globalTypeSigs) throw MerkError("Child Scope: globalTypeSigs is null");

        localTypes.attach(*globalTypeSigs);
    } else {
        throw MerkError("Scope Initialized Incorrectly -> Scope::linkTypes() && " + metaString());
    }
}


void Scope::initRootTypes() {
    if (!isRoot) throw MerkError("initRootTypes called on non-root scope");

    if (!globalTypes) throw MerkError("initRootTypes: globalTypes is null");

    if (!globalTypeSigs) {
        globalTypeSigs = makeShared<TypeSignatureRegistryManager>(*globalTypes);
    }

    localTypes.attach(*globalTypeSigs);
}


// constructor for root.
// Scope::Scope(int scopeNum, bool interpretMode, bool isRootBool) 
//     : interpretMode(interpretMode) {
//         isRoot = isRootBool;
//         if (isRoot) {
//             Scope::counts.roots += 1;
//         }
//         scopeLevel = scopeNum;
//         DEBUG_FLOW(FlowLevel::LOW);

//     DEBUG_LOG(LogLevel::DEBUG, "[Ctor(root)] this=", this, " level=", scopeLevel, "\n");
//     // if (!globalTypes) {
        
//     // }
//     if (isRoot) {
//         globalFunctions = makeShared<FunctionRegistry>();
//         globalClasses = makeShared<ClassRegistry>();
//         globalTypes = makeShared<TypeRegistry>();
//     }
//     // String rootStr = isRoot ? "true" : "false";
//     // if (!globalTypes) {
//     //     throw MerkError("Global Types Was Not Initialized and isRoot is set to: " + rootStr);
//     // }

//     DEBUG_LOG(LogLevel::TRACE, 
//              "Initialized Root Scope with level: ", scopeLevel, 
//              " | Memory Loc: ", this, 
//              " | Parent Loc: ", formatPointer(nullptr));

//     ++liveScopeCount;
//     ++totalScopeCreated;
//     DEBUG_FLOW_EXIT();
// }

// Scope::Scope(int scopeNum, bool interpretMode, bool isRootBool)
//   : interpretMode(interpretMode)
// {
//     isRoot = isRootBool;
//     scopeLevel = scopeNum;

//     if (isRoot) {
//         globalFunctions = makeShared<FunctionRegistry>();
//         globalClasses   = makeShared<ClassRegistry>();
//         if (!globalTypes) {
//             globalTypes = makeShared<TypeRegistry>();         // nominal ids live here
//         }
        
//         localTypes      = TypeSignatureRegistry(*globalTypes, nullptr);
//     } 
    
//     // else {
//     //     throw MerkError("Non-root Scope(int,...) ctor used with isRoot=false");
//     // }

//     ++liveScopeCount;
//     ++totalScopeCreated;
// }

Scope::Scope(int scopeNum, bool interpretMode, bool isRootBool)
  : interpretMode(interpretMode)
{
    isRoot = isRootBool;
    scopeLevel = scopeNum;

    if (isRoot) {
        globalFunctions = makeShared<FunctionRegistry>();
        globalClasses   = makeShared<ClassRegistry>();

        globalTypes = makeShared<TypeRegistry>();                 // nominal TypeId space
        globalTypeSigs = makeShared<TypeSignatureRegistryManager>(*globalTypes); // signature pool

        initRootTypes();
    }

    ++liveScopeCount;
    ++totalScopeCreated;
}



String Scope::formattedScope() {
    std::ostringstream oss;
    oss << this;
    return oss.str();
}

ScopeCounts Scope::getCounts() {
    return counts;
}

// Constructor for child
Scope::Scope(SharedPtr<Scope> parent, SharedPtr<FunctionRegistry> globalF, SharedPtr<ClassRegistry> globalC, SharedPtr<TypeRegistry> globalT, bool interpretMode)
: parentScope(parent), 
    globalFunctions(std::move(globalF)),
    globalClasses(std::move(globalC)),
    globalTypes(std::move(globalT)),
    interpretMode(interpretMode)
{
    scopeLevel = parent ? parent->scopeLevel + 1 : 0;

    if (!parent) throw MerkError("Scope ctor: parent is null");
    globalTypeSigs = parent->globalTypeSigs;

    if (!globalTypes)    throw MerkError("Scope ctor: globalTypes is null");
    if (!globalTypeSigs) throw MerkError("Scope ctor: globalTypeSigs is null");

    localTypes.attach(*globalTypeSigs);

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
        if (globalTypes) globalTypes.reset();
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

// void Scope::appendChildScope(SharedPtr<Scope> child, bool update) {
//     if (!child) return;

//     if (child.get() == this) {
//         throw MerkError("Cannot append scope as its own child");
//     }

//     // Prevent cycles: if I'm already inside child's parent chain, adding child under me makes a cycle.
//     if (child->isAncestorOf(this)) {
//         throw MerkError("appendChildScope would create a cycle");
//     }

//     // Optional: prevent double-parenting
//     if (auto oldParent = child->getParent()) {
//         if (oldParent.get() != this) {
//             // Either detach from old parent, or throw.
//             oldParent->removeChildScope(child);
//         }
//     }

//     if (update) { child->updateChildLevelsRecursively(); }

//     child->setParent(shared_from_this());
//     childScopes.push_back(child);
// }


// void Scope::appendChildScope(SharedPtr<Scope> childScope, bool update) {
//     MARK_UNUSED_MULTI(update);
//     DEBUG_FLOW(FlowLevel::LOW);
//     auto hasChild = this->has(childScope);
//     if (!hasChild) {
//         childScope->parentScope = shared_from_this();
//         childScope->scopeLevel = getScopeLevel() + 1;
//         childScopes.push_back(childScope);
//         totalWithout += 1;
//     } else {
//         debugLog(true, "CHILD FOUND in Scope holding Meta: " + metaString());
//         throw MerkError("CHILD FOUND in Scope holding Meta: " + metaString());
//         totalWith += 1;
//     }

//     // KEEEEEEEEEEEEP THIS BLOCK IT IS FOR AFTER SCOPING WITHIN CALLABLES IS DETERMINANT
//     // childScope->parentScope = shared_from_this();
//     // childScope->scopeLevel = getScopeLevel() + 1;
//     // childScopes.push_back(childScope);

//     childScope->isDetached = false;
//     childScope->owner = owner;
//     includeMetaData(childScope, false);
    
//     if (update) { childScope->updateChildLevelsRecursively(); }
//     DEBUG_FLOW_EXIT();
// }

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
        // String out = "CHILD FOUND in Scope holding Meta: " + metaString() + " OWNER: " + owner;
        // debugLog(true, out);
        // if (kind != ScopeKind::Root && owner != "GLOBAL") {
        //     throw MerkError(out);
        // } else {
        //     debugLog(true, out);
        // }
        
        totalWith += 1;
    }
    // If it already has a parent, that's a bug unless you have a real detach/reattach protocol.
    // validateNoCycles(childScope);
    // if (childScope->getParent()) {
    //     childScope->getParent()->removeChildScope(childScope);
    // }
    
    // childScope->parentScope = shared_from_this();
    // childScope->scopeLevel  = getScopeLevel() + 1;
    // childScopes.push_back(childScope);

    childScope->isDetached = false;
    childScope->owner = owner;
    includeMetaData(childScope, false);

    if (update) childScope->updateChildLevelsRecursively();
}



SharedPtr<Scope> Scope::createChildScope() {
    auto c = makeShared<Scope>(shared_from_this(),
                                     globalFunctions,
                                     globalClasses,
                                     globalTypes,
                                     interpretMode);
    c->isRoot = false;
    childScopes.push_back(c);
    Scope::counts.blocks += 1;
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
    debugLog(true,
        "DECL x declaredSig=", localTypes.toString(value->getVarFlags().declaredSig),
        " inferredSig=", localTypes.toString(value->getVarFlags().inferredSig),
        " fullType=", value->getVarFlags().fullType.toString()
        );
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

        // TypeSignatureId rhsSig = inferSigFromRuntimeNode(value, tr);

        // TypeSignatureId expected = kInvalidTypeSignatureId;
        // if (vf.declaredSig != kInvalidTypeSignatureId && vf.declaredSig != tr.any()) {
        //     expected = vf.declaredSig;
        // } else if (vf.inferredSig != kInvalidTypeSignatureId) {
        //     expected = vf.inferredSig;
        // }

        // if (expected != kInvalidTypeSignatureId) {
        //     auto r = tr.matchValue(expected, value);
        //     if (!r.ok) {
        //         debugLog(true,
        //             "ASSIGN name=", name,
        //             " value.getType()=", nodeTypeToString(value.getType()),
        //             " value.isInstance()=", value.isInstance() ? "true" : "false",
        //             " value.toString()=", value.toString(),
        //             " expected=", tr.toString(expected),
        //             " rhsSig=", tr.toString(rhsSig)
        //         );

        //         throw MerkError(
        //             "Type mismatch for '" + name + "': expected " +
        //             tr.toString(expected) + ", got " + tr.toString(rhsSig)
        //         );
        //     }
        // } else {
        //     // infer-on-first-assign for var x = ...
        //     vf.inferredSig = rhsSig;
        // }

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

    throw VariableNotFoundError(name);
}



// void Scope::updateVariable(const String& name, const Node& value) {
//     if (context.hasVariable(name)) {
//         context.updateVariable(name, value);
//         return;
//     }

//     // DEBUG_LOG(LogLevel::PERMISSIVE, "Scope::updateVariable Missed variable ", name, "in The below scope: ");
//     // debugPrint();

//     // Delegate to parent scope if the variable is not in the current scope
//     if (auto parent = parentScope.lock()) {
//         parent->updateVariable(name, value);
//         return;
//     }

//     throw VariableNotFoundError(name);
// }

VarNode& Scope::getVariable(const String& name) {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);

    if (auto variable = context.getVariable(name)) {
        return variable.value();
    }
    // DEBUG_LOG(LogLevel::PERMISSIVE, "Scope::getVariable Missed variable ", name, "in The below scope: ");
    // debugPrint();

    // Delegate to parent scope if it exists
    if (auto parent = parentScope.lock()) {
        return parent->getVariable(name);
    }

    if (hasVariable(name)) {throw MerkError("The Variable Lives Here, yet it is not getting pulled");}
    // DEBUG_LOG(LogLevel::PERMISSIVE, "LOOKUP MISS name=", name,
    //       " start this=", (void*)this,
    //       " level=", scopeLevel,
    //       " owner=", owner,
    //       " kind=", scopeKindToString(kind));

    // for (auto s = shared_from_this(); s; s = s->parentScope.lock()) {
    //     DEBUG_LOG(LogLevel::PERMISSIVE,
    //         "  chain scope=", (void*)s.get(),
    //         " level=", s->scopeLevel,
    //         " owner=", s->owner,
    //         " kind=", scopeKindToString(s->kind),
    //         " keys=", s->context.keysToString()); // add this
    // }

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
    // Prefer local overload sets (shadowing). If not found, walk up parents, then fall back to globals.
    if (auto funcs = localFunctions.getFunction(name)) { return funcs; }
    if (auto parent = parentScope.lock()) { return parent->lookupFunction(name); }
    if (globalFunctions && globalFunctions->hasFunction(name)) {
        auto sig = globalFunctions->getFunction(name);
        if (sig.has_value()) return sig;
    }
    return std::nullopt;
  }

// ------------------------------------------------------------
// Type-aware overload resolution
// ------------------------------------------------------------
std::optional<SharedPtr<CallableSignature>> Scope::resolveFunctionOverload(
    const String& name,
    const ArgumentList& args,
    const TypeMatchOptions& opt
) {
    auto overloadOpt = lookupFunction(name);
    if (!overloadOpt.has_value() || overloadOpt.value().empty()) {
        return std::nullopt;
    }

    auto& reg = localTypes; // shared across scope chain

    struct Candidate {
        SharedPtr<CallableSignature> sig;
        TypeMatchResult match;
        BoundArgs bound;
    };

    Vector<Candidate> viable;
    viable.reserve(overloadOpt.value().size());

    for (auto& sig : overloadOpt.value()) {
        if (!sig) continue;

        // DEF: keep as a fallback if nothing else matches.
        if (sig->getSubType() == CallableType::DEF) {
            Candidate c;
            c.sig = sig;
            c.match = TypeMatchResult::Yes(0, 1000);
            viable.push_back(std::move(c));
            continue;
        }

        // First: bind args to params (named/default/varargs). If it fails, this overload isn't viable.
        ParamList params = sig->getParameters();
        BoundArgs bound;
        try {
            bound = args.bindToBound(params, /*allowDefaults=*/true);
        } catch (...) {
            continue;
        }

        // Ensure callable has a type signature id (lazily built).
        if (sig->getTypeSignature() == kInvalidTypeSignatureId) {
            // Bind param annotations to TypeSignatureIds
            params.bindTypes(reg, *this);

            InvocableSigType m;
            m.methodName = name;
            m.variadic = (!params.empty() && params.back().isVarArgsParameter());
            m.ret = reg.any();
            m.retEnforced = false;

            m.params.reserve(params.size());
            m.enforced.reserve(params.size());
            for (size_t i = 0; i < params.size(); ++i) {
                const auto& p = params[i];
                if (p.isTyped() && p.getTypeSig() != 0) {
                    m.params.push_back(p.getTypeSig());
                    m.enforced.push_back(1);
                } else {
                    m.params.push_back(reg.any());
                    m.enforced.push_back(0);
                }
            }

            sig->setTypeSignature(reg.invocableType(m));
        }

        // Match types against the bound/flattened args.
        ArgumentList flat;
        auto flatNodes = bound.flatten();
        for (auto& n : flatNodes) flat.addPositionalArg(n);

        auto match = reg.matchCall(sig->getTypeSignature(), flat, opt);
        if (!match.ok) continue;

        viable.push_back(Candidate{sig, match, std::move(bound)});
    }

    if (viable.empty()) {
        return std::nullopt;
    }

    // Pick best by score, then lowest cost.
    auto bestIt = viable.begin();
    for (auto it = viable.begin(); it != viable.end(); ++it) {
        if (it->match.score > bestIt->match.score) bestIt = it;
        else if (it->match.score == bestIt->match.score && it->match.cost < bestIt->match.cost) bestIt = it;
    }

    // Ambiguity check: same score+cost but different signatures.
    int bestScore = bestIt->match.score;
    int bestCost = bestIt->match.cost;
    int ties = 0;
    for (auto& c : viable) {
        if (c.match.score == bestScore && c.match.cost == bestCost) ties++;
    }
    if (ties > 1) {
        throw MerkError("Ambiguous overload for '" + name + "' (" + std::to_string(ties) + " candidates match equally)");
    }

    return bestIt->sig;
}

void Scope::registerType(TypeId id) {
    if (!globalTypes) throw MerkError("registerType: globalTypes is null");
    if (!localTypes.isAttached()) linkTypes();
    // Ensure the signature exists in this scope and bind the canonical name.
    const String& n = globalTypes->nameOf(id);
    if (!n.empty() && n != "<invalid>") {
        localTypes.bindName(n, localTypes.nominal(id));
    }
}

void Scope::registerPrimitiveType(NodeValueType t) {
    if (!globalTypes) throw MerkError("registerPrimitiveType: globalTypes is null");
    if (!localTypes.isAttached()) linkTypes();
    TypeId id = globalTypes->primitiveId(t);
    const String& n = globalTypes->nameOf(id);
    if (!n.empty() && n != "<invalid>") {
        localTypes.bindName(n, localTypes.nominal(id));
    }
}

void Scope::registerNamedType(String& name) {
    if (!globalTypes) throw MerkError("registerNamedType: globalTypes is null");
    if (!localTypes.isAttached()) linkTypes();
    TypeId id = globalTypes->getOrCreate(name);
    localTypes.bindName(name, localTypes.nominal(id));
}

SharedPtr<TypeRegistry> Scope::getTypeRegistry() {
    return globalTypes;
}

// TypeId Scope::getTypeOf(TypeNode val) {
//     // return TypeRegistry::idOf(val);
//     return 0
// }




std::optional<SharedPtr<CallableSignature>> Scope::getFunction(const String& name, const ArgumentList& args) {

    DEBUG_FLOW(FlowLevel::MED);

    // Overload resolution is now type-aware and centralized here.
    auto resolved = resolveFunctionOverload(name, args);
    DEBUG_FLOW_EXIT();
    return resolved;
}

// For Function Reference
std::optional<Vector<SharedPtr<CallableSignature>>> Scope::getFunction(const String& name) {
    if (auto funcs = lookupFunction(name)) { return funcs.value(); }
    if (auto parent = parentScope.lock()) { return parent->getFunction(name); }
    if (globalFunctions) {
        auto globals = globalFunctions->getFunction(name);
        if (globals) { return globals.value(); }
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
    globalTypes->getOrCreate(name);
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

SharedPtr<ClassRegistry> Scope::getClassRegistry() { return globalClasses; }
