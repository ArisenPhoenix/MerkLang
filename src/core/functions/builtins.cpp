
#include "core/functions/callable.h"
#include "core/functions/native_function.h"
#include "core/functions/function_node.h"
#include "core/functions/param_node.h"


#include "core/functions/builtins.h"
#include "core/scope.h"

// Define native functions



// SharedPtr<NativeFunction> createPrintFunction([[maybe_unused]] SharedPtr<Scope> scope) {
//     ParamList params;
//     auto param = ParamNode("value", NodeValueType::Any);
//     param.setIsVarArgsParam(true);
//     params.addParameter(param);
//     return makeShared<NativeFunction>(
//         "print",
//         std::move(params),
//         [](Vector<Node> args, [[maybe_unused]] SharedPtr<Scope> scope) -> Node {
//             std::cout << args[0].toString() << std::endl;
//             return Node(); // none/void-equivalent
//         }
//     );
// }


SharedPtr<NativeFunction> createPrintFunction([[maybe_unused]] SharedPtr<Scope> scope) {
    ParamList params;
    auto param = ParamNode("value", NodeValueType::Any);
    param.setIsVarArgsParam(true);
    params.addParameter(param);

    return makeShared<NativeFunction>(
        "print",
        std::move(params),
        [](Vector<Node> args, [[maybe_unused]] SharedPtr<Scope> scope) -> Node {
            for (size_t i = 0; i < args.size(); ++i) {
                std::cout << args[i].toString();
                if (i < args.size() - 1) std::cout << " ";
            }
            std::cout << std::endl;
            return Node(); // none
        }
    );
}




std::unordered_map<String, NativeFuncFactory> nativeFunctionFactories = {
    {"print", createPrintFunction},
};


std::unordered_map<String, SharedPtr<CallableSignature>> getNativeFunctions(SharedPtr<Scope> scope) {
    std::unordered_map<String, SharedPtr<CallableSignature>> builtins;
    for (auto& [funcName, funcSigCall] : nativeFunctionFactories) {
        auto nativeFunc = funcSigCall(scope); // safe
        builtins[funcName] = nativeFunc->toCallableSignature();
    }
    return builtins;
}

// Vector<std::unordered_map<String, SharedPtr<CallableSignature>>> getNativeFunctions(SharedPtr<Scope> scope) {
//     Vector<std::unordered_map<String, SharedPtr<CallableSignature>>> builtins;
//     for (auto& [name, func] : nativeFunctionFactories) {
//         // builtins.push_back( std::unordered_map(name makeShared<FunctionNode>(func(scope)));
//         builtins.push_back({name, func})
//     }

    

//     return builtins;
// }









// SharedPtr<Callable> createPrintFunction([[maybe_unused]] SharedPtr<Scope> scope) {
//     ParamList params;
//     params.addParameter(ParamNode("value", NodeValueType::Any));
//     // NativeFunction()
//     return makeShared<NativeFunction>(
//         "print",
//         std::move(params),
//         [](Vector<Node> args, [[maybe_unused]] SharedPtr<Scope> scope) -> Node {
//             std::cout << args[0].toString() << std::endl;
//             return Node(); // none/void-equivalent
//         }
//     );
// }

// NativeFunction("", params, [](Vector<Node> args, SharedPtr<Scope> scope) -> Node {
    //         std::cout << args[0].toString() << std::endl;
    //         return Node(); // none-equivalent
    //     });



    // NativeFunction(
    //     "print",
    //     std::move(params),
    //     [](Vector<Node> args, SharedPtr<Scope> scope) -> Node {
    //         std::cout << args[0].toString() << std::endl;
    //         return Node(); // none-equivalent
    //     }, scope)


// std::unordered_map<String, NativeFuncFactory> nativeFunctionFactories = {
//     {"print", createPrintFunction},
//     // String("print"), createPrintFunction,
// };


// Vector<SharedPtr<CallableNode>> getNativeFunctions(SharedPtr<Scope> scope) {
//     Vector<std::unordered_map<String, SharedPtr<CallableNode>>> builtins;
//     for (auto& [name, func] : nativeFunctionFactories) {
//         builtins.push_back({name, makeShared<FunctionNode>(func(scope))});
//     }

    

//     return builtins;
// }





    // builtins.push_back(makeShared<FunctionNode>(createPrintFunction()));

// makeShared<FunctionNode>(std::static_pointer_cast<Callable>(nativeFn))
    // builtins.push_back(makeShared<FunctionNode>(createLenFunction()));
    // Add more as needed