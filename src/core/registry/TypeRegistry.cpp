#include "core/TypesFWD.hpp"
#include "core/types/Type.hpp"
#include "core/registry/TypeRegistry.hpp"
#include <algorithm>
#include <cassert>

// ------------------------------------------------------------
// Registry lifecycle
// ------------------------------------------------------------

TypeRegistry::TypeRegistry() {
    // Reserve slot 0 as invalid
    pool_.emplace_back(
        kInvalidTypeSignatureId,
        "<invalid>",
        nullptr
    );

    // Intern Any immediately
    auto anyId = any();
    bindName("Any", anyId);
}

TypeRegistry& TypeRegistry::global() {
    static TypeRegistry instance;
    return instance;
}

// ------------------------------------------------------------
// Interning helpers
// ------------------------------------------------------------

TypeSignatureId TypeRegistry::intern(
    const Key& key,
    SharedPtr<TypeBase> node,
    String displayName
) {
    auto it = intern_.find(key);
    if (it != intern_.end())
        return it->second;

    TypeSignatureId id = static_cast<TypeSignatureId>(pool_.size());
    pool_.emplace_back(id, std::move(displayName), std::move(node));
    intern_.emplace(key, id);
    return id;
}

// ------------------------------------------------------------
// Built-in type constructors
// ------------------------------------------------------------

TypeSignatureId TypeRegistry::any() {
    Key k;
    k.kind = TypeSigKind::Any;

    return intern(
        k,
        std::make_shared<AnyType>(),
        "Any"
    );
}

TypeSignatureId TypeRegistry::primitive(NodeValueType prim) {
    Key k;
    k.kind = TypeSigKind::Primitive;
    k.prim = prim;

    auto node = std::make_shared<PrimitiveType>(prim);
    return intern(k, node, node->describe(*this));
}

TypeSignatureId TypeRegistry::classType(const String& className) {
    Key k;
    k.kind = TypeSigKind::Class;
    k.name = className;

    auto node = std::make_shared<ClassType>(className);
    return intern(k, node, className);
}

TypeSignatureId TypeRegistry::methodType(MethodType m) {
    assert(m.params.size() == m.enforced.size());

    Key k;
    k.kind        = TypeSigKind::Method;
    k.name        = m.methodName;
    k.kids        = m.params;
    k.enforced    = m.enforced;
    k.ret         = m.ret;
    k.variadic    = m.variadic;
    k.retEnforced = m.retEnforced;

    auto node = std::make_shared<MethodType>(std::move(m));
    String display = node->describe(*this);

    return intern(k, node, std::move(display));
}

TypeSignatureId TypeRegistry::unite(Vector<TypeSignatureId> members) {
    // Normalize: remove duplicates, sort
    std::sort(members.begin(), members.end());
    members.erase(
        std::unique(members.begin(), members.end()),
        members.end()
    );

    // Collapse single-member union
    if (members.size() == 1)
        return members[0];

    Key k;
    k.kind = TypeSigKind::Union;
    k.kids = members;

    auto node = std::make_shared<UnionType>(members);
    return intern(k, node, node->describe(*this));
}

TypeSignatureId TypeRegistry::container(
    const String& base,
    Vector<TypeSignatureId> args
) {
    Key k;
    k.kind = TypeSigKind::Container;
    k.name = base;
    k.kids = args;

    auto node = std::make_shared<ContainerType>(base, args);
    return intern(k, node, node->describe(*this));
}

// ------------------------------------------------------------
// Lookup & naming
// ------------------------------------------------------------

const TypeSignature& TypeRegistry::get(TypeSignatureId id) const {
    assert(id < pool_.size());
    return pool_[id];
}

void TypeRegistry::bindName(const String& name, TypeSignatureId id) {
    nameToId_[name] = id;
}

std::optional<TypeSignatureId> TypeRegistry::lookupName(
    const String& name
) const {
    auto it = nameToId_.find(name);
    if (it == nameToId_.end())
        return std::nullopt;
    return it->second;
}

