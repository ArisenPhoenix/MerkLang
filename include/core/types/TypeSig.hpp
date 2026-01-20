#pragma once
#include <memory>
#include <vector>

#include "core/types/TypeIds.hpp"
#include "core/TypesFWD.hpp"

class Node;
class ITypeSigContext;

struct InvocableSigType {
    String name;
    Vector<TypeSignatureId> params;
    Vector<uint8_t> enforced;
    TypeSignatureId ret = kInvalidTypeSignatureId;
    bool retEnforced = false;
    bool variadic = false;
};

class TypeSigBase {
public:
    virtual ~TypeSigBase() = default;
    virtual TypeSigKind kind() const = 0;
    virtual String describe(const ITypeSigContext& ctx) const = 0;

    virtual TypeMatchResult matchValue(
        const Node& v,
        const ITypeSigContext& ctx,
        const TypeMatchOptions& opt
    ) const { (void)v; (void)ctx; (void)opt; return TypeMatchResult::No(); }

    virtual TypeMatchResult matchCall(
        const ArgumentList& args,
        const ITypeSigContext& ctx,
        const TypeMatchOptions& opt
    ) const { (void)args; (void)ctx; (void)opt; return TypeMatchResult::No(); }
};

class AnySig final : public TypeSigBase {
public:
    TypeSigKind kind() const override { return TypeSigKind::Any; }
    String describe(const ITypeSigContext&) const override { return "Any"; }
    TypeMatchResult matchValue(const Node& v, const ITypeSigContext& ctx, const TypeMatchOptions& opt) const override;
};

class NominalSig final : public TypeSigBase {
public:
    TypeId base;
    explicit NominalSig(TypeId b) : base(b) {}
    TypeSigKind kind() const override { return TypeSigKind::Nominal; }
    String describe(const ITypeSigContext& ctx) const override;
    TypeMatchResult matchValue(const Node& v, const ITypeSigContext& ctx, const TypeMatchOptions& opt) const override;
};

class ContainerSig final : public TypeSigBase {
public:
    TypeId base;
    Vector<TypeSignatureId> args;
    ContainerSig(TypeId b, Vector<TypeSignatureId> a) : base(b), args(std::move(a)) {}
    TypeSigKind kind() const override { return TypeSigKind::Container; }
    String describe(const ITypeSigContext& ctx) const override;
    TypeMatchResult matchValue(const Node& v, const ITypeSigContext& ctx, const TypeMatchOptions& opt) const override;
};

class UnionSig final : public TypeSigBase {
public:
    Vector<TypeSignatureId> members;
    explicit UnionSig(Vector<TypeSignatureId> m) : members(std::move(m)) {}
    TypeSigKind kind() const override { return TypeSigKind::Union; }
    String describe(const ITypeSigContext& ctx) const override;
    TypeMatchResult matchValue(const Node& v, const ITypeSigContext& ctx, const TypeMatchOptions& opt) const override;
};

class InvocableSig final : public TypeSigBase {
public:
    InvocableSigType data;
    explicit InvocableSig(InvocableSigType d) : data(std::move(d)) {}
    TypeSigKind kind() const override { return TypeSigKind::Invocable; }
    String describe(const ITypeSigContext& ctx) const override;
    TypeMatchResult matchCall(const ArgumentList& args, const ITypeSigContext& ctx, const TypeMatchOptions& opt) const override;
};
