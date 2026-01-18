#include "core/registry/TypeSignatureRegistryManager.hpp"
#include "core/registry/TypeRegistry.hpp"
#include "core/errors.h"
#include "core/node/Node.hpp"
#include "core/Scope.hpp"

#include <algorithm>

// ----------------------------
// Key eq/hash
// ----------------------------
bool TypeSignatureRegistryManager::Key::operator==(const Key& o) const {
    return kind == o.kind
        && base == o.base
        && kids == o.kids
        && enforced == o.enforced
        && ret == o.ret
        && variadic == o.variadic
        && retEnforced == o.retEnforced
        && name == o.name;
}

size_t TypeSignatureRegistryManager::KeyHash::operator()(const Key& k) const {
    size_t h = std::hash<int>()((int)k.kind);
    h ^= std::hash<TypeId>()(k.base) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<String>()(k.name) + 0x9e3779b9 + (h << 6) + (h >> 2);

    for (auto id : k.kids)
        h ^= std::hash<TypeSignatureId>()(id) + 0x9e3779b9 + (h << 6) + (h >> 2);

    for (auto e : k.enforced)
        h ^= std::hash<uint8_t>()(e) + 0x9e3779b9 + (h << 6) + (h >> 2);

    h ^= std::hash<TypeSignatureId>()(k.ret) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<bool>()(k.variadic) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<bool>()(k.retEnforced) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
}

// ----------------------------
// ctor / init
// ----------------------------
TypeSignatureRegistryManager::TypeSignatureRegistryManager(TypeRegistry& types)
    : types_(&types)
{
    ensureInit();
}

void TypeSignatureRegistryManager::ensureInit() {
    if (!types_) throw MerkError("TypeSignatureRegistryManager: types_ is null");
    if (anySigId_ != kInvalidTypeSignatureId) return;

    if (pool_.empty()) {
        pool_.emplace_back(kInvalidTypeSignatureId, "<invalid>", nullptr, kInvalidTypeId);
    }

    Key k;
    k.kind = TypeSigKind::Any;

    auto it = intern_.find(k);
    if (it != intern_.end()) {
        anySigId_ = it->second;
        return;
    }

    TypeSignatureId id = (TypeSignatureId)pool_.size();
    pool_.emplace_back(id, "Any", std::make_shared<AnySig>(), types_->anyId());
    intern_.emplace(k, id);
    anySigId_ = id;
}

// ----------------------------
// intern
// ----------------------------
TypeSignatureId TypeSignatureRegistryManager::intern(
    const Key& key,
    std::shared_ptr<TypeSigBase> node,
    String displayName,
    TypeId baseId
) {
    ensureInit();

    auto it = intern_.find(key);
    if (it != intern_.end()) return it->second;

    TypeSignatureId id = (TypeSignatureId)pool_.size();
    pool_.emplace_back(id, std::move(displayName), std::move(node), baseId);
    intern_.emplace(key, id);
    return id;
}

// ----------------------------
// factories
// ----------------------------
TypeSignatureId TypeSignatureRegistryManager::any() {
    ensureInit();
    return anySigId_;
}

TypeSignatureId TypeSignatureRegistryManager::nominal(TypeId base) {
    ensureInit();
    Key k; k.kind = TypeSigKind::Nominal; k.base = base;
    auto node = std::make_shared<NominalSig>(base);
    return intern(k, node, node->describe(*this), base);
}

TypeSignatureId TypeSignatureRegistryManager::primitive(NodeValueType prim) {
    ensureInit();
    return nominal(types_->primitiveId(prim));
}

TypeSignatureId TypeSignatureRegistryManager::classType(const String& className) {
    ensureInit();
    return nominal(types_->getOrCreate(className));
}

TypeSignatureId TypeSignatureRegistryManager::container(TypeId base, Vector<TypeSignatureId> args) {
    ensureInit();

    Key k; k.kind = TypeSigKind::Container; k.base = base; k.kids = args;

    auto node = std::make_shared<ContainerSig>(base, args);
    return intern(k, node, node->describe(*this), base);
}

TypeSignatureId TypeSignatureRegistryManager::unite(Vector<TypeSignatureId> members) {
    ensureInit();

    std::sort(members.begin(), members.end());
    members.erase(std::unique(members.begin(), members.end()), members.end());
    if (members.size() == 1) return members[0];

    Key k; k.kind = TypeSigKind::Union; k.kids = members;

    auto node = std::make_shared<UnionSig>(members);
    return intern(k, node, node->describe(*this), types_->lookupOrInvalid("Union"));
}

