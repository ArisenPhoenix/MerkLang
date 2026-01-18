// #pragma once
// #include <unordered_map>
// #include <optional>

// #include "core/types/TypeIds.hpp"
// #include "core/types/TypeSig.hpp"
// #include "core/TypesFWD.hpp"

// class Node;
// class Scope;
// class TypeRegistry;
// class ResolvedType;

// class TypeSignature {
// public:
//     TypeSignatureId id() const { return id_; }
//     TypeSigKind kind() const { return type_ ? type_->kind() : TypeSigKind::Any; }
//     const String& name() const { return name_; }
//     String toString() const { return name_; }
//     TypeId baseId() const { return baseId_; }

//     TypeSignature(TypeSignatureId id, String name, std::shared_ptr<TypeSigBase> t, TypeId baseId)
//         : id_(id), name_(std::move(name)), type_(std::move(t)), baseId_(baseId) {}

// private:
//     friend class TypeSignatureRegistry;
//     TypeSignatureId id_ = kInvalidTypeSignatureId;
//     String name_;
//     std::shared_ptr<TypeSigBase> type_;
//     TypeId baseId_ = kInvalidTypeId;
// };

// class TypeSignatureRegistry {
// public:
//     explicit TypeSignatureRegistry(TypeRegistry& types, TypeSignatureRegistry* parent = nullptr);

//     TypeRegistry& types() { return types_; }
//     const TypeRegistry& types() const { return types_; }

//     // same-ish API
//     TypeSignatureId any();
//     TypeSignatureId primitive(NodeValueType prim);                // becomes nominal(primitiveId)
//     TypeSignatureId classType(const String& className);           // becomes nominal(getOrCreate)
//     TypeSignatureId nominal(TypeId base);

//     TypeSignatureId container(const String& base, Vector<TypeSignatureId> args); // base name -> TypeId
//     TypeSignatureId container(TypeId base, Vector<TypeSignatureId> args);        // direct
//     TypeSignatureId listOf(TypeSignatureId elem);
//     TypeSignatureId dictOf(TypeSignatureId key, TypeSignatureId val);

//     TypeSignatureId unite(Vector<TypeSignatureId> members);

//     TypeSignatureId invocableType(InvocableSigData m);

//     const TypeSignature& get(TypeSignatureId id) const;

//     void bindName(const String& name, TypeSignatureId id);
//     std::optional<TypeSignatureId> lookupName(const String& name) const;

//     TypeSignatureId bindResolvedType(const ResolvedType& rt, Scope& scope);

//     TypeMatchResult matchValue(TypeSignatureId expected, const Node& v, const TypeMatchOptions& opt = {}) const;
//     TypeMatchResult matchCall(TypeSignatureId invSig, const ArgumentList& args, const TypeMatchOptions& opt = {}) const;

//     String toString(TypeSignatureId id) const;

//     bool isValid(TypeSignatureId id) const;

//     TypeSignatureId inferFromValue(const Node& v);

// private:
//     TypeRegistry& types_;
//     TypeSignatureRegistry* parent_ = nullptr;

//     struct Key {
//         TypeSigKind kind = TypeSigKind::Any;
//         TypeId base = kInvalidTypeId;                // for Nominal/Container
//         Vector<TypeSignatureId> kids;                // container args, union members, invocable params
//         Vector<uint8_t> enforced;                    // invocable
//         TypeSignatureId ret = kInvalidTypeSignatureId;
//         bool variadic = false;
//         bool retEnforced = false;
//         String name;                                 // invocable methodName (optional)

//         bool operator==(const Key& o) const;
//     };

//     struct KeyHash {
//         size_t operator()(const Key& k) const;
//     };

//     TypeSignatureId intern(const Key& key, std::shared_ptr<TypeSigBase> node, String displayName, TypeId baseId);

//     Vector<TypeSignature> pool_;
//     std::unordered_map<Key, TypeSignatureId, KeyHash> intern_;
//     std::unordered_map<String, TypeSignatureId> nameToId_;
// };



// #pragma once
// #include <unordered_map>
// #include <optional>

// #include "core/types/TypeIds.hpp"
// #include "core/types/TypeSig.hpp"
// #include "core/TypesFWD.hpp"
// #include "core/registry/TypeSignatureRegistryManager.hpp"
// class Node;
// class Scope;
// class TypeRegistry;
// class ResolvedType;



