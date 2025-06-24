#include "core/types.h"
#include "core/classes/bultins.h"
#include "core/functions/builtins.h"

class Scope;


std::unordered_map<String, SharedPtr<CallableSignature>> getNativeClasses(SharedPtr<Scope>);


std::unordered_map<String, SharedPtr<CallableSignature>> getNativeFunctions(SharedPtr<Scope>);
