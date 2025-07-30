#ifndef ALL_BUILTIN_H
#define ALL_BUILTIN_H


#include "core/types.h"
#include "core/callables/classes/bultins.h"
#include "core/callables/functions/builtins.h"

class Scope;


std::unordered_map<String, SharedPtr<ClassBase>> getNativeClasses(SharedPtr<Scope>);


std::unordered_map<String, SharedPtr<CallableSignature>> getNativeFunctions(SharedPtr<Scope>);


#endif //ALL_BUILTIN_H