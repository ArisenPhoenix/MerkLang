#pragma once
#include "core/types.h"
#include "core/errors.h"
#include <unordered_map>

class Type;
class TypeSet;
class InvocableType;

using TypePtr = SharedPtr<Type>;
using Types   = SharedPtr<TypeSet>;

Types makeTypes();

class TypeSet {
public:
    Vector<TypePtr> items;

    void add(const TypePtr& t);
    const Vector<TypePtr>& getRef() const;
    std::size_t size() const;
    bool empty() const;

    String toString(const String& sep = "|") const;
};

class Type {
public:
    NodeValueType primary{};

    Type() = default;
    explicit Type(NodeValueType p) : primary(p) {}
    virtual ~Type() = default;

    virtual NodeValueType getPrimary() const;
    virtual Types getSecondary() const;
    virtual Types getTertiary() const;

    virtual String toString() const;
};

class InvocableType : public Type {
public:
    Types args;
    Types returns;

    InvocableType();
    InvocableType(const Types& a, const Types& r);

    Types getSecondary() const override; // args
    Types getTertiary() const override;  // returns
    String toString() const override;
};

class ClassType : public Type {
public:
    String name;

    // Members: usually unique names => 1 index each
    Vector<String> memberNames;
    Types members;
    std::unordered_map<String, std::size_t> memberIndex;

    // Methods: overloads => name -> many indices
    Vector<String> methodNames; // one entry per overload
    Types methods;              // one entry per overload (InvocableType stored as TypePtr)
    std::unordered_map<String, Vector<std::size_t>> methodIndex;

    ClassType();
    explicit ClassType(const String& n);

    void addMember(const String& memberName, const TypePtr& memberType);

    // Adds ONE overload
    void addMethodOverload(const String& methodName, const SharedPtr<InvocableType>& sig);

    // Returns all overload candidates (as TypePtr, but they should dynamic_cast to InvocableType)
    Vector<TypePtr> getMethodOverloads(const String& methodName) const;

    // Convenience: return indices directly (useful for resolver)
    const Vector<std::size_t>* getMethodOverloadIndices(const String& methodName) const;

    TypePtr getMemberType(const String& memberName) const;

    Types getSecondary() const override; // members
    Types getTertiary() const override;  // methods
    String toString() const override;
};
