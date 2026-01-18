// #
// #include "core/Scope.hpp"
// #include "core/registry/TypeRegistry.hpp"
// # // TypeBase subclasses (PrimitiveType, ContainerType, etc)

// // Utility: ensure registry exists
// static TypeRegistry& types(SharedPtr<Scope> scope) {
//     if (!scope->globalTypes) {
//         // scope->globalTypes = makeShared<TypeRegistry>();
//         throw MerkError("No scope->globalType");
//     }
//     return *scope->globalTypes;
// }

// void registerBuiltinTypes(SharedPtr<Scope> scope) {
//     auto& tr = types(scope);

//     auto anyId = tr.any();
//     tr.bindName("Any", anyId);

//     tr.bindName("Bool",  tr.primitive(NodeValueType::Bool));
//     tr.bindName("Int",   tr.primitive(NodeValueType::Int));
//     tr.bindName("Float", tr.primitive(NodeValueType::Float));
//     tr.bindName("String",tr.primitive(NodeValueType::String));

//     // Bare container aliases
//     tr.bindName("List", tr.container("List", { anyId }));
//     tr.bindName("Dict", tr.container("Dict", { anyId, anyId }));
// }