// ------------------------------------------------------------
// Annotation binding (Option C)
// ------------------------------------------------------------

TypeSignatureId TypeRegistry::bindResolvedType(
    const ResolvedType& rt,
    Scope& /*scope*/
) {
    // Base case: Any / empty
    if (rt.getBaseType().empty())
        return any();

    // Look up named type
    if (rt.getInnerType().empty()) {
        if (auto it = lookupName(rt.getBaseType()))
            return *it;

        // Unknown named type — treat as nominal class
        auto id = classType(rt.getBaseType());
        bindName(rt.getBaseType(), id);
        return id;
    }

    // Generic/container type
    Vector<TypeSignatureId> args;
    args.reserve(rt.getInnerType().size());
    for (const auto& inner : rt.getInnerType()) {
        args.push_back(bindResolvedType(inner, /*scope*/ *(Scope*)nullptr));
    }

    return container(rt.getBaseType(), std::move(args));
}

// ------------------------------------------------------------
// Matching dispatch
// ------------------------------------------------------------

TypeMatchResult TypeRegistry::matchValue(
    TypeSignatureId expected,
    const Node& v,
    const TypeMatchOptions& opt
) const {
    if (expected == kInvalidTypeSignatureId)
        return TypeMatchResult::No();

    const auto& sig = get(expected);
    if (!sig.type_)
        return TypeMatchResult::No();

    return sig.type_->matchValueInternal(v, *this, opt);
}

TypeMatchResult TypeRegistry::matchCall(
    TypeSignatureId sigId,
    const ArgumentList& args,
    const TypeMatchOptions& opt
) const {
    const auto& sig = get(sigId);
    if (!sig.type_ || sig.kind() != TypeSigKind::Method)
        return TypeMatchResult::No();

    return sig.type_->matchCallInternal(args, *this, opt);
}

// ------------------------------------------------------------
// Numeric helpers
// ------------------------------------------------------------

bool TypeRegistry::isNumeric(NodeValueType t) {
    switch (t) {
        case NodeValueType::Int:
        case NodeValueType::Float:
        case NodeValueType::Double:
            return true;
        default:
            return false;
    }
}

int TypeRegistry::numericRank(NodeValueType t) {
    switch (t) {
        case NodeValueType::Int:    return 1;
        case NodeValueType::Float:  return 2;
        case NodeValueType::Double: return 3;
        default:                    return 0;
    }
}

// ------------------------------------------------------------
// Stringification
// ------------------------------------------------------------

String TypeRegistry::toString(TypeSignatureId id) const {
    if (id == kInvalidTypeSignatureId)
        return "<invalid>";
    return get(id).name();
}

// ------------------------------------------------------------
// Key hashing & equality
// ------------------------------------------------------------

bool TypeRegistry::Key::operator==(const Key& o) const {
    return kind        == o.kind
        && prim        == o.prim
        && name        == o.name
        && kids        == o.kids
        && enforced    == o.enforced
        && ret         == o.ret
        && variadic    == o.variadic
        && retEnforced == o.retEnforced;
}

size_t TypeRegistry::KeyHash::operator()(const Key& k) const {
    size_t h = std::hash<int>()(static_cast<int>(k.kind));
    h ^= std::hash<int>()(static_cast<int>(k.prim)) + 0x9e3779b9 + (h<<6) + (h>>2);
    h ^= std::hash<String>()(k.name) + 0x9e3779b9 + (h<<6) + (h>>2);

    for (auto id : k.kids)
        h ^= std::hash<TypeSignatureId>()(id) + 0x9e3779b9 + (h<<6) + (h>>2);

    for (auto e : k.enforced)
        h ^= std::hash<uint8_t>()(e) + 0x9e3779b9 + (h<<6) + (h>>2);

    h ^= std::hash<TypeSignatureId>()(k.ret);
    h ^= std::hash<bool>()(k.variadic);
    h ^= std::hash<bool>()(k.retEnforced);
    return h;
}