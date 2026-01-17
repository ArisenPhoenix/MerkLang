#include "core/TypesFWD.hpp"
// #include "core/TypeBuiltins.hpp"
// #include "core/Scope.hpp" // for scope->... access
// #include <unordered_map>
// // static inline void addPrim(TypeRegistry& tr, NodeValueType t) {
// //     tr.add(TypeRegistry::primitive(t));
// // }

// // static inline void addNamed(TypeRegistry& tr, const String& n) {
// //     tr.add(TypeRegistry::named(n));
// // }

// // If your Scope exposes its type registry as `scope->globalTypes` or similar,
// // adjust accordingly.
// void registerBuiltinTypes(SharedPtr<Scope> scope) {
//     if (!scope) return;

//     // Core meta
//     scope->registerType(TypeRegistry::any());
    
//     // Scalar primitives
//     auto primitives = {NodeValueType::None, NodeValueType::Null, NodeValueType::Uninitialized};
//     scope->registerPrimitiveType(NodeValueType::None);
//     scope->registerPrimitiveType(NodeValueType::Null);
//     scope->registerPrimitiveType(NodeValueType::Uninitialized);

//     auto scalars = {
//         NodeValueType::Bool, NodeValueType::Char, NodeValueType::String, 
//         NodeValueType::Int, NodeValueType::Long, NodeValueType::Float, NodeValueType::Double,
//         NodeValueType::Number, NodeValueType::Text
//     };

//     for (auto& scalar : scalars) {
//         scope->registerPrimitiveType(scalar);
//     }


//     // These are "group supertypes" in matcher;
//     scope->registerPrimitiveType(NodeValueType::Number);
//     scope->registerPrimitiveType(NodeValueType::Text);


//     // Nominal builtins that exist as classes in the runtime

//     // 3) Containers (as nominal names + structural generics)
//     // Deciding whether to add them or not
//     //  - primitives (NodeValueType::List/Dict/etc)
//     //  - OR "Named"/"Container" kinds
//     //
//     // With TypeKind, these are Container (base + params).
//     // Seed the typical "unknown element type" versions:
//     // These should match native classes:
    
//     auto Any = TypeRegistry::any();
//     const std::unordered_map<String, Vector<TypeId>>& containers = {
//         {"List", {Any}}, {"Array", {Any}}, 
//         {"Set", {Any}}, {"Dict", {Any, Any}}
//     };
    
//     for (auto& cont : containers) {
//         scope->registerType(TypeRegistry::container(cont.first, cont.second));
//     }

//     for (auto [named, arr] : containers) {
//         auto name = named;
//         scope->registerNamedType(name);
//     }


//     // Callable-ish nominals (making them visible as types)
//     auto callables = {
//         NodeValueType::Function, NodeValueType::Method, 
//         NodeValueType::Callable, NodeValueType::CallableSignature, 
//         NodeValueType::Class, NodeValueType::ClassInstance             // Class / instances (For specificity in semantics)
//     };

//     for (auto& callable : callables) {
//         scope->registerPrimitiveType(callable);
//     }

//     // I May choose to add scope as a runtime type, but probably don't want to expose it
//     // addPrim(scope->globalTypes, NodeValueType::Scope);
// }



void registerBuiltinTypes(SharedPtr<Scope> scope) {(void)scope;}