// ==============================
// TypeSystem.cpp
// ==============================
#include "core/Type.hpp"
#include "core/types.h"
#include <algorithm>
#include <sstream>
#include "utilities/debugging_functions.h"

// If you have this function in your codebase, keep it.
// Otherwise replace toString(Primitive) with your own conversion.

// --------------------
// tiny hash combiner
// --------------------
static inline void hash_combine(size_t& seed, size_t v) {
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// --------------------
// Store access
// --------------------
TypeRegistry::Store& TypeRegistry::store() {
    static Store g;
    return g;
}

// --------------------
// TypeRegistry::TypeNodeHash
// (alias intentionally ignored because TypeNode::operator== ignores it)
// --------------------
size_t TypeRegistry::TypeNodeHash::operator()(const TypeNode& n) const {
    size_t h = 0;
    hash_combine(h, std::hash<int>()(static_cast<int>(n.kind)));
    hash_combine(h, std::hash<int>()(static_cast<int>(n.prim)));
    hash_combine(h, std::hash<String>()(n.name));
    for (auto p : n.params) {
        hash_combine(h, std::hash<uint32_t>()(p));
    }
    return h;
}

// --------------------
// Store implementation
// --------------------
TypeRegistry::Store::Store() {
    // reserve 0 invalid
    nodes.push_back(TypeNode{});
    // seed Any at 1
    (void)internNode(TypeNode{TypeKind::Any, NodeValueType::Any, "Any", {}, ""});
}

TypeId TypeRegistry::Store::internNode(TypeNode n) {
    auto it = intern.find(n);
    if (it != intern.end()) return it->second;

    TypeId id = static_cast<TypeId>(nodes.size());
    nodes.push_back(std::move(n));
    intern.emplace(nodes[id], id);
    return id;
}

Vector<TypeId> TypeRegistry::Store::normalizeUnionMembers(const Vector<TypeId>& members) const {
    Vector<TypeId> out;
    out.reserve(members.size());

    // Flatten nested unions
    for (auto m : members) {
        const auto& mn = nodes.at(m);
        if (mn.kind == TypeKind::Union) {
            for (auto inner : mn.params) out.push_back(inner);
        } else {
            out.push_back(m);
        }
    }

    // Canonicalize: sort + unique
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

// --------------------
// Global canonicalizer
// --------------------
TypeId TypeRegistry::idOf(const TypeNode& n) {
    // IMPORTANT: alias is display-only and is ignored by equality/hash,
    // so passing alias here won't affect interning. If you need alias
    // for printing, store it in per-scope tables, not in the global node.
    return store().internNode(n);
}

const TypeNode& TypeRegistry::nodeOf(TypeId id) {
    return store().nodes.at(id);
}

// --------------------
// Global constructors (canonical ids)
// --------------------
TypeId TypeRegistry::any() {
    return store().internNode(TypeNode{TypeKind::Any, NodeValueType::Any, "Any", {}, ""});
}


TypeId TypeRegistry::primitive(NodeValueType t, const String& alias) {
    if (t == NodeValueType::Any) return any();
    TypeNode n;
    n.kind = TypeKind::Primitive;
    n.prim = t;
    n.alias = alias; // display-only; ignored by hashing/eq
    return store().internNode(std::move(n));
}

TypeId TypeRegistry::named(const String& name, const String& alias) {
    TypeNode n;
    n.kind = TypeKind::Named;
    n.name = name;
    n.alias = alias;
    return store().internNode(std::move(n));
}

TypeId TypeRegistry::instanceOf(const String& name, const String& alias) {
    TypeNode n;
    n.kind = TypeKind::Instance;
    n.name = name;
    n.alias = alias;
    return store().internNode(std::move(n));
}

TypeId TypeRegistry::container(const String& base, const Vector<TypeId>& ps, const String& alias) {
    TypeNode n;
    n.kind = TypeKind::Container;
    n.name = base;
    n.params = ps;
    n.alias = alias;
    return store().internNode(std::move(n));
}

TypeId TypeRegistry::unite(const Vector<TypeId>& members, const String& alias) {
    auto norm = store().normalizeUnionMembers(members);
    if (norm.empty()) return any();
    if (norm.size() == 1) return norm[0];

    TypeNode n;
    n.kind = TypeKind::Union;
    n.params = std::move(norm);
    n.alias = alias;
    return store().internNode(std::move(n));
}

// --------------------
// Per-scope membership
// --------------------
bool TypeRegistry::contains(TypeId id) const {
    return known_.find(id) != known_.end();
}

void TypeRegistry::add(TypeId id) {
    known_.insert(id);
}

void TypeRegistry::clear() {
    known_.clear();
}

void TypeRegistry::addBuiltins() {
    // Minimal: Any + common primitives (tune this list to your NodeValueType enum)
    add(any());
    add(primitive(NodeValueType::None));
    add(primitive(NodeValueType::Bool));
    add(primitive(NodeValueType::Int));
    add(primitive(NodeValueType::Long));
    add(primitive(NodeValueType::Float));
    add(primitive(NodeValueType::Double));
    add(primitive(NodeValueType::Number));
    add(primitive(NodeValueType::Char));
    add(primitive(NodeValueType::String));
    add(primitive(NodeValueType::Text));
}

// --------------------
// Matching / printing (global)
// --------------------
std::optional<MatchResult> TypeRegistry::match(TypeId actual, TypeId expected, const MatchOptions& opt) {
    return nodeOf(actual).match(expected, opt);
}

String TypeRegistry::toString(TypeId id) {
    const auto& n = nodeOf(id);

    // If alias exists on the canonical node, you can display it.
    // NOTE: because alias is ignored for identity, the first alias used
    // for a given structural node "wins" globally. If you want per-scope
    // aliasing, do NOT rely on node.alias; store alias in Scope maps instead.
    if (!n.alias.empty()) return n.alias;

    if (n.kind == TypeKind::Any) return "Any";

    if (n.kind == TypeKind::Primitive) {
        return nodeTypeToString(n.prim);
    }

    if (n.kind == TypeKind::Named) return n.name;

    if (n.kind == TypeKind::Instance) return n.name + " (instance)";

    if (n.kind == TypeKind::Container) {
        std::ostringstream oss;
        oss << n.name;
        if (!n.params.empty()) {
            oss << "[";
            for (size_t i = 0; i < n.params.size(); ++i) {
                oss << toString(n.params[i]);
                if (i + 1 < n.params.size()) oss << ", ";
            }
            oss << "]";
        }
        return oss.str();
    }

    if (n.kind == TypeKind::Union) {
        std::ostringstream oss;
        for (size_t i = 0; i < n.params.size(); ++i) {
            oss << toString(n.params[i]);
            if (i + 1 < n.params.size()) oss << " | ";
        }
        return oss.str();
    }

    if (n.kind == TypeKind::Callable) return "Callable";
    if (n.kind == TypeKind::Invocable) return "Invocable";

    return "<Type>";
}

// --------------------
// Helpers (global)
// --------------------
bool TypeRegistry::isNumeric(NodeValueType t) {
    switch (t) {
        case NodeValueType::Number:
        case NodeValueType::Int:
        case NodeValueType::Long:
        case NodeValueType::Float:
        case NodeValueType::Double:
            return true;
        default:
            return false;
    }
}

bool TypeRegistry::isTextual(NodeValueType t) {
    switch (t) {
        case NodeValueType::Text:
        case NodeValueType::Char:
        case NodeValueType::String:
            return true;
        default:
            return false;
    }
}

bool TypeRegistry::isNone(NodeValueType t) {
    switch (t) {
        case NodeValueType::None:
        case NodeValueType::Null:
        case NodeValueType::Uninitialized:
            return true;
        default:
            return false;
    }
}

int TypeRegistry::numericRank(NodeValueType t) {
    switch (t) {
        case NodeValueType::Int:    return 1;
        case NodeValueType::Long:   return 2;
        case NodeValueType::Float:  return 3;
        case NodeValueType::Double: return 4;
        case NodeValueType::Number: return 5; // supertype
        default: return 0;
    }
}

// --------------------
// TypeNode matching (pure; global node lookup)
// --------------------
std::optional<MatchResult> TypeNode::match(TypeId otherId, const MatchOptions& opt) const {
    const TypeNode& other = TypeRegistry::nodeOf(otherId);

    // Any policy
    if (opt.allowAny) {
        if (kind == TypeKind::Any) return MatchResult{0};
        if (other.kind == TypeKind::Any) return MatchResult{0};
    }

    // Exact mode: structural equality
    if (opt.mode == MatchMode::Exact) {
        if (*this == other) return MatchResult{0};
        return std::nullopt;
    }

    // Union expected: match any branch
    if (other.kind == TypeKind::Union) {
        int best = 1'000'000;
        for (auto m : other.params) {
            if (auto r = this->match(m, opt)) best = std::min(best, r->cost);
        }
        if (best == 1'000'000) return std::nullopt;
        return MatchResult{best};
    }

    // Union actual: require all members assignable to expected
    if (kind == TypeKind::Union) {
        int worst = 0;
        for (auto m : params) {
            auto r = TypeRegistry::nodeOf(m).match(otherId, opt);
            if (!r) return std::nullopt;
            worst = std::max(worst, r->cost);
        }
        return MatchResult{worst};
    }

    // None semantics: "no value"
    if (kind == TypeKind::Primitive && TypeRegistry::isNone(prim)) {
        if (other.kind == TypeKind::Primitive && TypeRegistry::isNone(other.prim))
            return MatchResult{0};

        if (opt.allowNoneToMatchAny && other.kind == TypeKind::Any)
            return MatchResult{0};

        return std::nullopt;
    }

    // Primitive widening/group supertypes
    if (kind == TypeKind::Primitive && other.kind == TypeKind::Primitive) {
        // Group supertypes
        if (other.prim == NodeValueType::Number && TypeRegistry::isNumeric(prim)) return MatchResult{1};
        if (other.prim == NodeValueType::Text  && TypeRegistry::isTextual(prim)) return MatchResult{1};

        // Numeric widening
        if (opt.allowNumericWidening && TypeRegistry::isNumeric(prim) && TypeRegistry::isNumeric(other.prim)) {
            int ra = TypeRegistry::numericRank(prim);
            int rb = TypeRegistry::numericRank(other.prim);
            if (ra <= rb) return MatchResult{rb - ra};
            return std::nullopt;
        }

        // Text widening: Char -> String
        if (prim == NodeValueType::Char && other.prim == NodeValueType::String) return MatchResult{1};

        return std::nullopt;
    }

    // Container matching
    if (kind == TypeKind::Container && other.kind == TypeKind::Container) {
        if (name != other.name) return std::nullopt;
        if (params.size() != other.params.size()) return std::nullopt;

        int cost = 0;
        for (size_t i = 0; i < params.size(); ++i) {
            MatchOptions childOpt = opt;
            if (opt.invariantGenerics) childOpt.mode = MatchMode::Exact;

            auto r = TypeRegistry::nodeOf(params[i]).match(other.params[i], childOpt);
            if (!r) return std::nullopt;
            cost += r->cost;
        }
        return MatchResult{cost};
    }

    // Nominal: Named/Instance
    if (kind == TypeKind::Named && other.kind == TypeKind::Named) {
        if (name == other.name) return MatchResult{0};
        return std::nullopt;
    }

    if (kind == TypeKind::Instance && other.kind == TypeKind::Instance) {
        if (name == other.name) return MatchResult{0};
        return std::nullopt;
    }

    // Instance matches Named (value is instance of that type)
    if (kind == TypeKind::Instance && other.kind == TypeKind::Named) {
        if (name == other.name) return MatchResult{0};
        return std::nullopt;
    }

    // Callable/Invocable placeholders: currently structural only.
    if (kind == other.kind && (kind == TypeKind::Callable || kind == TypeKind::Invocable)) {
        if (params == other.params && name == other.name && prim == other.prim) return MatchResult{0};
        return std::nullopt;
    }

    // Duck mode placeholder (add hooks later if desired)
    if (opt.mode == MatchMode::Duck) {
        // Intentionally not implemented: would require external contracts/data sources.
    }

    return std::nullopt;
}


void printTypeNode(TypeNode typeNode, String moreOutput="") {
    debugLog(true, typeNode.name +":", "NodeType:", nodeTypeToString(typeNode.prim));
}

void TypeRegistry::debugPrint() const {
    for (const auto& node : store().nodes) {
        printTypeNode(node);
    }
    for (const auto& interned : store().intern) {
        printTypeNode(interned.first, std::to_string(interned.second));
    }
}