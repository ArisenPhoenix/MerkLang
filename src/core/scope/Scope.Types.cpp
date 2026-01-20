#include "core/Scope.hpp"
#include "core/registry/TypeRegistry.hpp"
#include "core/node/ArgumentNode.hpp"
#include "core/types/Typer.hpp"

void Scope::initRootTypes() {
    if (!isRoot) throw MerkError("initRootTypes called on non-root scope");

    if (!globalTypes) throw MerkError("initRootTypes: globalTypes is null");

    if (!globalTypeSigs) {
        globalTypeSigs = makeShared<TypeSignatureRegistryManager>(*globalTypes);
    }

    localTypes.attach(*globalTypeSigs);

    // for (TypeId id = 1; id < globalTypes-> size(); ++id) {
    //     const String& n = globalTypes->nameOf(id);
    //     if (!n.empty() && n != "<invalid>") {
    //         localTypes.bindName(n, localTypes.nominal(id));
    //     }
    // }
}


TypeSignatureId Scope::resolveTypeNameSig(const String& name) {
    if (!localTypes.isAttached()) localTypes.attach(*globalTypeSigs);

    // 1) local alias / shadowed name
    if (auto a = localTypes.lookupName(name)) return *a;

    // 2) nominal global
    TypeId tid = globalTypes->lookupOrInvalid(name);
    if (tid != kInvalidTypeId) return globalTypeSigs->nominal(tid);

    // 3) optional: treat unknown as new nominal (forward declare)
    tid = globalTypes->getOrCreate(name);
    return globalTypeSigs->nominal(tid);
}

void Scope::bindTypeAlias(const String& alias, TypeSignatureId sig) {
    if (!localTypes.isAttached()) localTypes.attach(*globalTypeSigs);
    localTypes.bindName(alias, sig);
}


TypeSignatureId Scope::bindResolvedType(const ResolvedType& rt, const String& aliasName) {
    if (!localTypes.isAttached()) localTypes.attach(*globalTypeSigs);

    TypeSignatureId id = globalTypeSigs->bindResolvedType(rt, *this);

    if (!aliasName.empty()) {
        localTypes.bindName(aliasName, id); // alias is scope-local
    }
    return id;
}




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
            c.match = TypeMatchResult::Yes(0, 999);
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
            // throw MerkError("No Signature Is Present");
            // Bind param annotations to TypeSignatureIds
            params.bindTypes(reg, *this);

            InvocableSigType m;
            m.name = name;
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



void Scope::registerFunction(const String& name, SharedPtr<CallableSignature> signature) {
    DEBUG_FLOW(FlowLevel::MED);
    // localFunctions.registerFunction(name, signature);
    if (!signature) throw MerkError("registerFunction: null signature for " + name);

    // Ensure TSR is attached (defensive)
    if (!localTypes.isAttached()) linkTypes();

    // Only FUNCTION overloads need invocable typing for resolution;
    // DEF can be left alone if you want, but it doesn’t hurt to build either.
    if (signature->getSubType() == CallableType::FUNCTION || signature->getSubType() == CallableType::NATIVE) {
        Typer::ensureInvocableSig(*signature, *this);
    }

    localFunctions.registerFunction(name, signature);
    DEBUG_FLOW_EXIT();
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