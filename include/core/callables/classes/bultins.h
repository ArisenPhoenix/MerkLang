#ifndef CLASS_BUILTIN_H
#define CLASS_BUILTIN_H

#include "core/callables/classes/NativeClass.hpp"

using NativeClassFactory = std::function<SharedPtr<NativeClass>(SharedPtr<Scope>)>;
std::unordered_map<String, SharedPtr<ClassBase>> getAllNativeClasses(SharedPtr<Scope>);



#endif //CLASS_BUILTIN_H