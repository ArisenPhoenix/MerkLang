// #include "core/types.h"
#include "core/TypesFWD.hpp"
#include "core/node/Node.hpp"
#include "core/node/NodeStructures.hpp"
#include "core/callables/classes/ClassBase.hpp"

Node handleVirtualMethod(SharedPtr<ClassInstanceNode> instanceNode, const String& methodName) {
    if (methodName == "clone") {
        auto clonedInstance = instanceNode->getInstanceNode().cloneInstance();
        if (clonedInstance->toInstance() == instanceNode->getInstance()) {
            throw MerkError("InstanceNode didn't actually clone");
        }
        return ClassInstanceNode(clonedInstance->toInstance());
    } 
        
    else if (methodName == "clear") {
        instanceNode->clear();
        return Node(Null);
    } else if (methodName == "length") {
        if (auto data = instanceNode->getInstance()->getNativeData()) {
            return Node(data->length());
        }
    }

    throw FunctionNotFoundError(methodName);

}


Node handleVirtualMethod(Node currentVal, const String& methodName, NodeList args = {}) {
    if (methodName == "clone") {
        auto clonedVal = currentVal.clone();
        return clonedVal;
 

    } else if (methodName == "clear") {
        currentVal.clear();
        return Node(Null);
    }

    else if (methodName == "sub") {
        if (currentVal.isString()) {
            return Node(currentVal.toString().find(args[0].toString()) != String::npos);
        }
        throw MerkError("Method Sub not implemented for type: " + currentVal.getTypeAsString());
    }

    throw FunctionNotFoundError(methodName);
}