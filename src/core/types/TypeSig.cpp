#include "core/types/TypeSig.hpp"
#include "core/registry/TypeSignatureRegistryManager.hpp"   // NEW
#include "core/registry/TypeRegistry.hpp"
#include "core/node/Node.hpp"
#include "core/node/ArgumentNode.hpp"
#include "core/types.h"
#include "core/callables/classes/ClassBase.hpp"
#include <algorithm>

// ---- helpers ----
static bool isNumeric(NodeValueType t) {
    switch (t) {
        case NodeValueType::Int:
        case NodeValueType::Float:
        case NodeValueType::Double:
            return true;
        default:
            return false;
    }
}
static int numericRank(NodeValueType t) {
    switch (t) {
        case NodeValueType::Int:    return 1;
        case NodeValueType::Float:  return 2;
        case NodeValueType::Double: return 3;
        default:                    return 0;
    }
}

TypeMatchResult AnySig::matchValue(const Node&, const ITypeSigContext&, const TypeMatchOptions& opt) const {
    return opt.allowAny ? TypeMatchResult::Yes(1) : TypeMatchResult::No();
}

String NominalSig::describe(const ITypeSigContext& ctx) const {
    return ctx.types().nameOf(base);
}

String ContainerSig::describe(const ITypeSigContext& ctx) const {
    String s = ctx.types().nameOf(base);
    s += "[";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i) s += ", ";
        s += ctx.toString(args[i]);
    }
    s += "]";
    return s;
}

String UnionSig::describe(const ITypeSigContext& ctx) const {
    String s;
    for (size_t i = 0; i < members.size(); ++i) {
        if (i) s += " | ";
        s += ctx.toString(members[i]);
    }
    return s;
}

String InvocableSig::describe(const ITypeSigContext& ctx) const {
    String s;
    if (!data.methodName.empty()) s += data.methodName;
    s += "(";
    for (size_t i = 0; i < data.params.size(); ++i) {
        if (i) s += ", ";
        s += data.enforced[i] ? ctx.toString(data.params[i]) : "Any";
        if (data.variadic && i + 1 == data.params.size()) s += "...";
    }
    s += ")";
    if (data.ret != kInvalidTypeSignatureId) {
        s += " -> ";
        s += ctx.toString(data.ret);
    }
    return s;
}

// Map runtime value -> TypeId WITHOUT mutating registries.
static TypeId typeIdOfValue(const Node& v, const TypeRegistry& tr) {
    if (v.isNull()) return tr.anyId();

    switch (v.getType()) {
        case NodeValueType::Bool:
        case NodeValueType::Int:
        case NodeValueType::Float:
        case NodeValueType::Double:
        case NodeValueType::String:
        case NodeValueType::Char:
            return tr.primitiveId(v.getType());
        default:
            break;
    }

    if (v.isInstance()) {
        const auto baseName = v.getFlags().fullType.getBaseType();
        if (!baseName.empty() && baseName != "Any") {
            return tr.lookupOrInvalid(baseName);
        }
        auto inst = v.toInstance();
        if (!inst) return kInvalidTypeId;
        return tr.lookupOrInvalid(inst->getName());
    }

    if (v.isList()) return tr.listId();
    if (v.isDict()) return tr.dictId();

    return tr.anyId();
}

TypeMatchResult NominalSig::matchValue(const Node& v, const ITypeSigContext& ctx, const TypeMatchOptions& opt) const {
    const auto& tr = ctx.types();

    TypeId got = typeIdOfValue(v, tr);
    if (got == base) return TypeMatchResult::Yes(100);

    if (opt.allowNumericWidening && v.getType() != NodeValueType::Any) {
        const auto& expName = tr.nameOf(base);
        NodeValueType expectedNvt = NodeValueType::Any;
        if (expName == "Int") expectedNvt = NodeValueType::Int;
        else if (expName == "Float") expectedNvt = NodeValueType::Float;
        else if (expName == "Double") expectedNvt = NodeValueType::Double;

        auto vt = v.getType();
        if (isNumeric(vt) && isNumeric(expectedNvt) && numericRank(vt) <= numericRank(expectedNvt))
            return TypeMatchResult::Yes(60);
    }

    return TypeMatchResult::No();
}

TypeMatchResult ContainerSig::matchValue(const Node& v, const ITypeSigContext& ctx, const TypeMatchOptions& opt) const {
    (void)opt;
    const auto& tr = ctx.types();
    TypeId got = typeIdOfValue(v, tr);
    if (got != base) return TypeMatchResult::No();
    return TypeMatchResult::Yes(80);
}

TypeMatchResult UnionSig::matchValue(const Node& v, const ITypeSigContext& ctx, const TypeMatchOptions& opt) const {
    int best = -1;
    for (auto id : members) {
        auto r = ctx.matchValue(id, v, opt);
        if (r.ok) best = std::max(best, r.score);
    }
    return (best >= 0) ? TypeMatchResult::Yes(best) : TypeMatchResult::No();
}

TypeMatchResult InvocableSig::matchCall(const ArgumentList& args, const ITypeSigContext& ctx, const TypeMatchOptions& opt) const {
    const size_t pcount = data.params.size();

    if (!data.variadic) {
        if (args.size() != pcount) return TypeMatchResult::No();
    } else {
        // Variadic: require at least pcount-1 args if last param is varargs bucket.
        if (pcount == 0) return TypeMatchResult::Yes(1); // weird but ok
        if (args.size() < pcount - 1) return TypeMatchResult::No();
    }

    int score = 0;

    for (size_t i = 0; i < args.size(); ++i) {
        size_t pi = i;

        if (data.variadic && pcount > 0 && i >= pcount) {
            pi = pcount - 1; // extra args match last param type
        } else if (pi >= pcount) {
            return TypeMatchResult::No();
        }

        if (pi >= data.enforced.size()) return TypeMatchResult::No(); // sanity
        if (!data.enforced[pi]) { score += 1; continue; }

        auto r = ctx.matchValue(data.params[pi], args[i], opt);
        if (!r.ok) return TypeMatchResult::No();
        score += r.score;
    }

    return TypeMatchResult::Yes(score);
}
