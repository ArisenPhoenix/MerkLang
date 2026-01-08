#include "core/node/ArgumentNode.hpp"
#include "core/callables/Callable.hpp"
#include "core/callables/classes/Method.hpp"


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

Callable::Callable(Callable& callable): // explicitly initialize the base
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

void Callable::setValue(const VariantType&) {throw MerkError("Cannot Set a Value on Base Callable Class");}
NodeValueType Callable::getType() const {return NodeValueType::Callable;}
SharedPtr<NodeBase> Callable::clone() { throw MerkError("Cannot Clone Base Callable"); }
void Callable::clear() {} 
VariantType Callable::getValue() const { throw MerkError("Cannot Get a Value From Base Callable"); }

SharedPtr<NodeBase> Callable::clone() const { throw MerkError("Cannot Clone Base Callable Class");}

String Callable::getName() const { return name; }

void Callable::setCallableType(CallableType primaryClassification) {callType = primaryClassification;} 
CallableType Callable::getCallableType() const { return callType; }


void Callable::setSubType(CallableType subClassification) {subType = subClassification;} 
CallableType Callable::getSubType() const {return subType;}


CallableNode::CallableNode(Vector<SharedPtr<CallableSignature>> callableSigs, String callableType ) {
    auto instanceSpecificFlags = std::unordered_map<String, String>{
        {"isCallable", "true"},
        {"isInstance", "true"},
        {"name", callableType + "(" + callableSigs[0]->getCallable()->getName() + ")"},
        {"fullType", "Callable"},
        {"type", callableTypeAsString(callableSigs[0]->getSubType())}
    };
}

        
CallableNode::CallableNode(SharedPtr<Callable> callable, String callableType ) {
    auto instanceSpecificFlags = std::unordered_map<String, String>{
        {"isCallable", "true"},
        {"isInstance", "true"},
        {"name", callableType + "(" + callable->name + ")"},
        {"fullType", "Callable"},
        {"type", callableTypeAsString(callable->getSubType())}
    };
    auto flags = getFlags();
    flags.merge(instanceSpecificFlags);
    setValue(callable);
}

SharedPtr<CallableNode> CallableNode::clone() const {return nullptr;}
VariantType CallableNode::getValue() const {return Node::getValue();}


CallableNode::CallableNode(SharedPtr<CallableNode> callableNode) {
    auto instanceSpecificFlags = std::unordered_map<String, String>{
        {"isCallable", "true"},
        {"isInstance", "true"},
        {"name", callableNode->getTypeAsString() + "(" + callableNode->getFlags().name + ")"},
        {"fullType", "Callable"}
    };
    // setFlags()
    auto flags = getFlags();
    flags.merge(instanceSpecificFlags);
    setFlags(flags);
    setValue(callableNode->getCallable());
    if (getFlags().name.find("Method") != String::npos) {
        throw MerkError("HIT CallableNode::CallableNode(SharedPtr<CallableNode> WITH META: " + getFlags().toString());
    }
}


bool CallableNode::isInstance() { return true; }
bool CallableNode::isInstance() const { return true; }

SharedPtr<ClassInstance> CallableNode::toInstance() {
    if (isInstance()) {
        
        return Node::toInstance();
    }
    throw MerkError("Callable Should Not Pull Out ClassInstance");
}
SharedPtr<ClassInstance> CallableNode::toInstance() const {
    if (isInstance()) {
        return Node::toInstance();
    }
    throw MerkError("Callable Should Not Pull Out ClassInstance");
}


std::size_t CallableNode::hash() const {
 
    return hash(); 
}

SharedPtr<Callable> CallableNode::getCallable() const {
    return std::get<SharedPtr<Callable>>(getValue());
}

void CallableNode::setInternalScope(SharedPtr<Scope> scope) { internalScope = scope; internalScope->owner = generateScopeOwner("InternalScope", getFlags().name);}
SharedPtr<Scope> CallableNode::getInternalScope() const { return internalScope; }

String CallableNode::toString() const {
    throw MerkError("Tried to print out a Callable directly");
    // return "<" + getFlags().fullType.getBaseType() + ": " + getCallable()->toString() + ">";
}


void Callable::placeArgsInCallScope(ArgResultType evaluatedArgs, SharedPtr<Scope> callScope) const {
    DEBUG_FLOW(FlowLevel::MED);
    parameters.verifyArguments(evaluatedArgs);
    ArgumentList args;

    for (auto& evaluated: evaluatedArgs) {
        DEBUG_LOG(LogLevel::TRACE, "EVALUATED ARG: ", evaluated.toString(), " Evaluated ARG NAME: ", evaluated.getFlags().name);
        args.addPositionalArg(evaluated);
    }

    DEBUG_LOG(LogLevel::TRACE, "EVALUATED ARGS: ", args.toString());

    auto finalArgs = args.bindTo(parameters);

    DEBUG_LOG(LogLevel::TRACE, "FINAL ARGS: ", joinVectorNodeStrings(finalArgs));
    for (size_t i = 0; i < parameters.size(); ++i) {
        auto arg = finalArgs[i];
        auto flags = args.getFlags();
        auto paramVar = VarNode(finalArgs[i]);
        paramVar.setFlags(arg.getFlags());
        if (paramVar.getFlags().name.empty()) {throw MerkError("ParamVar Name is Empty WITH " + paramVar.getFlags().toString());}
        String varName = flags.name;
        if (arg.getFlags().name.empty()) {throw MerkError("Arg Is Empty");}
        DEBUG_LOG(LogLevel::TRACE, "PARAMETER's NAME:  ------------------------ > ", arg.getFlags().toString());

        callScope->declareVariable(arg.getFlags().name, makeUnique<VarNode>(arg));
    }

    DEBUG_FLOW_EXIT();
}



SharedPtr<Callable> asCallable(SharedPtr<NodeBase> callable) {
    return std::static_pointer_cast<Callable>(callable);
}