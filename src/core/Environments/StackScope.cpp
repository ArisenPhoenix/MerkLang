#include "core/Environments/StackScope.hpp"

#include "core/errors.h"
#include "core/Environments/Scope.hpp"

SharedPtr<Scope> StackScope::clone(bool strict) const {
    SharedPtr<StackScope> newScope;
    if (auto parent = getParent()) {
        newScope = makeShared<StackScope>(parent, globalFunctions, globalClasses, globalTypes, interpretMode);
    } else {
        if (strict && !isRoot && kind != ScopeKind::Root) {
            throw ParentScopeNotFoundError("clone called on non-root scope: " + metaString());
        }
        newScope = makeShared<StackScope>(globalFunctions, globalClasses, globalTypes, globalTypeSigs, interpretMode);
        newScope->isRoot = true;
    }

    newScope->context = context.clone();
    newScope->localFunctions = this->localFunctions.clone();
    newScope->localClasses = this->localClasses.clone();
    newScope->localTypes = this->localTypes;
    if (!getParent()) {
        newScope->localTypes.attach(*newScope->globalTypeSigs);
    }

    newScope->isClonedScope = true;
    includeMetaData(newScope, isDetached);

    return newScope;
}

void StackScope::noteSlotKind(NodeValueType valueType) {
    switch (Frame::classify(valueType)) {
        case Frame::SlotKind::Int:
            slotKindInt.fetch_add(1, std::memory_order_relaxed);
            break;
        case Frame::SlotKind::Double:
            slotKindDouble.fetch_add(1, std::memory_order_relaxed);
            break;
        case Frame::SlotKind::Bool:
            slotKindBool.fetch_add(1, std::memory_order_relaxed);
            break;
        case Frame::SlotKind::Object:
            slotKindObject.fetch_add(1, std::memory_order_relaxed);
            break;
        case Frame::SlotKind::Unknown:
        default:
            slotKindUnknown.fetch_add(1, std::memory_order_relaxed);
            break;
    }
}

VarNode* StackScope::findLocalVar(const String& name) {
    if (VarNode* slot = frame.getSlot(name)) {
        cacheHits.fetch_add(1, std::memory_order_relaxed);
        return slot; 
    }
    cacheMisses.fetch_add(1, std::memory_order_relaxed);

    VarNode* var = getContext().findVariable(name);
    if (var) {
        localContextHits.fetch_add(1, std::memory_order_relaxed);
        frame.bindSlot(name, var, var->getValueNode().getType());
    } else {
        localContextMisses.fetch_add(1, std::memory_order_relaxed);
    }
    return var;
}

const VarNode* StackScope::findLocalVar(const String& name) const {
    if (const VarNode* slot = frame.getSlot(name)) {
        cacheHits.fetch_add(1, std::memory_order_relaxed);
        return slot;
    }
    cacheMisses.fetch_add(1, std::memory_order_relaxed);

    const VarNode* var = getContext().findVariable(name);
    if (var) {
        localContextHits.fetch_add(1, std::memory_order_relaxed);
        frame.bindSlot(name, const_cast<VarNode*>(var), var->getValueNode().getType());
    } else {
        localContextMisses.fetch_add(1, std::memory_order_relaxed);
    }
    return var;
}

void StackScope::clear(bool internalCall) {
    frame.clear();
    Scope::clear(internalCall);
}

void StackScope::resetInstrumentation() {
    cacheHits.store(0, std::memory_order_relaxed);
    cacheMisses.store(0, std::memory_order_relaxed);
    localContextHits.store(0, std::memory_order_relaxed);
    localContextMisses.store(0, std::memory_order_relaxed);
    parentFallbackLookups.store(0, std::memory_order_relaxed);
    parentFallbackUpdates.store(0, std::memory_order_relaxed);
    slotKindUnknown.store(0, std::memory_order_relaxed);
    slotKindInt.store(0, std::memory_order_relaxed);
    slotKindDouble.store(0, std::memory_order_relaxed);
    slotKindBool.store(0, std::memory_order_relaxed);
    slotKindObject.store(0, std::memory_order_relaxed);
}

