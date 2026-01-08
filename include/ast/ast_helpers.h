#pragma once
#include "core/TypesFWD.hpp"
#include "core/node/Node.hpp"
#include "core/node/NodeStructures.hpp"
#include "core/callables/classes/ClassBase.hpp"

Node handleVirtualMethod(SharedPtr<ClassInstanceNode> instanceNode, const String& methodName);


Node handleVirtualMethod(Node currentVal, const String& methodName);

