#pragma once
#include "core/TypesFWD.hpp"
#include "core/node/Node.hpp"
#include "core/node/NodeStructures.hpp"
#include "core/callables/classes/ClassBase.hpp"


class Chain;
class ParamList;
class Scope;
class MethodDef;


Node handleVirtualMethod(SharedPtr<ClassInstanceNode> instanceNode, const String& methodName);
Node handleVirtualMethod(Node currentVal, const String& methodName, NodeList args);

bool handleChain(Chain* chain, ParamList params, String accessor, String name, SharedPtr<Scope> classScope);
Vector<Chain*> applyAccessorScopeFix(MethodDef* methodDef, SharedPtr<Scope> classScope, const String& accessor);
void fixupClassChains(SharedPtr<Scope> classScope, String accessor);
void stripImplicitAccessor(MethodDef* methodDef, const String& accessor); 