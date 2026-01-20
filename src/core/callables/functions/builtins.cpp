#include "core/node/Node.hpp"
#include "core/node/ParamNode.hpp"
#include "core/node/ArgumentNode.hpp"
#include "core/node/NodeStructures.hpp"

#include "core/types.h"
#include "core/callables/Callable.hpp"
#include "core/callables/functions/NativeFunction.hpp"
#include "core/callables/functions/Function.hpp"
#include "core/callables/functions/builtins.h"
#include "core/Scope.hpp"

// Define native functions 

Node print(ArgumentList args, [[maybe_unused]] SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) {
    // throw MerkError("Showed Print Node to String");
    for (size_t i = 0; i < args.size(); ++i) {
        // DEBUG_LOG(LogLevel::PERMISSIVE, args[i].toString(), "  META", args[i].getFlags().toString());
        
        // DEBUG_LOG(LogLevel::PERMISSIVE, args[i].getTypeAsString());
        std::cout << args[i].toString();
        if (i < args.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;
    return Node(Null); // null
}

Node debug_log(ArgumentList args, [[maybe_unused]] SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) {
    DEBUG_LOG(LogLevel::DEBUG, "ALL ARGS: ", args.toString());
    debugLog(true, highlight("\n====================================== DEBUG_LOG MERK INTERNALS BEGIN ======================================\n", Colors::bg_bright_red));
    for (size_t i = 0; i < args.size(); ++i) {
            auto arg = args[i];
            DEBUG_LOG(LogLevel::DEBUG, highlight("ARG: ", Colors::bg_bright_red), arg.toString(), highlight(" ARG META: ", Colors::bg_blue), arg.getFlags().toString());
            std::cout << args[i].toString();
            if (i < args.size() - 1) std::cout << " ";
        }
    std::cout << std::endl;
    auto first = args[0];
    if (instanceNode) {
        auto instScope = instanceNode->getInstanceScope();
        debugLog(true, highlight("The instance scope internals", Colors::bg_green));
        instScope->debugPrint();

        debugLog(true, highlight("The instance scope children", Colors::bg_green));
        instScope->printChildScopes();
    }
    if (first.isBool() && first.toBool()) {
        throw MerkError(highlight("DEBUG_LOG threw because first arg was a boolean of " + first.toString(), Colors::bg_magenta));
    }
    debugLog(true, highlight("\n====================================== DEBUG_LOG MERK INTERNALS END ======================================\n", Colors::bg_bright_red));
    return Node(Null); // none
}

// Node floatFunc(ArgumentList args, [[maybe_unused]] SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) {
//     if (args.size() != 1) {throw MerkError("Only One Argument may be passed to Function 'Float;");}
//     return Node(args[0].toFloat());
// }

Node intFunc(ArgumentList args, [[maybe_unused]] SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) {
    if (args.size() != 1) {throw MerkError("Only One Argument may be passed to Function 'Float;");}
    return Node(DynamicNode::forceTo<int>(args[0]));
}

Node stringFunc(ArgumentList args, [[maybe_unused]] SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) {
    if (args.size() != 1) {throw MerkError("Only One Argument may be passed to Function 'Float;");}
    return Node(args[0].toString());
}

Node isInstanceFunc(ArgumentList args, [[maybe_unused]] SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) {
    if (args.size() != 2) {throw MerkError("Only One Argument may be passed to Function 'Float;");}

    auto var = args[0];
    auto instance = args[1];

    if (var.isInstance()) {
        auto inst = std::get<SharedPtr<Callable>>(var.getValue());
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

SharedPtr<NativeFunction> createDebugLogFunction([[maybe_unused]] SharedPtr<Scope> scope) {
    ParamList params;
    auto param = ParamNode("value", NodeValueType::Any);
    param.setIsVarArgsParam(true);
    params.addParameter(param);

    return makeShared<NativeFunction>(
        "DEBUG_LOG",
        std::move(params),
        debug_log
    );
}

// SharedPtr<NativeFunction> createFloatFunction([[maybe_unused]] SharedPtr<Scope> scope) {
//     ParamList params;
//     auto param = ParamNode("value", NodeValueType::Any);
//     params.addParameter(param);

//     return makeShared<NativeFunction>("Float", std::move(params), floatFunc);
// }

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
    // {"Float", createFloatFunction},
    {"Int", createIntFunction},
    {"String", createStringFunction},
    {"isInstance", createIsInstanceFunction},
    {"DEBUG_LOG", createDebugLogFunction}
};




std::unordered_map<String, SharedPtr<CallableSignature>> getAllNativeFunctions(SharedPtr<Scope> scope) {
    std::unordered_map<String, SharedPtr<CallableSignature>> builtins;
    for (auto& [funcName, funcSigCall] : nativeFunctionFactories) {
        auto nativeFunc = funcSigCall(scope); // safe
        builtins[funcName] = nativeFunc->toCallableSignature();
    }
    return builtins;
}




