#include "core/callables/callable.h"
#include "core/callables/classes/method.h"
#include "core/callables/argument_node.h"


Callable::Callable(Method& method){
    callType = method.callType;
    requiresReturn = method.requiresReturn;
    parameters = method.parameters.clone();
    subType = method.subType;
    name = method.name;
    if (callableTypeAsString(callType) == "Unknown"){throw MerkError("Failed to instantiate callType at Callable::Callable copy from method instantiation");}
}

Callable::Callable(Function& function){
    callType = function.callType;
    subType = function.subType;
    requiresReturn = function.requiresReturn;
    parameters = function.parameters.clone();
    name = function.name;
    if (callableTypeAsString(callType) == "Unknown"){throw MerkError("Failed to instantiate callType at Callable::Callable copy from function instantiation");}
}

Callable::Callable(Callable& callable)
    : std::enable_shared_from_this<Callable>(), // explicitly initialize the base
      name(callable.name),
      parameters(callable.parameters.clone()),
      callType(callable.callType),
      subType(callable.subType),
      requiresReturn(callable.requiresReturn)
      
{   
    // if (callable.getName().size()) { throw MerkError("Constructor for callable is NULL: " + callable.getName());}
    // if (!callable.getName().size()) { throw MerkError("Constructor for callable is NULL: ");}
    if (callableTypeAsString(callType) == "Unknown") {throw MerkError("Failed to instantiate callType at Callable::Callable copy from callable instantiation");}
}


bool Callable::getIsStatic() {
    return isStatic;
}
void Callable::setIsStatic(bool updatedStatic) {
    isStatic = updatedStatic;
}
bool Callable::getRequiresReturn() {
    return requiresReturn;
}
void Callable::setRequiresReturn(bool updatedRequiresReturn) {
    requiresReturn = updatedRequiresReturn;
}


String& Callable::getQualifiedName() {
    String& newName = name;
    return newName;
}

Callable::Callable(String name, ParamList params, CallableType callType)
    : name(std::move(name)), parameters(std::move(params)), callType(callType) {
    DEBUG_FLOW(FlowLevel::MED);
    
    if (callableTypeAsString(callType) == "Unknown"){throw MerkError("Failed to instantiate callType at Callable::Callable instantiation");}
    DEBUG_FLOW_EXIT();
    }
        

Callable::Callable(String name, ParamList params, CallableType callType, bool requiresReturn, bool isStatic)
    : name(std::move(name)), parameters(std::move(params)), callType(callType), requiresReturn(requiresReturn), isStatic(isStatic) {
    DEBUG_FLOW(FlowLevel::MED);
    if (callableTypeAsString(callType) == "Unknown"){throw MerkError("Failed to instantiate callType at Callable::Callable with requiresReturn instantiation");}
    DEBUG_FLOW_EXIT();
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
    // data.fullType;
}

CallableNode::CallableNode(SharedPtr<CallableNode> callableNode) {
    data.type = callableNode->data.type;
    data.value = callableNode->getValue(); // Stored as SharedPtr<Callable>
    nodeType = callableNode->nodeType + "(" + callableNode->name + ")";
    isCallable = true;
    name = callableNode->name;
    data.fullType = callableNode->getFullType();
}

SharedPtr<Callable> CallableNode::getCallable() const {
    return std::get<SharedPtr<Callable>>(data.value);
}

void CallableNode::setInternalScope(SharedPtr<Scope> scope) { internalScope = scope; internalScope->owner = generateScopeOwner("InternalScope", name);}
SharedPtr<Scope> CallableNode::getInternalScope() const { return internalScope; }

String CallableNode::toString() const {return "<" + nodeType + ": " + getCallable()->toString() + ">";}


void Callable::placeArgsInCallScope(ArgResultType evaluatedArgs, SharedPtr<Scope> callScope) const {
    DEBUG_FLOW(FlowLevel::MED);
    parameters.verifyArguments(evaluatedArgs);
    ArgumentList args;

    for (auto& evaluated: evaluatedArgs) {
        args.addPositionalArg(evaluated);
    }

    // DEBUG_LOG(LogLevel::PERMISSIVE, "========================================================== ARGS ADDED TO CALLABLE", args.toString());
    auto finalArgs = args.bindTo(parameters);
    
    for (size_t i = 0; i < parameters.size(); ++i) {
        VarNode paramVar(finalArgs[i]);
        String varName = parameters[i].getName();
        if (getCallableType() == CallableType::METHOD && varName == "newValue" && callScope->hasVariable(varName)) {
            callScope->debugPrint();
            throw MerkError("Attempted to redeclare " + parameters[i].getName());
        }
        callScope->declareVariable(varName, makeUnique<VarNode>(paramVar));
    }

    DEBUG_FLOW_EXIT();
}



