// #include "core/registry/TypeSignatureRegistry.hpp"
// #include "core/registry/TypeRegistry.hpp"
// #include "core/errors.h"
// #include "core/node/Node.hpp"
// #include <algorithm>
// #include "core/callables/classes/ClassBase.hpp"
// #include "core/Scope.hpp"

// TypeSignatureRegistry::TypeSignatureRegistry() = default;
// void TypeSignatureRegistry::ensureInit() {
//     if (!types_) {throw MerkError("TypeSignatureRegistry not attached");}
//     if (anySigId_ != kInvalidTypeSignatureId) return;

//     // slot 0 invalid (only once)
//     if (pool_.empty()) {
//         pool_.emplace_back(kInvalidTypeSignatureId, "<invalid>", nullptr, kInvalidTypeId);
//     }

//     // Intern Any exactly once.
//     Key k;
//     k.kind = TypeSigKind::Any;

//     auto it = intern_.find(k);
//     if (it != intern_.end()) {
//         anySigId_ = it->second;
//     } else {
//         TypeSignatureId id = (TypeSignatureId)pool_.size();
//         pool_.emplace_back(id, "Any", std::make_shared<AnySig>(), types_->anyId());
//         intern_.emplace(k, id);
//         anySigId_ = id;
//     }

//     // Bind name "Any" (local alias). Parent lookup already exists anyway, but this is convenient.
//     nameToId_["Any"] = anySigId_;
// }


// TypeSignatureRegistry::TypeSignatureRegistry(TypeRegistry& types, TypeSignatureRegistry* parent) {
//     attach(types, parent);
// }

// void TypeSignatureRegistry::attach(TypeRegistry& types, TypeSignatureRegistry* parent) {
//     types_  = &types;
//     ensureInit();
// }

// TypeRegistry& TypeSignatureRegistry::types() {
//     if (!types_) throw MerkError("TypeSignatureRegistry not attached (types())");
//     return *types_;
// }
// const TypeRegistry& TypeSignatureRegistry::types() const {
//     if (!types_) throw MerkError("TypeSignatureRegistry not attached (types() const)");
//     return *types_;
// }

// bool TypeSignatureRegistry::isAttached() const { return types_ != nullptr; }  

// // --- intern ---
// TypeSignatureId TypeSignatureRegistry::intern(
//     const Key& key,
//     std::shared_ptr<TypeSigBase> node,
//     String displayName,
//     TypeId baseId
// ) {
//     ensureInit();

//     auto it = intern_.find(key);
//     if (it != intern_.end()) return it->second;

//     TypeSignatureId id = (TypeSignatureId)pool_.size();
//     pool_.emplace_back(id, std::move(displayName), std::move(node), baseId);
//     intern_.emplace(key, id);
//     return id;
// }


// // --- constructors ---
// TypeSignatureId TypeSignatureRegistry::any() {
//     ensureInit();
//     return anySigId_;
// }


// TypeSignatureId TypeSignatureRegistry::nominal(TypeId base) {
//     Key k; k.kind = TypeSigKind::Nominal; k.base = base;
//     return intern(k, std::make_shared<NominalSig>(base), types_->nameOf(base), base);
// }

// TypeSignatureId TypeSignatureRegistry::primitive(NodeValueType prim) {
//     return nominal(types_->primitiveId(prim));
// }

// TypeSignatureId TypeSignatureRegistry::classType(const String& className) {
//     return nominal(types_->getOrCreate(className));
// }

// TypeSignatureId TypeSignatureRegistry::container(const String& base, Vector<TypeSignatureId> args) {
//     return container(types_->getOrCreate(base), std::move(args));
// }

// TypeSignatureId TypeSignatureRegistry::container(TypeId base, Vector<TypeSignatureId> args) {
//     Key k; k.kind = TypeSigKind::Container; k.base = base; k.kids = args;
//     auto node = std::make_shared<ContainerSig>(base, args);
//     return intern(k, node, node->describe(*this), base);
// }

// TypeSignatureId TypeSignatureRegistry::unite(Vector<TypeSignatureId> members) {
//     std::sort(members.begin(), members.end());
//     members.erase(std::unique(members.begin(), members.end()), members.end());
//     if (members.size() == 1) return members[0];

//     Key k; k.kind = TypeSigKind::Union; k.kids = members;
//     auto node = std::make_shared<UnionSig>(members);
//     return intern(k, node, node->describe(*this), types_->lookupOrInvalid("Union"));
// }

// TypeSignatureId TypeSignatureRegistry::invocableType(InvocableSigType m) {
//     if (m.params.size() != m.enforced.size())
//         throw MerkError("invocableType: params/enforced mismatch");

//     Key k;
//     k.kind        = TypeSigKind::Invocable;
//     k.name        = m.methodName;
//     k.kids        = m.params;
//     k.enforced    = m.enforced;
//     k.ret         = m.ret;
//     k.variadic    = m.variadic;
//     k.retEnforced = m.retEnforced;

//     auto node = std::make_shared<InvocableSig>(std::move(m));
//     return intern(k, node, node->describe(*this), types_->lookupOrInvalid("Invocable"));
// }