void StackScope::printInstrumentation(std::ostream& os) {
    const uint64_t hits = cacheHits.load(std::memory_order_relaxed);
    const uint64_t misses = cacheMisses.load(std::memory_order_relaxed);
    const uint64_t total = hits + misses;
    const double hitRate = (total > 0) ? (100.0 * static_cast<double>(hits) / static_cast<double>(total)) : 0.0;

    os << "StackScope Instrumentation:\n";
    os << "  cacheHits: " << hits << '\n';
    os << "  cacheMisses: " << misses << '\n';
    os << "  cacheHitRate: " << hitRate << "%\n";
    os << "  localContextHits: " << localContextHits.load(std::memory_order_relaxed) << '\n';
    os << "  localContextMisses: " << localContextMisses.load(std::memory_order_relaxed) << '\n';
    os << "  parentFallbackLookups: " << parentFallbackLookups.load(std::memory_order_relaxed) << '\n';
    os << "  parentFallbackUpdates: " << parentFallbackUpdates.load(std::memory_order_relaxed) << '\n';
    os << "  slotKindUnknown: " << slotKindUnknown.load(std::memory_order_relaxed) << '\n';
    os << "  slotKindInt: " << slotKindInt.load(std::memory_order_relaxed) << '\n';
    os << "  slotKindDouble: " << slotKindDouble.load(std::memory_order_relaxed) << '\n';
    os << "  slotKindBool: " << slotKindBool.load(std::memory_order_relaxed) << '\n';
    os << "  slotKindObject: " << slotKindObject.load(std::memory_order_relaxed) << '\n';
}

void StackScope::declareVariable(const String& name, UniquePtr<VarNode> value) {
    if (name.empty()) {
        throw MerkError("Name is an empty string WITH: " + value->toString());
    }

    Context& ctx = getContext();
    if (ctx.hasVariable(name)) {
        if (disregardDeclarations) {
            ctx.setVariable(name, std::move(value));
            VarNode* var = ctx.findVariable(name);
            if (var) {
                frame.bindSlot(name, var, var->getValueNode().getType());
                noteSlotKind(var->getValueNode().getType());
            }
            return;
        }
        throw VariableAlreadyDeclaredError(name, shared_from_this());
    }

    ctx.setVariable(name, std::move(value));
    VarNode* var = ctx.findVariable(name);
    if (var) {
        frame.bindSlot(name, var, var->getValueNode().getType());
        noteSlotKind(var->getValueNode().getType());
    }
}

void StackScope::updateVariable(const String& name, const Node& value) {
    if (frame.hasOwned(name)) {
        DataTypeFlags* vf = frame.getOwnedFlags(name);
        if (!vf) {
            throw VariableNotFoundError(name, "StackScope::updateVariable(owned-flags)");
        }

        if (vf->isConst) {
            throw MerkError("Cannot assign to const variable '" + name + "'");
        }

        auto& tsr = localTypes;
        const bool hasDeclared = (vf->declaredSig != kInvalidTypeSignatureId && vf->declaredSig != tsr.any());
        if (hasDeclared) {
            const TypeSignatureId rhsSig = tsr.inferFromValue(value);
            auto r = tsr.matchValue(vf->declaredSig, value);
            if (!r.ok) {
                throw MerkError(
                    "Type mismatch for '" + name + "': expected " +
                    tsr.toString(vf->declaredSig) + ", got " + tsr.toString(rhsSig)
                );
            }
        } else if (vf->inferredSig == kInvalidTypeSignatureId) {
            vf->inferredSig = tsr.inferFromValue(value);
        }

        Node stored = value;
        if (stored.getFlags().isMutable != vf->isMutable) {
            stored.getFlags().isMutable = vf->isMutable;
        }

        frame.updateOwnedValue(name, stored);
        noteSlotKind(stored.getType());
        return;
    }

    if (VarNode* var = findLocalVar(name)) {
        DataTypeFlags& vf = var->getVarFlags();

        if (vf.isConst) {
            throw MerkError("Cannot assign to const variable '" + name + "'");
        }

        auto& tsr = localTypes;
        const bool hasDeclared = (vf.declaredSig != kInvalidTypeSignatureId && vf.declaredSig != tsr.any());
        if (hasDeclared) {
            const TypeSignatureId rhsSig = tsr.inferFromValue(value);
            auto r = tsr.matchValue(vf.declaredSig, value);
            if (!r.ok) {
                throw MerkError(
                    "Type mismatch for '" + name + "': expected " +
                    tsr.toString(vf.declaredSig) + ", got " + tsr.toString(rhsSig)
                );
            }
        } else if (vf.inferredSig == kInvalidTypeSignatureId) {
            vf.inferredSig = tsr.inferFromValue(value);
        }

        Context& ctx = getContext();
        if (value.getFlags().isMutable == vf.isMutable) {
            ctx.updateVariable(name, value);
            frame.setSlotKind(name, value.getType());
            noteSlotKind(value.getType());
            return;
        }

        Node stored = value;
        stored.getFlags().isMutable = vf.isMutable;
        ctx.updateVariable(name, stored);
        frame.setSlotKind(name, stored.getType());
        noteSlotKind(stored.getType());
        return;
    }

    if (auto parent = getParent()) {
        parentFallbackUpdates.fetch_add(1, std::memory_order_relaxed);
        parent->updateVariable(name, value);
        return;
    }
    throw VariableNotFoundError(name, "StackScope::updateVariable");
}

