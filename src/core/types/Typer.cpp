// // #include "core/types/Typer.hpp"
// #include "core/Scope.hpp"
// #include "core/callables/classes/Method.hpp"

// namespace Typer {
// TypeSignatureId TypeMethod(SharedPtr<Method> method, TypeRegistry& typeRegistry) {
//     InvocableType mt;
//     mt.methodName = method->getName();
//     SharedPtr<Scope> scope = method->getCapturedScope();
//     SharedPtr<Scope>& workingScope = scope;
//     // Parameters
//     for (auto& param : method->parameters) {
//         // if (param)
//         if (param.flags.type != NodeValueType::Any) {
//             auto tid = typeRegistry.bindResolvedType(param.typeExpr, workingScope);
//             mt.params.push_back(tid);
//             mt.enforced.push_back(1);
//         } else {
//             mt.params.push_back(typeRegistry.any());
//             mt.enforced.push_back(0);
//         }
//     }

//     // Return type
//     if (method->getRequiresReturn()) {
//         mt.ret = typeRegistry.bindResolvedType(method->getReturnType, workingScope);
//         mt.retEnforced = true;
//     } else {
//         mt.ret = typeRegistry.any();
//         mt.retEnforced = false;
//     }

//     // Intern canonical signature
//     sig.typeSig = typeRegistry.invocableType(std::move(mt));
// }

//     TypeSignatureId TypeClass() {

//     }

//     TypeSignatureId TypeFunction() {

//     }

//     TypeSignatureId TypeNodeValue() {

//     }

//     TypeSignatureId TypeResolvedType() {

//     }
// }