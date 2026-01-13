// ==============================
// TypeSystem.hpp
// ==============================
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Your project aliases / forward decls
#include "core/TypesFWD.hpp"   // String, Vector, SharedPtr, etc.
// #include "core/types.h"        // NodeValueType

// using TypeId = uint32_t;
// static constexpr TypeId kInvalidTypeId = 0;

// --------------------
// Canonical kind
// --------------------
enum class TypeKind : uint8_t {
    Any,
    Primitive,   // Int, String, Bool, None, ...
    Container,   // List[T], Dict[K,V], Set[T], Array[T], Vector[T] ...
    Named,       // nominal type by name (symbolic)
    Instance,    // instance-of named/class type (nominal)
    Union,       // A|B|C
    Callable,    // future
    Invocable
};

enum class MatchMode : uint8_t {
    Exact,
    Assignable,
    Duck
};

struct MatchOptions {
    MatchMode mode = MatchMode::Assignable;

    bool allowAny = true;
    bool allowNumericWidening = true;
    bool invariantGenerics = true;     // for containers
    bool allowNoneToMatchAny = false;  // policy knob
};

struct MatchResult {
    int cost = 0; // 0 perfect. Extend as needed.
};

struct TypeNode {
    TypeKind kind = TypeKind::Any;
    NodeValueType prim = NodeValueType::Any;   // valid for Primitive/Any
    String name;                               // Container base OR Named/Instance name
    Vector<TypeId> params;                     // Container args OR Union members OR Callable parts
    String alias;                              // display-only (NOT part of identity)
    Vector<TypeId> methods;                    // Methods for a class if any
    // Pure matching: resolves through global canonical store (TypeRegistry::nodeOf).
    std::optional<MatchResult> match(TypeId other, const MatchOptions& opt = {}) const;

    // Structural identity (alias ignored)
    bool operator==(const TypeNode& o) const {
        return kind == o.kind && prim == o.prim && name == o.name && params == o.params;
    }
};

// --------------------
// TypeRegistry: per-scope membership + global canonical store
//

// --------------------
class TypeRegistry {
public:
    TypeRegistry() = default;

    // -------- Global canonicalizer --------
    static TypeId idOf(const TypeNode& n);
    static const TypeNode& nodeOf(TypeId id);

    // Convenience global constructors (canonical ids)
    static TypeId any();
    static TypeId primitive(NodeValueType t, const String& alias = "");
    static TypeId named(const String& name, const String& alias = "");
    static TypeId instanceOf(const String& name, const String& alias = "");
    static TypeId container(const String& base, const Vector<TypeId>& ps, const String& alias = "");
    static TypeId unite(const Vector<TypeId>& members, const String& alias = "");

    // -------- Per-scope membership --------
    bool contains(TypeId id) const;
    void add(TypeId id);
    void clear();

    // Optional helpers: declare common builtins in a scope
    void addBuiltins(); // Any + primitive core tags (tune to your language)

    // -------- Matching / printing --------
    static std::optional<MatchResult> match(TypeId actual, TypeId expected, const MatchOptions& opt = {});
    static String toString(TypeId id);

    // -------- Helpers --------
    static bool isNumeric(NodeValueType t);
    static bool isTextual(NodeValueType t);
    static bool isNone(NodeValueType t);
    static int  numericRank(NodeValueType t);

    void addPrim(NodeValueType t) {
        add(primitive(t));
    }

    void addNamed(const String& n) {
        add(named(n));
    }

private:
    // Membership set (scope visibility)
    std::unordered_set<TypeId> known_;

    // -------- Global canonical store --------
    struct TypeNodeHash {
        size_t operator()(const TypeNode& n) const;
    };
    struct TypeNodeEq {
        bool operator()(const TypeNode& a, const TypeNode& b) const { return a == b; }
    };

    struct Store {
        Vector<TypeNode> nodes; // index == TypeId
        std::unordered_map<TypeNode, TypeId, TypeNodeHash, TypeNodeEq> intern;

        Store();
        TypeId internNode(TypeNode n);
        Vector<TypeId> normalizeUnionMembers(const Vector<TypeId>& members) const;
    };

    static Store& store();

public:
    void debugPrint() const;
};
