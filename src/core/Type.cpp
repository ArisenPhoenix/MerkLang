#include "core/Type.hpp"

Types makeTypes() { return std::make_shared<TypeSet>(); }

// -------- TypeSet --------
void TypeSet::add(const TypePtr& t) {
    if (!t) return;
    items.emplace_back(t);
}
const Vector<TypePtr>& TypeSet::getRef() const { return items; }
std::size_t TypeSet::size() const { return items.size(); }
bool TypeSet::empty() const { return items.empty(); }

String TypeSet::toString(const String& sep) const {
    String out;
    bool first = true;
    for (const auto& t : items) {
        if (!t) continue;
        if (!first) out += sep;
        out += t->toString();
        first = false;
    }
    return out;
}

// -------- Type --------
NodeValueType Type::getPrimary() const { return primary; }
Types Type::getSecondary() const { return {}; }
Types Type::getTertiary() const { return {}; }
String Type::toString() const { return "Type"; }

// -------- InvocableType --------
InvocableType::InvocableType() : Type() {
    args = makeTypes();
    returns = makeTypes();
}
InvocableType::InvocableType(const Types& a, const Types& r) : Type(), args(a), returns(r) {}

Types InvocableType::getSecondary() const { return args; }
Types InvocableType::getTertiary() const { return returns; }

String InvocableType::toString() const {
    const String a = args ? args->toString(",") : "";
    const String r = returns ? returns->toString("|") : "";
    return "fn(" + a + ") -> " + r;
}

// -------- ClassType --------
ClassType::ClassType() : Type() {
    members = makeTypes();
    methods = makeTypes();
}
ClassType::ClassType(const String& n) : Type(), name(n) {
    members = makeTypes();
    methods = makeTypes();
}

void ClassType::addMember(const String& memberName, const TypePtr& memberType) {
    if (!members) members = makeTypes();

    if (memberIndex.find(memberName) != memberIndex.end()) {
        throw MerkError("Duplicate class member: " + memberName);
    }

    const std::size_t idx = members->size();
    memberNames.emplace_back(memberName);
    members->add(memberType);
    memberIndex.emplace(memberName, idx);

    if (memberNames.size() != members->size()) {
        throw MerkError("ClassType invariant broken: memberNames/members length mismatch");
    }
}

void ClassType::addMethodOverload(const String& methodName, const SharedPtr<InvocableType>& sig) {
    if (!methods) methods = makeTypes();

    const std::size_t idx = methods->size();

    methodNames.emplace_back(methodName);
    methods->add(sig); // store as TypePtr (polymorphic)

    // append overload index
    methodIndex[methodName].emplace_back(idx);

    if (methodNames.size() != methods->size()) {
        throw MerkError("ClassType invariant broken: methodNames/methods length mismatch");
    }
}

const Vector<std::size_t>* ClassType::getMethodOverloadIndices(const String& methodName) const {
    auto it = methodIndex.find(methodName);
    if (it == methodIndex.end()) return nullptr;
    return &it->second;
}

Vector<TypePtr> ClassType::getMethodOverloads(const String& methodName) const {
    Vector<TypePtr> out;
    if (!methods) return out;

    auto it = methodIndex.find(methodName);
    if (it == methodIndex.end()) return out;

    const auto& v = methods->getRef();
    for (std::size_t idx : it->second) {
        if (idx < v.size() && v[idx]) out.emplace_back(v[idx]);
    }
    return out;
}

TypePtr ClassType::getMemberType(const String& memberName) const {
    if (!members) return {};
    auto it = memberIndex.find(memberName);
    if (it == memberIndex.end()) return {};

    const auto idx = it->second;
    const auto& v = members->getRef();
    return (idx < v.size()) ? v[idx] : TypePtr{};
}

Types ClassType::getSecondary() const { return members; }
Types ClassType::getTertiary() const { return methods; }

String ClassType::toString() const {
    return "class " + name;
}