// TypeSignatureId TypeSignatureRegistry::listOf(TypeSignatureId elem) {
//     return container(types_->listId(), Vector<TypeSignatureId>{elem});
// }
// TypeSignatureId TypeSignatureRegistry::dictOf(TypeSignatureId key, TypeSignatureId val) {
//     return container(types_->dictId(), Vector<TypeSignatureId>{key, val});
// }

// // --- lookup ---
// bool TypeSignatureRegistry::isValid(TypeSignatureId id) const {
//     return id != kInvalidTypeSignatureId && (size_t)id < pool_.size();
// }

// const TypeSignature& TypeSignatureRegistry::get(TypeSignatureId id) const {
//     // if (!isValid(id)) return pool_[0];
//     if (!isValid(id)) {
//         throw MerkError("Invalid TypeSignatureId in TypeSignatureRegistry::get()");
//     }
//     return pool_[id];
// }

// void TypeSignatureRegistry::bindName(const String& name, TypeSignatureId id) {
//     nameToId_[name] = id;
// }

// std::optional<TypeSignatureId> TypeSignatureRegistry::lookupName(const String& name) const {
//     auto it = nameToId_.find(name);
//     if (it != nameToId_.end()) return it->second;
//     return std::nullopt;
// }

// // --- match dispatch ---
// TypeMatchResult TypeSignatureRegistry::matchValue(
//     TypeSignatureId expected,
//     const Node& v,
//     const TypeMatchOptions& opt
// ) const {
//     if (expected == kInvalidTypeSignatureId) return TypeMatchResult::No();
//     const auto& sig = get(expected);
//     if (!sig.type_) return TypeMatchResult::No();
//     return sig.type_->matchValue(v, *this, opt);
// }

// TypeMatchResult TypeSignatureRegistry::matchCall(
//     TypeSignatureId invSig,
//     const ArgumentList& args,
//     const TypeMatchOptions& opt
// ) const {
//     const auto& sig = get(invSig);
//     if (!sig.type_ || sig.kind() != TypeSigKind::Invocable) return TypeMatchResult::No();
//     return sig.type_->matchCall(args, *this, opt);
// }

// String TypeSignatureRegistry::toString(TypeSignatureId id) const {
//     if (!isValid(id)) {return "<invalid>";}
//     if (id == kInvalidTypeSignatureId) return "<invalid>";

//     // pool_ holds TypeSignature objects directly
//     if (id >= pool_.size()) return "<invalid>";

//     // name_ can be empty; still safe
//     const auto& sig = pool_[id];
//     if (sig.id() != id) return "<invalid>"; // optional sanity check

//     return sig.toString();
// }


// // --- Key eq/hash ---
// bool TypeSignatureRegistry::Key::operator==(const Key& o) const {
//     return kind == o.kind
//         && base == o.base
//         && kids == o.kids
//         && enforced == o.enforced
//         && ret == o.ret
//         && variadic == o.variadic
//         && retEnforced == o.retEnforced
//         && name == o.name;
// }

// size_t TypeSignatureRegistry::KeyHash::operator()(const Key& k) const {
//     size_t h = std::hash<int>()((int)k.kind);
//     h ^= std::hash<TypeId>()(k.base) + 0x9e3779b9 + (h<<6) + (h>>2);
//     h ^= std::hash<String>()(k.name) + 0x9e3779b9 + (h<<6) + (h>>2);

//     for (auto id : k.kids)
//         h ^= std::hash<TypeSignatureId>()(id) + 0x9e3779b9 + (h<<6) + (h>>2);

//     for (auto e : k.enforced)
//         h ^= std::hash<uint8_t>()(e) + 0x9e3779b9 + (h<<6) + (h>>2);

//     h ^= std::hash<TypeSignatureId>()(k.ret);
//     h ^= std::hash<bool>()(k.variadic);
//     h ^= std::hash<bool>()(k.retEnforced);
//     return h;
// }

// // --- bindResolvedType: identical behavior, but uses sig registry + TypeIds ---
// TypeSignatureId TypeSignatureRegistry::bindResolvedType(const ResolvedType& rt, Scope& scope) {
//     ensureInit();
//     const String base = rt.getBaseType();
//     const auto inner = rt.getInnerType();

//     if (inner.empty()) {
//         if (auto id = scope.lookupTypeSigName(base)) return *id;

//         // otherwise interpret base as nominal
//         TypeId tid = types().lookupOrInvalid(base);
//         if (tid == kInvalidTypeId) tid = types().getOrCreate(base);
//         return nominal(tid);
//     }

//     Vector<TypeSignatureId> kids;
//     kids.reserve(inner.size());
//     for (const auto& child : inner)
//         kids.push_back(bindResolvedType(child, scope));

//     return container(base, std::move(kids));
// }


// // --- inferFromValue: now produces signatures using TypeId ---
// TypeSignatureId TypeSignatureRegistry::inferFromValue(const Node& v) {
//     if (v.isNull()) return any();

