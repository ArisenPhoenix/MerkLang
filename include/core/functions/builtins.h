#include "core/functions/native_function.h"
#include "core/functions/function_node.h"
#include "core/functions/param_node.h"

class Scope;
class Callable;

using NativeFuncFactory = std::function<SharedPtr<NativeFunction>(SharedPtr<Scope>)>;
SharedPtr<NativeFunction> createPrintFunction(SharedPtr<Scope>);
std::unordered_map<String, SharedPtr<CallableSignature>> getAllNativeFunctions(SharedPtr<Scope>);

