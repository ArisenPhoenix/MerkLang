#pragma once
#include <unordered_map>
#include <optional>

#include "core/TypesFWD.hpp"
#include "core/types/TypeIds.hpp"
#include "core/types/TypeSig.hpp"

// forward decls
class Node;
class Scope;
class TypeRegistry;
class ResolvedType;
class TypeSigBase;

struct ITypeSigContext {
    virtual ~ITypeSigContext() = default;

    virtual const TypeRegistry& types() const = 0;
    virtual String toString(TypeSignatureId id) const = 0;

    virtual TypeMatchResult matchValue(
        TypeSignatureId expected,
        const Node& v,
        const TypeMatchOptions& opt
    ) const = 0;
};

class TypeSignature {
public:
    TypeSignatureId id() const { return id_; }
    TypeSigKind kind() const { return type_ ? type_->kind() : TypeSigKind::Any; }
    const String& name() const { return name_; }
    String toString() const { return name_; }
    TypeId baseId() const { return baseId_; }

    TypeSignature(TypeSignatureId id, String name, std::shared_ptr<TypeSigBase> t, TypeId baseId)
        : id_(id), name_(std::move(name)), type_(std::move(t)), baseId_(baseId) {}

private:
    friend class TypeSignatureRegistryManager;
    friend class TypeSignatureRegistry;
    TypeSignatureId id_ = kInvalidTypeSignatureId;
    String name_;
    std::shared_ptr<TypeSigBase> type_;
    TypeId baseId_ = kInvalidTypeId;
};

class TypeSignatureRegistryManager : public ITypeSigContext {
public:
    explicit TypeSignatureRegistryManager(TypeRegistry& types);

    // construction/interning
    TypeSignatureId any();
    TypeSignatureId primitive(NodeValueType);
    TypeSignatureId classType(const String&);
    TypeSignatureId nominal(TypeId);
    TypeSignatureId container(TypeId, Vector<TypeSignatureId>);
    TypeSignatureId unite(Vector<TypeSignatureId>);
    TypeSignatureId invocableType(InvocableSigType);

    // access
    const TypeSignature& get(TypeSignatureId) const;
    bool isValid(TypeSignatureId) const;

    // “registry-like”
    TypeSignatureId inferFromValue(const Node& v);
    TypeSignatureId bindResolvedType(const ResolvedType&, Scope&);
    TypeMatchResult matchCall(TypeSignatureId, const ArgumentList&, const TypeMatchOptions& = {}) const;

    // ITypeSigContext
    const TypeRegistry& types() const override;
    String toString(TypeSignatureId id) const override;
    TypeMatchResult matchValue(TypeSignatureId expected, const Node& v, const TypeMatchOptions& opt) const override;

    // non-const convenience
    TypeRegistry& types() { return *types_; }

private:
    void ensureInit();

    struct Key {
        TypeSigKind kind = TypeSigKind::Any;
        TypeId base = kInvalidTypeId;
        Vector<TypeSignatureId> kids;
        Vector<uint8_t> enforced;
        TypeSignatureId ret = kInvalidTypeSignatureId;
        bool variadic = false;
        bool retEnforced = false;
        String name;

        bool operator==(const Key& o) const;
    };

    struct KeyHash {
        size_t operator()(const Key& k) const;
    };

    TypeSignatureId intern(const Key& key, std::shared_ptr<TypeSigBase> node, String displayName, TypeId baseId);

private:
    TypeRegistry* types_ = nullptr;
    TypeSignatureId anySigId_ = kInvalidTypeSignatureId;

    Vector<TypeSignature> pool_;
    std::unordered_map<Key, TypeSignatureId, KeyHash> intern_;
};
