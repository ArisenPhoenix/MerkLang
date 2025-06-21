#include "core/functions/callable.h"
#include "core/classes/method.h"


Callable::Callable(Method& method){
    callType = method.callType;
    requiresReturn = method.requiresReturn;
    parameters = method.parameters;
    name = method.name;
    if (callableTypeAsString(callType) == "Unknown"){
        throw MerkError("Failed to instantiate callType at Callable::Callable copy from method instantiation");
    }
}

Callable::Callable(Function& function){
    callType = function.callType;
    requiresReturn = function.requiresReturn;
    parameters = function.parameters;
    name = function.name;
    if (callableTypeAsString(callType) == "Unknown"){
        throw MerkError("Failed to instantiate callType at Callable::Callable copy from function instantiation");
    }
}

Callable::Callable(Callable& callable)
    : std::enable_shared_from_this<Callable>(), // explicitly initialize the base
      name(callable.name),
      parameters(callable.parameters),
      callType(callable.callType),
      requiresReturn(callable.requiresReturn)
{
    if (callableTypeAsString(callType) == "Unknown"){
        throw MerkError("Failed to instantiate callType at Callable::Callable copy from callable instantiation");
    }
}


String& Callable::getQualifiedName() {
    String& newName = name;
    return newName;
}

Callable::Callable(String name, ParamList params, CallableType callType)
    : name(std::move(name)), parameters(std::move(params)), callType(callType) {
        if (callableTypeAsString(callType) == "Unknown"){
            throw MerkError("Failed to instantiate callType at Callable::Callable instantiation");
        }
    }

Callable::Callable(String name, ParamList params, CallableType callType, bool requiresReturn)
    : name(std::move(name)), parameters(std::move(params)), callType(callType), requiresReturn(requiresReturn) {
        if (callableTypeAsString(callType) == "Unknown"){
            throw MerkError("Failed to instantiate callType at Callable::Callable with requiresReturn instantiation");
        }
    }


String Callable::getName() const { return name; }

void Callable::setCallableType(CallableType primaryClassification) {callType = primaryClassification;} 
CallableType Callable::getCallableType() const { return callType; }


void Callable::setSubType(CallableType subClassification) {subType = subClassification;} 
CallableType Callable::getSubType() const {return subType;}



        
CallableNode::CallableNode(SharedPtr<Callable> callable, String callableType ) {
    data.type = NodeValueType::Callable;
    data.value = callable; // Stored as SharedPtr<Callable>
    nodeType = callableType + "(" + callable->name + ")";
    isCallable = true;
    name = callable->name;
}

CallableNode::CallableNode(SharedPtr<CallableNode> callableNode) {
    data.type = callableNode->data.type;
    data.value = callableNode->getValue(); // Stored as SharedPtr<Callable>
    nodeType = callableNode->nodeType + "(" + callableNode->name + ")";
    isCallable = true;
    name = callableNode->name;
}

SharedPtr<Callable> CallableNode::getCallable() const {
    return std::get<SharedPtr<Callable>>(data.value);
}

void CallableNode::setInternalScope(SharedPtr<Scope> scope) { internalScope = scope; }
SharedPtr<Scope> CallableNode::getInternalScope() const { return internalScope; }

String CallableNode::toString() const {
    return "<" + nodeType + ": " + getCallable()->toString() + ">";
}