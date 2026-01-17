#pragma once
#include "core/TypesFWD.hpp"
#include "core/types/Type.hpp" // your new TypeRegistry/TypeNode

class Scope;

// Seed builtin type visibility into the provided scope's TypeRegistry.
// (Usually called once on root scope startup.)
void registerBuiltinTypes(SharedPtr<Scope> scope);
