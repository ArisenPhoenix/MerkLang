#pragma once
#include <unordered_map>
#include <vector>

#include "core/types/TypeIds.hpp"
#include "core/TypesFWD.hpp" // String, Vector, NodeValueType

class TypeRegistry {
public:
    TypeRegistry();

    // Nominal identity
    TypeId getOrCreate(const String& name);
    TypeId lookupOrInvalid(const String& name) const;
    const String& nameOf(TypeId id) const;

    // Primitives
    TypeId primitiveId(NodeValueType t) const;

    // Convenience (optional)
    TypeId anyId() const { return anyId_; }
    TypeId listId() const { return listId_; }
    TypeId dictId() const { return dictId_; }

private:
    std::unordered_map<String, TypeId> nameToId_;
    Vector<String> idToName_;
    TypeId nextId_ = 1;

    TypeId anyId_  = kInvalidTypeId;
    TypeId listId_ = kInvalidTypeId;
    TypeId dictId_ = kInvalidTypeId;
    TypeId setId_  = kInvalidTypeId;
    TypeId arrId_   = kInvalidTypeId;

    TypeId httpId_ = kInvalidTypeId;
    TypeId fileId_ = kInvalidTypeId;
    void seedBase();
};
