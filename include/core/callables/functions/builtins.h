#ifndef FUNCTION_BUILTIN_H
#define FUNCTION_BUILTIN_H

#include "core/callables/functions/NativeFunction.hpp"

class Scope;
class Callable;

using NativeFuncFactory = std::function<SharedPtr<NativeFunction>(SharedPtr<Scope>)>;
std::unordered_map<String, SharedPtr<CallableSignature>> getAllNativeFunctions(SharedPtr<Scope>);



#endif //FUNCTION_BUILTIN_H