#pragma once
#include "core/types.h"
#include "core/Scope.hpp"

class Chain;
class ParamList;
class Scope;
class MethodDef;


bool handleChain(Chain* chain, ParamList params, String accessor, String name, SharedPtr<Scope> classScope);

Vector<Chain*> applyAccessorScopeFix(MethodDef* methodDef, SharedPtr<Scope> classScope, const String& accessor);

void fixupClassChains(SharedPtr<Scope> classScope, String accessor);

void stripImplicitAccessor(MethodDef* methodDef, const String& accessor); 