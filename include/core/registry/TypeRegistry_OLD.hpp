// #pragma once

// #


// // --------------------
// // TypeSignature facade
// // --------------------
// class TypeRegistry;

// class TypeSignature {
// public:
//     TypeSignatureId id() const;
//     TypeId baseId() const { return baseId_; }

//     TypeSigKind kind() const;
//     const String& name() const;
//     String toString() const;

//     TypeMatchResult matchValue(
//         const Node&,
//         const TypeRegistry&,
//         const TypeMatchOptions& opt = {}
//     ) const;

//     TypeMatchResult matchCall(
//         const ArgumentList&,
//         const TypeRegistry&,
//         const TypeMatchOptions& opt = {}
//     ) const;


//     TypeSignature(TypeSignatureId id, String name, SharedPtr<TypeBase> t);
    
// private:
//     friend class TypeRegistry;
    
//     TypeSignatureId id_;
//     String name_;
//     SharedPtr<TypeBase> type_;
//     TypeId baseId_ = kInvalidTypeId;

    
// };

// // --------------------
// // Registry
// // --------------------

// class TypeRegistry {
// public:
//     TypeRegistry();
//     static TypeRegistry& global();

//     TypeSignatureId any();
//     TypeSignatureId primitive(NodeValueType prim);
//     TypeSignatureId classType(const String& className);
//     TypeSignatureId invocableType(InvocableType m);
//     TypeSignatureId unite(Vector<TypeSignatureId> members);
//     TypeSignatureId container(const String& base, Vector<TypeSignatureId> args);
//     TypeSignatureId listOf(TypeSignatureId elem);

//     TypeSignatureId dictOf(TypeSignatureId key, TypeSignatureId val);


//     const TypeSignature& get(TypeSignatureId id) const;

//     void bindName(const String& name, TypeSignatureId id);
//     std::optional<TypeSignatureId> lookupName(const String& name) const;

//     TypeSignatureId bindResolvedType(const ResolvedType& rt, Scope& scope);

//     TypeMatchResult matchValue(
//         TypeSignatureId expected,
//         const Node& v,
//         const TypeMatchOptions& opt = {}
//     ) const;

//     TypeMatchResult matchCall(
//         TypeSignatureId sigId,
//         const ArgumentList& args,
//         const TypeMatchOptions& opt = {}
//     ) const;

//     static bool isNumeric(NodeValueType t);
//     static int numericRank(NodeValueType t);
//     String toString(TypeSignatureId id) const;

//     static void setGlobal(TypeRegistry*);

//     bool isValid(TypeSignatureId id) const;
//     bool isValid(const TypeSignatureId id);
//     TypeSignatureId inferFromValue(const Node& v);

//     std::unordered_map<String, TypeId> baseNameToTypeId_;
//     Vector<String> typeIdToBaseName_;
//     TypeId nextTypeId_ = 1;

//     TypeId getOrCreateTypeId(const String& baseName);
//     TypeId getTypeIdOrInvalid(const String& baseName) const;

// private:
//     struct Key {
//         TypeSigKind kind = TypeSigKind::Any;
//         NodeValueType prim = NodeValueType::Any;
//         String name;
//         Vector<TypeSignatureId> kids;
//         Vector<uint8_t> enforced;
//         TypeSignatureId ret = 0;
//         bool variadic = false;
//         bool retEnforced = false;

//         bool operator==(const Key& o) const;
//     };

//     struct KeyHash {
//         size_t operator()(const Key& k) const;
//     };

//     TypeSignatureId intern(const Key& key, SharedPtr<TypeBase> node, String displayName);

//     Vector<TypeSignature> pool_;
//     std::unordered_map<Key, TypeSignatureId, KeyHash> intern_;
//     std::unordered_map<String, TypeSignatureId> nameToId_;
// };






