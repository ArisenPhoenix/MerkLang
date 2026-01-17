
#include "core/node/Node.hpp"
#include "core/node/ParamNode.hpp"
#include "core/node/ArgumentNode.hpp"

#include "core/types.h"
#include "utilities/debugger.h"

#include "core/Scope.hpp" 
#include "ast/Ast.hpp"
#include "ast/Exceptions.hpp"
#include "ast/AstControl.hpp"

#include "ast/AstFunction.hpp"

#include "core/callables/Invocable.hpp"

#include "ast/AstCallable.hpp"




Function::Function(String name, ParamList params, [[maybe_unused]] CallableType funcType, bool requiresReturn, bool isStatic)
    : Invocable(name, params, CallableType::FUNCTION, requiresReturn, isStatic)
{
    DEBUG_FLOW(FlowLevel::NONE);
    // DEBUG_LOG(LogLevel::TRACE, "FuncType: ", callableTypeAsString(funcType));
    DEBUG_FLOW_EXIT();
}

UserFunction::UserFunction(String name, UniquePtr<FunctionBody> body, ParamList parameters, CallableType funcType)
    : Function(name, parameters, CallableType::FUNCTION), body(std::move(body)) {
        DEBUG_FLOW(FlowLevel::NONE);

        setSubType(funcType);
        setCallableType(CallableType::FUNCTION);
        DEBUG_FLOW_EXIT();
}



Node UserFunction::execute(ArgumentList args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    // (void)args;
    // DEBUG_FLOW(FlowLevel::NONE);
    // if (!scope){throw MerkError("UserFunction::execute -> Starting Scope Null in: ");}
    // placeArgsInCallScope(args, scope);

    // DEBUG_FLOW_EXIT();
    // return body->evaluate(scope, instanceNode);

    DEBUG_FLOW(FlowLevel::NONE);
    if (!scope){throw MerkError("UserFunction::execute -> Starting Scope Null in: ");}
    placeArgsInCallScope(args, scope);

    DEBUG_FLOW_EXIT();
    EvalResult r = body->evaluateFlow(scope, instanceNode);

    if (r.isReturn()) return r.value;
    if (r.isThrow())  throw RunTimeError("Unhandled throw"); // or convert to your error model
    if (r.isBreak() || r.isContinue()) throw MerkError("break/continue used outside loop");

    // no explicit return
    if (requiresReturn) throw MerkError("Function did not return a value.");
    return Node();
}

// Node UserFunction::executeFlow(ArgumentList args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
//     DEBUG_FLOW(FlowLevel::NONE);
//     if (!scope){throw MerkError("UserFunction::execute -> Starting Scope Null in: ");}
//     placeArgsInCallScope(args, scope);

//     DEBUG_FLOW_EXIT();
//     EvalResult r = body->evaluateFlow(scope, instanceNode);

//     if (r.isReturn()) return r.value;
//     if (r.isThrow())  throw RunTimeError("Unhandled throw"); // or convert to your error model
//     if (r.isBreak() || r.isContinue()) throw MerkError("break/continue used outside loop");

//     // no explicit return
//     if (requiresReturn) throw MerkError("Function did not return a value.");
//     return Node();
// }

FunctionBody::~FunctionBody(){if (getScope()) {DEBUG_LOG(LogLevel::TRACE, highlight("Destroying FunctionBody", Colors::orange)); getScope().reset();}} 

SharedPtr<CallableSignature> UserFunction::toCallableSignature() {
    DEBUG_FLOW(FlowLevel::NONE);

    SharedPtr<CallableSignature> funcSig = makeShared<CallableSignature>(
        shared_from_this(), getCallableType()
    );

    funcSig->setSubType(getSubType());

    if (funcSig->getCallableType() == CallableType::DEF) {throw MerkError("Primary Callable Type is: " + callableTypeAsString(funcSig->getCallableType()));}

    funcSig->setParameters(parameters.clone());
 
    DEBUG_FLOW_EXIT();
    return funcSig;
}

