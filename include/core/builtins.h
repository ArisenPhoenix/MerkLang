#pragma once


// #include "core/types.h"
#include "core/TypesFWD.hpp"
#include "core/callables/classes/bultins.h"
#include "core/callables/functions/builtins.h"

class Scope;


std::unordered_map<String, SharedPtr<ClassBase>> getNativeClasses(SharedPtr<Scope>);


std::unordered_map<String, SharedPtr<CallableSignature>> getNativeFunctions(SharedPtr<Scope>);