// #
// #include "core/types.h"
// #include "core/node/ArgumentNode.hpp"
// #include <algorithm>
// #include <cassert>
// #include "core/registry/TypeRegistry.hpp"
// #include "core/callables/classes/ClassBase.hpp"

// // Legacy helpers (kept here so the legacy TypeBase hierarchy continues to compile).
// static bool isNumeric(NodeValueType t) {
//     switch (t) {
//         case NodeValueType::Int:
//         case NodeValueType::Float:
//         case NodeValueType::Double:
//             return true;
//         default:
//             return false;
//     }
// }

// static int numericRank(NodeValueType t) {
//     switch (t) {
//         case NodeValueType::Int: return 1;
//         case NodeValueType::Float: return 2;
//         case NodeValueType::Double: return 3;
//         default: return 0;
//     }
// }
// // --------------------
// // TypeBase defaults
// // --------------------

// TypeMatchResult TypeBase::matchValueInternal(
//     const Node&,
//     const TypeRegistry&,
//     const TypeMatchOptions&
// ) const {
//     return TypeMatchResult::No();
// }

// TypeMatchResult TypeBase::matchCallInternal(
//     const ArgumentList&,
//     const TypeRegistry&,
//     const TypeMatchOptions&
// ) const {
//     return TypeMatchResult::No();
// }

// // --------------------
// // AnyType
// // --------------------

// TypeSigKind AnyType::kind() const { return TypeSigKind::Any; }

// String AnyType::describe(const TypeRegistry&) const {
//     return "Any";
// }

// TypeMatchResult AnyType::matchValueInternal(
//     const Node&,
//     const TypeRegistry&,
//     const TypeMatchOptions& opt
// ) const {
//     return opt.allowAny ? TypeMatchResult::Yes(1) : TypeMatchResult::No();
// }

// // --------------------
// // PrimitiveType
// // --------------------

// PrimitiveType::PrimitiveType(NodeValueType p) : prim(p) {}

// TypeSigKind PrimitiveType::kind() const { return TypeSigKind::Nominal; }

// String PrimitiveType::describe(const TypeRegistry&) const {
//     return nodeTypeToString(prim); // assuming helper exists
// }

// TypeMatchResult PrimitiveType::matchValueInternal(
//     const Node& v,
//     const TypeRegistry&,
//     const TypeMatchOptions& opt
// ) const {
//     auto vt = v.getType();
//     if (vt == prim)
//         return TypeMatchResult::Yes(100);

//     if (opt.allowNumericWidening &&
//         isNumeric(vt) &&
//         isNumeric(prim) &&
//         numericRank(vt) <=
//         numericRank(prim)) {
//         return TypeMatchResult::Yes(60);
//     }

//     return TypeMatchResult::No();
// }




// // --------------------
// // ClassType
// // --------------------

// ClassType::ClassType(String n) : className(std::move(n)) {}

// TypeSigKind ClassType::kind() const { return TypeSigKind::Nominal; }

// String ClassType::describe(const TypeRegistry&) const {
//     return className;
// }

// TypeMatchResult ClassType::matchValueInternal(
//     const Node& v,
//     const TypeRegistry&,
//     const TypeMatchOptions&
// ) const {
//     if (!v.isInstance())
//         return TypeMatchResult::No();

//     const auto inst = v.toInstance();
//     if (!inst)
//         return TypeMatchResult::No();

//     if (inst->getName() == className)
//         return TypeMatchResult::Yes(100);

//     return TypeMatchResult::No();
// }


// // --------------------
// // InvocableType
// // --------------------

// TypeSigKind InvocableType::kind() const { return TypeSigKind::Invocable; }

// String InvocableType::describe(const TypeRegistry& reg) const {
//     String s;

//     if (!methodName.empty())
//         s += methodName;

//     s += "(";

//     for (size_t i = 0; i < params.size(); ++i) {
//         if (i) s += ", ";

//         if (!enforced[i]) {
//             s += "Any";
//         } else {
//             s += reg.get(params[i]).name();
//         }
//     }

//     s += ")";

//     if (ret != kInvalidTypeSignatureId) {
//         s += " -> ";
//         s += reg.get(ret).name();
//     }

//     return s;
// }


// TypeMatchResult InvocableType::matchCallInternal(
//     const ArgumentList& args,
//     const TypeRegistry& reg,
//     const TypeMatchOptions& opt
// ) const {
//     if (!variadic && args.size() != params.size())
//         return TypeMatchResult::No();

//     int score = 0;
//     for (size_t i = 0; i < args.size(); ++i) {
//         if (!enforced[i]) {
//             score += 1;
//             continue;
//         }
//         auto r = reg.matchValue(params[i], args[i], opt);
//         if (!r.ok)
//             return TypeMatchResult::No();

//         score += r.score;
//     }
//     return TypeMatchResult::Yes(score);
// }

// // --------------------
// // UnionType
// // --------------------

// UnionType::UnionType(Vector<TypeSignatureId> m) : members(std::move(m)) {}

// TypeSigKind UnionType::kind() const { return TypeSigKind::Union; }

// String UnionType::describe(const TypeRegistry& reg) const {
//     String s;
//     for (size_t i = 0; i < members.size(); ++i) {
//         if (i) s += " | ";
//         s += reg.get(members[i]).name();
//     }
//     return s;
// }

// TypeMatchResult UnionType::matchValueInternal(
//     const Node& v,
//     const TypeRegistry& reg,
//     const TypeMatchOptions& opt
// ) const {
//     int best = -1;
//     for (auto id : members) {
//         auto r = reg.matchValue(id, v, opt);
//         if (r.ok)
//             best = std::max(best, r.score);
//     }
//     return best >= 0 ? TypeMatchResult::Yes(best) : TypeMatchResult::No();
// }

// // --------------------
// // ContainerType
// // --------------------

// ContainerType::ContainerType(String b, Vector<TypeSignatureId> a)
//     : base(std::move(b)), args(std::move(a)) {}

// TypeSigKind ContainerType::kind() const { return TypeSigKind::Container; }

// String ContainerType::describe(const TypeRegistry&) const {
//     return base;
// }

// TypeMatchResult ContainerType::matchValueInternal(
//     const Node&,
//     const TypeRegistry&,
//     const TypeMatchOptions&
// ) const {
//     return TypeMatchResult::No();
// }

// // --------------------
// // TypeSignature
// // --------------------

// TypeSignature::TypeSignature(TypeSignatureId id, String name, SharedPtr<TypeBase> t)
//     : id_(id), name_(std::move(name)), type_(std::move(t)) {}

// TypeSignatureId TypeSignature::id() const { return id_; }
// TypeSigKind TypeSignature::kind() const { return type_->kind(); }
// const String& TypeSignature::name() const { return name_; }
// String TypeSignature::toString() const { return name_; }

// TypeMatchResult TypeSignature::matchValue(
//     const Node& v,
//     const TypeRegistry& reg,
//     const TypeMatchOptions& opt
// ) const {
//     return reg.matchValue(id_, v, opt);
// }

// TypeMatchResult TypeSignature::matchCall(
//     const ArgumentList& args,
//     const TypeRegistry& reg,
//     const TypeMatchOptions& opt
// ) const {
//     return reg.matchCall(id_, args, opt);
// }