//     switch (v.getType()) {
//         case NodeValueType::Bool:
//         case NodeValueType::Int:
//         case NodeValueType::Float:
//         case NodeValueType::Double:
//         case NodeValueType::String:
//         case NodeValueType::Char:
//             return primitive(v.getType());
//         default:
//             break;
//     }

//     const auto base = v.getFlags().fullType.getBaseType();
//     if (v.isInstance()) {
        
//         if (!base.empty() && base != "Any") {
//             return classType(base);
//         }
//         return classType(v.toInstance()->getName());
//     }

//     if (v.isList()) return container("List", { any() });
//     if (v.isDict()) return container("Dict", { any(), any() });
//     if (v.isSet())  return container("Set", { any() });


//     if (base == "File") return container("List", {});
//     if (base == "Http") return container("Http", {});
//     return any();
// }





#include "core/registry/TypeSignatureRegistry.hpp"

#include "core/registry/TypeRegistry.hpp"
#include "core/Scope.hpp"   // if bindResolvedType needs it (it does)
#include "core/node/Node.hpp"
#include "core/TypesFWD.hpp"

// ----------------------------
// helpers
// ----------------------------
void TypeSignatureRegistry::ensureAttached() const {
    if (!mgr_) throw MerkError("TypeSignatureRegistry not attached");
}

// ----------------------------
// scope-local name bindings
// ----------------------------
void TypeSignatureRegistry::bindName(const String& name, TypeSignatureId id) {
    if (name.empty()) return;
    nameToId_[name] = id;
}

std::optional<TypeSignatureId> TypeSignatureRegistry::lookupName(const String& name) const {
    auto it = nameToId_.find(name);
    if (it != nameToId_.end()) return it->second;
    return std::nullopt;
}

// ----------------------------
// forwarding factories
// ----------------------------
TypeSignatureId TypeSignatureRegistry::any() {
    ensureAttached();
    return mgr_->any();
}

TypeSignatureId TypeSignatureRegistry::primitive(NodeValueType t) {
    ensureAttached();
    return mgr_->primitive(t);
}

TypeSignatureId TypeSignatureRegistry::classType(const String& name) {
    ensureAttached();
    return mgr_->classType(name);
}

TypeSignatureId TypeSignatureRegistry::nominal(TypeId base) {
    ensureAttached();
    return mgr_->nominal(base);
}

TypeSignatureId TypeSignatureRegistry::container(TypeId base, Vector<TypeSignatureId> args) {
    ensureAttached();
    return mgr_->container(base, std::move(args));
}

TypeSignatureId TypeSignatureRegistry::container(const String& baseName, Vector<TypeSignatureId> args) {
    ensureAttached();
    TypeId base = mgr_->types().getOrCreate(baseName);
    return mgr_->container(base, std::move(args));
}

TypeSignatureId TypeSignatureRegistry::listOf(TypeSignatureId elem) {
    ensureAttached();
    return mgr_->container(mgr_->types().listId(), { elem });
}

TypeSignatureId TypeSignatureRegistry::dictOf(TypeSignatureId key, TypeSignatureId val) {
    ensureAttached();
    return mgr_->container(mgr_->types().dictId(), { key, val });
}

TypeSignatureId TypeSignatureRegistry::unite(Vector<TypeSignatureId> members) {
    ensureAttached();
    return mgr_->unite(std::move(members));
}

TypeSignatureId TypeSignatureRegistry::invocableType(InvocableSigType m) {
    ensureAttached();
    return mgr_->invocableType(std::move(m));
}

// ----------------------------
// access / stringify
// ----------------------------
const TypeSignature& TypeSignatureRegistry::get(TypeSignatureId id) const {
    ensureAttached();
    return mgr_->get(id);
}

bool TypeSignatureRegistry::isValid(TypeSignatureId id) const {
    return mgr_ && mgr_->isValid(id);
}

String TypeSignatureRegistry::toString(TypeSignatureId id) const {
    ensureAttached();
    return mgr_->toString(id);
}

// ----------------------------
// infer / bind
// ----------------------------
TypeSignatureId TypeSignatureRegistry::inferFromValue(const Node& v) {
    ensureAttached();
    return mgr_->inferFromValue(v);
}

TypeSignatureId TypeSignatureRegistry::bindResolvedType(const ResolvedType& rt, Scope& scope) {
    ensureAttached();
    return mgr_->bindResolvedType(rt, scope);
}

// ----------------------------
// matching
// ----------------------------
TypeMatchResult TypeSignatureRegistry::matchValue(
    TypeSignatureId expected,
    const Node& v,
    const TypeMatchOptions& opt
) const {
    ensureAttached();
    return mgr_->matchValue(expected, v, opt);
}

TypeMatchResult TypeSignatureRegistry::matchCall(
    TypeSignatureId invSig,
    const ArgumentList& args,
    const TypeMatchOptions& opt
) const {
    ensureAttached();
    return mgr_->matchCall(invSig, args, opt);
}

// ----------------------------
// types
// ----------------------------
TypeRegistry& TypeSignatureRegistry::types() {
    ensureAttached();
    return mgr_->types();
}

const TypeRegistry& TypeSignatureRegistry::types() const {
    ensureAttached();
    return mgr_->types();
}

