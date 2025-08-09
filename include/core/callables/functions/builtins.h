#ifndef FUNCTION_BUILTIN_H
#define FUNCTION_BUILTIN_H

#include "core/callables/functions/native_function.h"

class Scope;
class Callable;

using NativeFuncFactory = std::function<SharedPtr<NativeFunction>(SharedPtr<Scope>)>;
std::unordered_map<String, SharedPtr<CallableSignature>> getAllNativeFunctions(SharedPtr<Scope>);



#endif //FUNCTION_BUILTIN_H