VarNode& StackScope::getVariable(const String& name) {
    if (VarNode* local = findLocalVar(name)) {
        return *local;
    }
    parentFallbackLookups.fetch_add(1, std::memory_order_relaxed);

    for (Scope* s = this; s != nullptr;) {
        if (VarNode* variable = s->getContext().findVariable(name)) {
            return *variable;
        }
        auto parent = s->getParent();
        s = parent.get();
    }

    throw VariableNotFoundError(name, "StackScope::getVariable");
}

bool StackScope::hasVariable(const String& name) const {
    if (findLocalVar(name)) {
        return true;
    }
    parentFallbackLookups.fetch_add(1, std::memory_order_relaxed);

    for (const Scope* s = this; s != nullptr;) {
        if (s->getContext().hasVariable(name)) {
            return true;
        }
        auto parent = s->getParent();
        s = parent.get();
    }

    return false;
}

bool StackScope::tryReadInt(const String& name, int& out) const {
    if (frame.tryGetInt(name, out)) {
        return true;
    }

    if (auto parent = getParent()) {
        return parent->tryReadInt(name, out);
    }
    return false;
}

bool StackScope::tryWriteInt(const String& name, int value) {
    if (frame.hasOwned(name)) {
        DataTypeFlags* vf = frame.getOwnedFlags(name);
        if (!vf) {
            throw VariableNotFoundError(name, "StackScope::tryWriteInt(owned-flags)");
        }

        if (vf->isConst) {
            throw MerkError("Cannot assign to const variable '" + name + "'");
        }

        // Hot path: slot already Int and type stable (inferred or no declared type)
        const Frame::SlotKind kind = frame.getSlotKind(name);
        const bool typeStable = (vf->inferredSig != kInvalidTypeSignatureId) ||
            (vf->declaredSig == kInvalidTypeSignatureId || vf->declaredSig == localTypes.any());
        if (kind == Frame::SlotKind::Int && typeStable) {
            if (vf->inferredSig == kInvalidTypeSignatureId) {
                vf->inferredSig = localTypes.inferFromValue(Node(0));
            }
            frame.updateOwnedInt(name, value);
            noteSlotKind(NodeValueType::Int);
            return true;
        }

        auto& tsr = localTypes;
        const TypeSignatureId intSig = tsr.inferFromValue(Node(0));
        const bool hasDeclared = (vf->declaredSig != kInvalidTypeSignatureId && vf->declaredSig != tsr.any());
        if (hasDeclared) {
            if (vf->declaredSig != intSig) {
                Node rhs(value);
                const TypeSignatureId rhsSig = tsr.inferFromValue(rhs);
                auto r = tsr.matchValue(vf->declaredSig, rhs);
                if (!r.ok) {
                    throw MerkError(
                        "Type mismatch for '" + name + "': expected " +
                        tsr.toString(vf->declaredSig) + ", got " + tsr.toString(rhsSig)
                    );
                }
            }
        } else if (vf->inferredSig == kInvalidTypeSignatureId) {
            vf->inferredSig = intSig;
        }

        frame.updateOwnedInt(name, value);
        noteSlotKind(NodeValueType::Int);
        return true;
    }

    if (VarNode* var = findLocalVar(name)) {
        DataTypeFlags& vf = var->getVarFlags();
        if (vf.isConst) {
            throw MerkError("Cannot assign to const variable '" + name + "'");
        }

        auto& tsr = localTypes;
        const TypeSignatureId intSig = tsr.inferFromValue(Node(0));
        const bool hasDeclared = (vf.declaredSig != kInvalidTypeSignatureId && vf.declaredSig != tsr.any());
        if (hasDeclared) {
            if (vf.declaredSig != intSig) {
                Node rhs(value);
                const TypeSignatureId rhsSig = tsr.inferFromValue(rhs);
                auto r = tsr.matchValue(vf.declaredSig, rhs);
                if (!r.ok) {
                    throw MerkError(
                        "Type mismatch for '" + name + "': expected " +
                        tsr.toString(vf.declaredSig) + ", got " + tsr.toString(rhsSig)
                    );
                }
            }
        } else if (vf.inferredSig == kInvalidTypeSignatureId) {
            vf.inferredSig = intSig;
        }

        Context& ctx = getContext();
        Node rhs(value);
        if (rhs.getFlags().isMutable != vf.isMutable) {
            rhs.getFlags().isMutable = vf.isMutable;
        }
        ctx.updateVariable(name, rhs);
        frame.setSlotKind(name, NodeValueType::Int);
        noteSlotKind(NodeValueType::Int);
        return true;
    }

    if (auto parent = getParent()) {
        parentFallbackUpdates.fetch_add(1, std::memory_order_relaxed);
        return parent->tryWriteInt(name, value);
    }

    return false;
}

