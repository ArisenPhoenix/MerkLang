#include "core/types.h"
#include "core/node/Node.hpp"
#include "core/node/NodeStructures.hpp"
#include "core/callables/classes/ClassBase.hpp"

Node handleVirtualMethod(SharedPtr<ClassInstanceNode> instanceNode, const String& methodName) {
    if (methodName == "clone") {
    
        // auto clonedInstance = instanceNode->clone();
        // if (clonedInstance->getInstance() == instanceNode->getInstance()) {
        //     throw MerkError("InstanceNode didn't actually clone");
        // }

        // return Node(clonedInstance->getInstanceNode());
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


Node handleVirtualMethod(Node currentVal, const String& methodName) {
    if (methodName == "clone") {
        auto clonedVal = currentVal.clone();
        return clonedVal;


    } else if (methodName == "clear") {
        currentVal.clear();
        return Node(Null);
    } 

    throw FunctionNotFoundError(methodName);
}