TypeSignatureId TypeSignatureRegistryManager::invocableType(InvocableSigType m) {
    ensureInit();

    if (m.params.size() != m.enforced.size())
        throw MerkError("invocableType: params/enforced mismatch");

    Key k;
    k.kind        = TypeSigKind::Invocable;
    k.name        = m.methodName;
    k.kids        = m.params;
    k.enforced    = m.enforced;
    k.ret         = m.ret;
    k.variadic    = m.variadic;
    k.retEnforced = m.retEnforced;

    auto node = std::make_shared<InvocableSig>(std::move(m));
    return intern(k, node, node->describe(*this), types_->lookupOrInvalid("Invocable"));
}

// ----------------------------
// accessors
// ----------------------------
bool TypeSignatureRegistryManager::isValid(TypeSignatureId id) const {
    return id != kInvalidTypeSignatureId && (size_t)id < pool_.size();
}

const TypeSignature& TypeSignatureRegistryManager::get(TypeSignatureId id) const {
    if (!isValid(id)) throw MerkError("Invalid TypeSignatureId in TypeSignatureRegistryManager::get()");
    return pool_[id];
}

// ----------------------------
// ITypeSigContext
// ----------------------------
const TypeRegistry& TypeSignatureRegistryManager::types() const { return *types_; }

String TypeSignatureRegistryManager::toString(TypeSignatureId id) const {
    if (!isValid(id)) return "<invalid>";
    const auto& sig = pool_[id];
    // If you prefer always-up-to-date printing:
    if (sig.type_) return sig.type_->describe(*this);
    return sig.toString();
}

TypeMatchResult TypeSignatureRegistryManager::matchValue(
    TypeSignatureId expected,
    const Node& v,
    const TypeMatchOptions& opt
) const {
    if (expected == kInvalidTypeSignatureId) return TypeMatchResult::No();
    const auto& sig = get(expected);
    if (!sig.type_) return TypeMatchResult::No();
    return sig.type_->matchValue(v, *this, opt);
}

TypeMatchResult TypeSignatureRegistryManager::matchCall(
    TypeSignatureId invSig,
    const ArgumentList& args,
    const TypeMatchOptions& opt
) const {
    const auto& sig = get(invSig);
    if (!sig.type_ || sig.kind() != TypeSigKind::Invocable) return TypeMatchResult::No();
    return sig.type_->matchCall(args, *this, opt);
}

// ----------------------------
// bindResolvedType / inferFromValue
// ----------------------------
TypeSignatureId TypeSignatureRegistryManager::bindResolvedType(const ResolvedType& rt, Scope& scope) {
    ensureInit();

    const String base = rt.getBaseType();
    const auto inner = rt.getInnerType();

    if (inner.empty()) {
        if (auto id = scope.lookupTypeSigName(base)) return *id;

        TypeId tid = types().lookupOrInvalid(base);
        if (tid == kInvalidTypeId) tid = types().getOrCreate(base);
        return nominal(tid);
    }

    Vector<TypeSignatureId> kids;
    kids.reserve(inner.size());
    for (const auto& child : inner)
        kids.push_back(bindResolvedType(child, scope));

    return container(types().getOrCreate(base), std::move(kids));
}

TypeSignatureId TypeSignatureRegistryManager::inferFromValue(const Node& v) {
    ensureInit();

    if (v.isNull()) return any();

    switch (v.getType()) {
        case NodeValueType::Bool:
        case NodeValueType::Int:
        case NodeValueType::Float:
        case NodeValueType::Double:
        case NodeValueType::String:
        case NodeValueType::Char:
            return primitive(v.getType());
        default:
            break;
    }

    const auto base = v.getFlags().fullType.getBaseType();

    if (v.isInstance()) {
        if (!base.empty() && base != "Any") return classType(base);
        return classType(v.toInstance()->getName());
    }

    if (v.isList()) return container(types().listId(), { any() });
    if (v.isDict()) return container(types().dictId(), { any(), any() });
    if (v.isSet())  return container(types().getOrCreate("Set"), { any() });

    if (base == "File") return container(types().getOrCreate("File"), {});
    if (base == "Http") return container(types().getOrCreate("Http"), {});
    return any();
}
