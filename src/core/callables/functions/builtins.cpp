#include "core/types.h"
#include "core/callables/callable.h"
#include "core/callables/functions/native_function.h"
#include "core/callables/functions/function.h"
#include "core/callables/param_node.h"
#include "core/callables/functions/builtins.h"
#include "core/callables/classes/node_structures.h"
#include "core/scope.h"

// Define native functions 

Node print(Vector<Node> args, [[maybe_unused]] SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) {
  for (size_t i = 0; i < args.size(); ++i) {
        // DEBUG_LOG(LogLevel::PERMISSIVE, args[i].getTypeAsString());
        std::cout << args[i].toString();
        if (i < args.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;
    return Node(); // none
}


Node floatFunc(Vector<Node> args, [[maybe_unused]] SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) {
    if (args.size() != 1) {throw MerkError("Only One Argument may be passed to Function 'Float;");}
    return Node(args[0].toFloat());
}

Node intFunc(Vector<Node> args, [[maybe_unused]] SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) {
    if (args.size() != 1) {throw MerkError("Only One Argument may be passed to Function 'Float;");}
    return Node(args[0].toInt());
}

Node stringFunc(Vector<Node> args, [[maybe_unused]] SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) {
    if (args.size() != 1) {throw MerkError("Only One Argument may be passed to Function 'Float;");}
    return Node(args[0].toString());
}

Node isInstanceFunc(Vector<Node> args, [[maybe_unused]] SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) {
    if (args.size() != 2) {throw MerkError("Only One Argument may be passed to Function 'Float;");}

    auto var = args[0];
    auto instance = args[1];

    if (var.isInstance()) {
        auto inst = std::get<SharedPtr<ClassInstance>>(var.getValue());
        if (inst->getName() == instance.toString()) {
            return Node(true);
        }
    }
    return Node(nodeTypeToString(var.getType(), false) == instance.toString());
}


SharedPtr<NativeFunction> createPrintFunction([[maybe_unused]] SharedPtr<Scope> scope) {
    ParamList params;
    auto param = ParamNode("value", NodeValueType::Any);
    param.setIsVarArgsParam(true);
    params.addParameter(param);

    return makeShared<NativeFunction>(
        "print",
        std::move(params),
        print
    );
}

SharedPtr<NativeFunction> createFloatFunction([[maybe_unused]] SharedPtr<Scope> scope) {
    ParamList params;
    auto param = ParamNode("value", NodeValueType::Any);
    params.addParameter(param);

    return makeShared<NativeFunction>("Float", std::move(params), floatFunc);
}

SharedPtr<NativeFunction> createIntFunction([[maybe_unused]] SharedPtr<Scope> scope) {
    ParamList params;
    auto param = ParamNode("value", NodeValueType::Any);
    params.addParameter(param);

    return makeShared<NativeFunction>("Int", std::move(params), intFunc);
}


SharedPtr<NativeFunction> createStringFunction([[maybe_unused]] SharedPtr<Scope> scope) {
    ParamList params;
    auto param = ParamNode("value", NodeValueType::Any);
    params.addParameter(param);

    return makeShared<NativeFunction>("String", std::move(params), stringFunc);
}

SharedPtr<NativeFunction> createIsInstanceFunction([[maybe_unused]] SharedPtr<Scope> scope) {
    ParamList params;
    auto param = ParamNode("value", NodeValueType::Any);
    auto param2 = ParamNode("instanceString", NodeValueType::String);
    params.addParameter(param);
    params.addParameter(param2);

    return makeShared<NativeFunction>("String", std::move(params), isInstanceFunc);
}


std::unordered_map<String, NativeFuncFactory> nativeFunctionFactories = {
    {"print", createPrintFunction},
    {"Float", createFloatFunction},
    {"Int", createIntFunction},
    {"String", createStringFunction},
    {"isInstance", createIsInstanceFunction}
};




std::unordered_map<String, SharedPtr<CallableSignature>> getAllNativeFunctions(SharedPtr<Scope> scope) {
    std::unordered_map<String, SharedPtr<CallableSignature>> builtins;
    for (auto& [funcName, funcSigCall] : nativeFunctionFactories) {
        auto nativeFunc = funcSigCall(scope); // safe
        builtins[funcName] = nativeFunc->toCallableSignature();
    }
    return builtins;
}