// class TypeSignatureRegistry {
// public:
//     TypeSignatureRegistry();

//     // Optional convenience ctor (still allowed)
//     explicit TypeSignatureRegistry(TypeRegistry& types, TypeSignatureRegistry* parent = nullptr);

//     void attach(TypeRegistry& types, TypeSignatureRegistry* parent = nullptr);

//     TypeRegistry& types();
//     const TypeRegistry& types() const;

//     bool isAttached() const;

//     // same-ish API
//     TypeSignatureId any();
//     TypeSignatureId primitive(NodeValueType prim);
//     TypeSignatureId classType(const String& className);
//     TypeSignatureId nominal(TypeId base);

//     TypeSignatureId container(const String& base, Vector<TypeSignatureId> args);
//     TypeSignatureId container(TypeId base, Vector<TypeSignatureId> args);
//     TypeSignatureId listOf(TypeSignatureId elem);
//     TypeSignatureId dictOf(TypeSignatureId key, TypeSignatureId val);

//     TypeSignatureId unite(Vector<TypeSignatureId> members);

//     TypeSignatureId invocableType(InvocableSigType m);

//     const TypeSignature& get(TypeSignatureId id) const;

//     void bindName(const String& name, TypeSignatureId id);
//     std::optional<TypeSignatureId> lookupName(const String& name) const;

//     TypeSignatureId bindResolvedType(const ResolvedType& rt, Scope& scope);

//     TypeMatchResult matchValue(TypeSignatureId expected, const Node& v, const TypeMatchOptions& opt = {}) const;
//     TypeMatchResult matchCall(TypeSignatureId invSig, const ArgumentList& args, const TypeMatchOptions& opt = {}) const;

//     String toString(TypeSignatureId id) const;

//     bool isValid(TypeSignatureId id) const;

//     TypeSignatureId inferFromValue(const Node& v);
// private:
//     TypeRegistry* types_ = nullptr;
//     // TypeSignatureRegistry* parent_ = nullptr;
//     TypeSignatureId anySigId_ = kInvalidTypeSignatureId;
//     void ensureInit();

//     struct Key {
//         TypeSigKind kind = TypeSigKind::Any;
//         TypeId base = kInvalidTypeId;
//         Vector<TypeSignatureId> kids;
//         Vector<uint8_t> enforced;
//         TypeSignatureId ret = kInvalidTypeSignatureId;
//         bool variadic = false;
//         bool retEnforced = false;
//         String name;

//         bool operator==(const Key& o) const;
//     };

//     struct KeyHash {
//         size_t operator()(const Key& k) const;
//     };

//     TypeSignatureId intern(const Key& key, std::shared_ptr<TypeSigBase> node, String displayName, TypeId baseId);

//     Vector<TypeSignature> pool_;
//     std::unordered_map<Key, TypeSignatureId, KeyHash> intern_;
//     std::unordered_map<String, TypeSignatureId> nameToId_;
// };

// #pragma once
// #include <unordered_map>
// #include <optional>

// #include "core/TypesFWD.hpp"
// #include "core/registry/TypeSignatureRegistryManager.hpp"

// class TypeSignatureRegistry {
// public:
//     TypeSignatureRegistry() = default;

//     void attach(TypeSignatureRegistryManager& mgr) { mgr_ = &mgr; }

//     bool isAttached() const { return mgr_ != nullptr; }
//     void ensureAttached() const {
//         if (!mgr_) throw MerkError("TypeSignatureRegistry not attached");
//     }

//     // scope-local
//     void bindName(const String& name, TypeSignatureId id) { nameToId_[name] = id; }
//     std::optional<TypeSignatureId> lookupName(const String& name) const;

