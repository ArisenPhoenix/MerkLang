#include "core/types.h"
#include "core/builtins.h"
#include "core/TypeBuiltins.hpp"
// #include "core/Scope.hpp"


std::unordered_map<String, SharedPtr<ClassBase>> getNativeClasses(SharedPtr<Scope> scope) {
    return getAllNativeClasses(scope);
}


std::unordered_map<String, SharedPtr<CallableSignature>> getNativeFunctions(SharedPtr<Scope> scope) {
    return getAllNativeFunctions(scope);
}

void getNativeTypes(SharedPtr<Scope> scope) {
    return registerBuiltinTypes(scope);
}