void UserFunction::setScope(SharedPtr<Scope> newScope) const {
    MARK_UNUSED_MULTI(newScope);
    DEBUG_FLOW(FlowLevel::NONE);
    // if (!newScope) {throw MerkError("No newScope in UserFunction::setScope");}
    // newScope->owner = generateScopeOwner("UserFunction", name);
    // auto children = body->getAllAst();
    // body->setScope(newScope);

    DEBUG_FLOW_EXIT();
}

void UserFunction::setCapturedScope(SharedPtr<Scope> newScope) {
    capturedScope = newScope;
    // setScope(newScope);
}

String UserFunction::toString() const {return "<Function: " + getName() + ">";}


FunctionNode::FunctionNode(SharedPtr<Function> function) : CallableNode(function, "Function") {getFlags().type = NodeValueType::Function;}


FunctionNode::FunctionNode(String originalName, Vector<SharedPtr<CallableSignature>> functionSigs) : CallableNode(functionSigs, "CallableSignature") {
    auto first = functionSigs[0];
    auto instanceSpecificFlags = std::unordered_map<String, String>{
        {"isCallable", "true"},
        {"isInstance", "false"},
        {"name", originalName},
        {"fullType", "Function"}
    };
    setFlags(getFlags().merge(instanceSpecificFlags));
    setValue(functionSigs);
    if (getFlags().name.find("Method") != String::npos) {
        throw MerkError("HIT CallableNode::CallableNode(SharedPtr<CallableNode> WITH META: " + getFlags().toString());
    }
}

String FunctionNode::toString() const {
    return "<FunctionRef " + std::to_string(std::get<Vector<SharedPtr<CallableSignature>>>(getValue()).size()) + " overload(s)>";
}

// FunctionNode::FunctionNode(SharedPtr<Callable> function) : CallableNode(function, "Function") {data.type = NodeValueType::Function;}

SharedPtr<Callable> FunctionNode::getCallable() const {return std::get<SharedPtr<Callable>>(getValue());}


CallableBody* UserFunction::getInvocableBody() {return body.get();}
CallableBody* UserFunction::getBody() const {return body.get();}
FunctionBody* UserFunction::getThisBody() const {return body.get();}
UniquePtr<CallableBody> UserFunction::getBody() {return static_unique_ptr_cast<CallableBody>(body->clone());}


SharedPtr<CallableSignature> FunctionNode::getFunction(String name, ArgumentList args) {
    if (!std::holds_alternative<Vector<SharedPtr<CallableSignature>>>(getValue())) {
        throw MerkError("FunctionNode holds type " + nodeTypeToString(DynamicNode::getTypeFromValue(getValue())));
    }
    auto funcSigs = std::get<Vector<SharedPtr<CallableSignature>>>(getValue());
    
    Vector<NodeValueType> argTypes;
    for (const auto &arg : args) { argTypes.push_back(arg.getType()); }

    if (funcSigs.size() == 0) { throw MerkError("There Were No Function signatures to pull from"); }
    for (auto funcSig : funcSigs) {
        if (funcSig->getSubType() == CallableType::DEF) {  
            return funcSig;
        }
        if (funcSig->getSubType() == CallableType::NATIVE) {
            if (funcSig->matches(argTypes)) {
                return funcSig;
            }
        }
        DEBUG_LOG(LogLevel::TRACE, "Checking Function Candidate", name, funcSig->getCallable()->parameters.toString());

        // If this is a def function, return it regardless.
        DEBUG_LOG(LogLevel::TRACE, "Function Type: ", callableTypeAsString(funcSig->getSubType()));
        
        if (funcSig->getSubType() == CallableType::FUNCTION) {

            if (funcSig->matches(argTypes)){
                return funcSig;
            }
            DEBUG_LOG(LogLevel::TRACE, "Candidate ", name, " skipped: subtype=", callableTypeAsString(funcSig->getSubType()));
        }
    }
    

    throw MerkError("FunctionRef " + name + " Was Not Found");
    
}
