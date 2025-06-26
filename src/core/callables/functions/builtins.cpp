// #include "core/types.h"
// #include "core/callables/callable.h"
// #include "core/callables/functions/native_function.h"
// #include "core/callables/functions/function.h"
// #include "core/callables/param_node.h"
// #include "core/callables/functions/builtins.h"

// #include "core/scope.h"

// // Define native functions

// Node print(Vector<Node> args, [[maybe_unused]] SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) {
//   (void)instanceNode;
//   for (size_t i = 0; i < args.size(); ++i) {
//         std::cout << args[i].toString();
//         if (i < args.size() - 1) std::cout << " ";
//     }
//     std::cout << std::endl;
//     return Node(); // none
// }  



// SharedPtr<NativeFunction> createPrintFunction([[maybe_unused]] SharedPtr<Scope> scope) {
//     ParamList params;
//     auto param = ParamNode("value", NodeValueType::Any);
//     param.setIsVarArgsParam(true);
//     params.addParameter(param);

//     return makeShared<NativeFunction>(
//         "print",
//         std::move(params),
//         print
//     );
// }




// std::unordered_map<String, NativeFuncFactory> nativeFunctionFactories = {
//     {"print", createPrintFunction},
// };


// std::unordered_map<String, SharedPtr<CallableSignature>> getAllNativeFunctions(SharedPtr<Scope> scope) {
//     std::unordered_map<String, SharedPtr<CallableSignature>> builtins;
//     for (auto& [funcName, funcSigCall] : nativeFunctionFactories) {
//         auto nativeFunc = funcSigCall(scope); // safe
//         builtins[funcName] = nativeFunc->toCallableSignature();
//     }
//     return builtins;
// }