//     // forwarding (old-ish API)
//     TypeSignatureId any() { ensureAttached(); return mgr_->any(); }
//     TypeSignatureId primitive(NodeValueType t) { ensureAttached(); return mgr_->primitive(t); }
//     TypeSignatureId classType(const String& n) { ensureAttached(); return mgr_->classType(n); }
//     TypeSignatureId nominal(TypeId id) { ensureAttached(); return mgr_->nominal(id); }
//     TypeSignatureId container(TypeId base, Vector<TypeSignatureId> args) { ensureAttached(); return mgr_->container(base, std::move(args)); }
//     TypeSignatureId listOf(TypeSignatureId elem) { ensureAttached(); return mgr_->container(mgr_->types().listId(), { elem }); }
//     TypeSignatureId dictOf(TypeSignatureId k, TypeSignatureId v) { ensureAttached(); return mgr_->container(mgr_->types().dictId(), { k, v }); }
//     TypeSignatureId unite(Vector<TypeSignatureId> m) { ensureAttached(); return mgr_->unite(std::move(m)); }
//     TypeSignatureId invocableType(InvocableSigType m) { ensureAttached(); return mgr_->invocableType(std::move(m)); }

//     const TypeSignature& get(TypeSignatureId id) const { ensureAttached(); return mgr_->get(id); }
//     bool isValid(TypeSignatureId id) const { return mgr_ && mgr_->isValid(id); }

//     String toString(TypeSignatureId id) const { ensureAttached(); return mgr_->toString(id); }

//     TypeSignatureId inferFromValue(const Node& v) { ensureAttached(); return mgr_->inferFromValue(v); }
//     TypeSignatureId bindResolvedType(const ResolvedType& rt, Scope& s) { ensureAttached(); return mgr_->bindResolvedType(rt, s); }

//     TypeMatchResult matchValue(TypeSignatureId e, const Node& v, const TypeMatchOptions& opt = {}) const {
//         ensureAttached();
//         return mgr_->matchValue(e, v, opt);
//     }
//     TypeMatchResult matchCall(TypeSignatureId invSig, const ArgumentList& args, const TypeMatchOptions& opt = {}) const {
//         ensureAttached();
//         return mgr_->matchCall(invSig, args, opt);
//     }

//     TypeRegistry& types() { ensureAttached(); return mgr_->types(); }
//     const TypeRegistry& types() const { ensureAttached(); return mgr_->types(); }

// private:
//     TypeSignatureRegistryManager* mgr_ = nullptr;
//     std::unordered_map<String, TypeSignatureId> nameToId_;
// };


#pragma once
#include <unordered_map>
#include <optional>

#include "core/TypesFWD.hpp"
#include "core/errors.h" // MerkError
#include "core/registry/TypeSignatureRegistryManager.hpp"

class Node;
class Scope;
class ResolvedType;

class TypeSignatureRegistry {
public:
    TypeSignatureRegistry() = default;

    // Attach to the global owner (stable ids + intern pool)
    void attach(TypeSignatureRegistryManager& mgr) { mgr_ = &mgr; }

    bool isAttached() const { return mgr_ != nullptr; }

    // Scope-local aliasing
    void bindName(const String& name, TypeSignatureId id);
    std::optional<TypeSignatureId> lookupName(const String& name) const;

    // --- Forwarding API (old-ish shape) ---
    TypeSignatureId any();
    TypeSignatureId primitive(NodeValueType t);
    TypeSignatureId classType(const String& name);
    TypeSignatureId nominal(TypeId base);

    TypeSignatureId container(TypeId base, Vector<TypeSignatureId> args);
    TypeSignatureId container(const String& baseName, Vector<TypeSignatureId> args);

    TypeSignatureId listOf(TypeSignatureId elem);
    TypeSignatureId dictOf(TypeSignatureId key, TypeSignatureId val);

    TypeSignatureId unite(Vector<TypeSignatureId> members);
    TypeSignatureId invocableType(InvocableSigType m);

    const TypeSignature& get(TypeSignatureId id) const;
    bool isValid(TypeSignatureId id) const;

    String toString(TypeSignatureId id) const;

    TypeSignatureId inferFromValue(const Node& v);
    TypeSignatureId bindResolvedType(const ResolvedType& rt, Scope& scope);

    TypeMatchResult matchValue(TypeSignatureId expected, const Node& v, const TypeMatchOptions& opt = {}) const;
    TypeMatchResult matchCall(TypeSignatureId invSig, const ArgumentList& args, const TypeMatchOptions& opt = {}) const;

    TypeRegistry& types();
    const TypeRegistry& types() const;

private:
    void ensureAttached() const;

private:
    TypeSignatureRegistryManager* mgr_ = nullptr;
    std::unordered_map<String, TypeSignatureId> nameToId_;
};



