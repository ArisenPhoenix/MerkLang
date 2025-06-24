#include "core/functions/native_function.h"
#include "core/functions/function_node.h"
#include "core/functions/param_node.h"


// Define native functions
// SharedPtr<Callable> createPrintFunction(SharedPtr<Scope> scope);

// using NativeFuncFactory = std::function<SharedPtr<Callable>(SharedPtr<Scope>)>;
// using NativeFuncFactory = std::function<SharedPtr<Callable>()>;
// std::unordered_map<String, NativeFuncFactory> nativeFunctionFactories;
// Vector<SharedPtr<CallableNode>> getNativeFunctions(SharedPtr<Scope> scope); 


class Scope;
class Callable;

using NativeFuncFactory = std::function<SharedPtr<NativeFunction>(SharedPtr<Scope>)>;
// using NativeFuncFactory = std::function<SharedPtr<NativeFunction>()>;


SharedPtr<NativeFunction> createPrintFunction(SharedPtr<Scope>);

std::unordered_map<String, SharedPtr<CallableSignature>> getNativeFunctions(SharedPtr<Scope>);
// Vector<std::unordered_map<String, SharedPtr<CallableSignature>>> getNativeFunctions(SharedPtr<Scope> scope);


// using NativeFuncFactory = std::function<std::pair<String, SharedPtr<CallableSignature>>()>;
