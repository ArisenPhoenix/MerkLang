
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/TypesFWD.hpp"   // String, Vector, SharedPtr, NodeValueType, ResolvedType, ArgumentList

class Node;
class Scope;

// --------------------
// Basic definitions
// --------------------

using TypeSignatureId = uint32_t;
static constexpr TypeSignatureId kInvalidTypeSignatureId = 0;

enum class TypeSigKind : uint8_t {
    Any,
    Primitive,
    Class,
    Method,
    Union,
    Container,
    Interface
};

struct TypeMatchOptions {
    bool allowAny = true;
    bool allowNumericWidening = true;
    bool allowDuck = false;
};

struct TypeMatchResult {
    bool ok = false;
    int score = -1;
    int cost = 0;

    static TypeMatchResult No();
    static TypeMatchResult Yes(int score_, int cost_ = 0);
};

class TypeRegistry;

// --------------------
// Type hierarchy
// --------------------

class TypeBase {
    friend class TypeRegistry;

public:
    virtual ~TypeBase() = default;
    virtual TypeSigKind kind() const = 0;
    virtual String describe(const TypeRegistry& reg) const = 0;

protected:
    virtual TypeMatchResult matchValueInternal(
        const Node&,
        const TypeRegistry&,
        const TypeMatchOptions&
    ) const;

    virtual TypeMatchResult matchCallInternal(
        const ArgumentList&,
        const TypeRegistry&,
        const TypeMatchOptions&
    ) const;
};

class AnyType final : public TypeBase {
public:
    TypeSigKind kind() const override;
    String describe(const TypeRegistry&) const override;

protected:
    TypeMatchResult matchValueInternal(
        const Node&,
        const TypeRegistry&,
        const TypeMatchOptions&
    ) const override;
};

class PrimitiveType final : public TypeBase {
public:
    NodeValueType prim;
    explicit PrimitiveType(NodeValueType p);

    TypeSigKind kind() const override;
    String describe(const TypeRegistry&) const override;

protected:
    TypeMatchResult matchValueInternal(
        const Node&,
        const TypeRegistry&,
        const TypeMatchOptions&
    ) const override;
};

class ClassType final : public TypeBase {
public:
    String className;
    explicit ClassType(String n);

    TypeSigKind kind() const override;
    String describe(const TypeRegistry&) const override;

protected:
    TypeMatchResult matchValueInternal(
        const Node&,
        const TypeRegistry&,
        const TypeMatchOptions&
    ) const override;
};

class MethodType final : public TypeBase {
public:
    String methodName;
    Vector<TypeSignatureId> params;
    Vector<uint8_t> enforced;
    TypeSignatureId ret = kInvalidTypeSignatureId;
    bool retEnforced = false;
    bool variadic = false;

    TypeSigKind kind() const override;
    String describe(const TypeRegistry& reg) const override;

protected:
    TypeMatchResult matchCallInternal(
        const ArgumentList&,
        const TypeRegistry&,
        const TypeMatchOptions&
    ) const override;
};

class UnionType final : public TypeBase {
public:
    Vector<TypeSignatureId> members;
    explicit UnionType(Vector<TypeSignatureId> m);

    TypeSigKind kind() const override;
    String describe(const TypeRegistry& reg) const override;

protected:
    TypeMatchResult matchValueInternal(
        const Node&,
        const TypeRegistry&,
        const TypeMatchOptions&
    ) const override;
};

class ContainerType final : public TypeBase {
public:
    String base;
    Vector<TypeSignatureId> args;

    ContainerType(String b, Vector<TypeSignatureId> a);

    TypeSigKind kind() const override;
    String describe(const TypeRegistry& reg) const override;

protected:
    TypeMatchResult matchValueInternal(
        const Node&,
        const TypeRegistry&,
        const TypeMatchOptions&
    ) const override;
};


