#pragma once

#include "core/callables/classes/NativeClass.hpp"

using NativeClassFactory = std::function<SharedPtr<NativeClass>(SharedPtr<Scope>)>;
std::unordered_map<String, SharedPtr<ClassBase>> getAllNativeClasses(SharedPtr<Scope>);
