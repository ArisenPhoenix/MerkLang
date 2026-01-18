// #include "core/registry/TypeRegistry_NEW.hpp"
// #include "core/TypesFWD.hpp" // nodeTypeToString if you want

// TypeRegistry::TypeRegistry() {
//     idToName_.resize(1); // slot 0 invalid
//     seedBase();
// }

// void TypeRegistry::seedBase() {
//     anyId_  = getOrCreate("Any");
//     getOrCreate("Bool");
//     getOrCreate("Int");
//     getOrCreate("Float");
//     getOrCreate("Double");
//     getOrCreate("String");
//     getOrCreate("Char");

//     listId_ = getOrCreate("List");
//     dictId_ = getOrCreate("Dict");
//     getOrCreate("Set");
//     getOrCreate("Array");

//     getOrCreate("Union");
//     getOrCreate("Invocable");
//     getOrCreate("Callable");
// }

// TypeId TypeRegistry::getOrCreate(const String& name) {
//     if (name.empty()) return kInvalidTypeId;

//     auto it = nameToId_.find(name);
//     if (it != nameToId_.end()) return it->second;

//     TypeId id = nextId_++;
//     nameToId_[name] = id;

//     if ((size_t)id >= idToName_.size()) idToName_.resize((size_t)id + 1);
//     idToName_[id] = name;
//     return id;
// }

// TypeId TypeRegistry::lookupOrInvalid(const String& name) const {
//     auto it = nameToId_.find(name);
//     if (it == nameToId_.end()) return kInvalidTypeId;
//     return it->second;
// }

// const String& TypeRegistry::nameOf(TypeId id) const {
//     static String invalid = "<invalid>";
//     if (id == kInvalidTypeId || (size_t)id >= idToName_.size()) return invalid;
//     return idToName_[id];
// }

// TypeId TypeRegistry::primitiveId(NodeValueType t) const {
//     switch (t) {
//         case NodeValueType::Bool:   return lookupOrInvalid("Bool");
//         case NodeValueType::Int:    return lookupOrInvalid("Int");
//         case NodeValueType::Float:  return lookupOrInvalid("Float");
//         case NodeValueType::Double: return lookupOrInvalid("Double");
//         case NodeValueType::String: return lookupOrInvalid("String");
//         case NodeValueType::Char:   return lookupOrInvalid("Char");
//         default:                    return anyId_;
//     }
// }