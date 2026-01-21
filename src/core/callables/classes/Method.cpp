#include "core/node/Node.hpp"
#include "core/node/ArgumentNode.hpp"

#include "core/types.h"
#include "utilities/debugger.h"
#include "core/errors.h"
#include "core/evaluators/FlowEvaluator.hpp"
#include "ast/Exceptions.hpp" 
#include "ast/AstBase.hpp"
#include "ast/AstClass.hpp"
#include "ast/AstChain.hpp"
#include "ast/AstCallable.hpp"
#include "core/callables/classes/Method.hpp"
#include "core/evaluators/Executor.hpp"

Method::Method(String name, ParamList params, CallableType definedType, bool requiresReturn, bool isStatic)
    : Invocable(name, params, CallableType::METHOD, requiresReturn, isStatic)
{
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    setSubType(definedType);
    DEBUG_FLOW_EXIT();
}

String Method::getAccessor() {return accessor;}
void Method::setAccessor(String access) {accessor = access;}

SharedPtr<Scope> Method::getClassScope() const {return classScope;}
void Method::setClassScope(SharedPtr<Scope> newClassScope) {
    if (!newClassScope) {
        throw MerkError("Cannot Set newClassScope to a null Scope");
    }
    classScope = newClassScope;
}

Method::~Method() {
    if (capturedScope) {
        capturedScope->clear();
    }
}

Vector<Chain*> UserMethod::getNonStaticElements() {
    return getThisBody()->getNonStaticElements();
}

String UserMethod::toString() const {
    std::ostringstream params;
    for (auto& param : parameters) {
        params << param.toShortString();
    }
    return "UserMethod<"+name+">(" + params.str() + ")" ;
}

UserMethod::UserMethod(String name, ParamList params, UniquePtr<MethodBody> body, SharedPtr<Scope> scope, CallableType callType)
    : Method(std::move(name), std::move(params), CallableType::METHOD, false, false), body(std::move(body))
{
    DEBUG_FLOW(FlowLevel::HIGH);
    DEBUG_LOG(LogLevel::TRACE, "Method created: ", getName());
    setCapturedScope(scope);
    setSubType(callType);
    if (subType == CallableType::METHOD) {throw MerkError("Method Sub Type cannot be Method from Full Constructor");}
}

UserMethod::UserMethod(Function&& function)
    : Method(function.getName(), std::move(function.parameters), CallableType::METHOD, false) {
    DEBUG_FLOW(FlowLevel::HIGH);

    body = std::make_unique<MethodBody>(function.getCapturedScope());

    for (auto& child : function.getBody()->getChildren()){
        body->addChild(child->clone());
    }

    setCapturedScope(function.getCapturedScope());
    setCallableType(CallableType::METHOD);
    setSubType(function.getSubType());
    if (subType == CallableType::METHOD) {throw MerkError("Method Sub Type cannot be Method from Function&");}
}

UserMethod::UserMethod(UserMethod& method) : Method(method) {
    DEBUG_FLOW(FlowLevel::HIGH);

    body = static_unique_ptr_cast<MethodBody>(method.body->clone());
    capturedScope = method.capturedScope;
    setCallableType(CallableType::METHOD);
    setSubType(method.getSubType());

    if (subType == CallableType::METHOD) {throw MerkError("Method Sub Type cannot be Method from UserMethod&");}
}

SharedPtr<CallableSignature> UserMethod::toCallableSignature(SharedPtr<UserMethod> method) {
    auto sig = makeShared<CallableSignature>(method, CallableType::METHOD);
    sig->setSubType(method->getSubType());
    sig->setParameters(parameters.clone());
    return sig;
}

void UserMethod::setScope(SharedPtr<Scope> newScope) const {
    MARK_UNUSED_MULTI(newScope);
}


SharedPtr<CallableSignature> UserMethod::toCallableSignature() {
    DEBUG_FLOW(FlowLevel::HIGH);
    SharedPtr<CallableSignature> methodSig = makeShared<CallableSignature>(shared_from_this(), getCallableType());
    
    methodSig->setSubType(getSubType());
    
    if (methodSig->getCallableType() == CallableType::DEF) {throw MerkError("Primary Callable Type is: " + callableTypeAsString(methodSig->getCallableType()));}
    methodSig->setParameters(parameters.clone());
    DEBUG_FLOW_EXIT();
    return methodSig;
}


Node nonFlowHandler(SharedPtr<Scope> callScope, String name, SharedPtr<Scope> capturedScope, CallableBody* body, SharedPtr<ClassInstanceNode> instanceNode) {   
    try {
        DEBUG_LOG(LogLevel::TRACE, "In try block");
        if (!callScope) {throw MerkError("Method " + name +" Has No Call Scope:");}

        String matches = callScope == capturedScope ? "true" : "false";

        Node val = body->evaluate(callScope, instanceNode);
        
        DEBUG_FLOW_EXIT();
        return val;
    } catch (const ReturnException& e) {
        auto val = e.getValue();
        DEBUG_LOG(LogLevel::TRACE, "METHOD " + name + " RETURNED: " + val.getFlags().toString());
        DEBUG_FLOW_EXIT();      
        return val;
    }
}


Node UserMethod::execute(ArgumentList args, SharedPtr<Scope> callScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    return Executor::Method(name, callScope, getCapturedScope(), requiresReturn, args, body.get(), parameters, instanceNode);
    // if (!instanceNode) {throw MerkError("An Instance In UserMethod::execute was not provided");}
    // DEBUG_LOG(LogLevel::TRACE, "Validated Instance Node");
    // callScope->owner = generateScopeOwner("MethodExecutor", name);
    
    // if (callScope == instanceNode->getInstanceScope()) {throw MerkError("callScope cannot be the same as instanceScope");}
    // DEBUG_LOG(LogLevel::TRACE, "Placing Args in Call Scope");
    // placeArgsInCallScope(args, callScope);

    
    // bool executeFlow = true;
    // if (executeFlow) {
    //     EvalResult r = body->evaluateFlow(callScope, instanceNode);
    //     if (r.isReturn()) {return r.value;}
    //     if (r.isThrow())  {throw RunTimeError("Unhandled throw");}
    //     if (r.isBreak() || r.isContinue()) throw MerkError("break/continue used outside loop");

    //     if (requiresReturn) throw MerkError("Method did not return a value.");
    //     return Node();
    // }
    // return nonFlowHandler(callScope, getName(), getCapturedScope(), body.get(), instanceNode);
    
}


CallableBody* UserMethod::getInvocableBody() {return body.get();}
CallableBody* UserMethod::getBody() const {return body.get();}
MethodBody* UserMethod::getThisBody() {return body.get();}
UniquePtr<CallableBody> UserMethod::getBody() {return static_unique_ptr_cast<CallableBody>(body->clone());}

MethodNode::~MethodNode() {
    clear();
}

MethodNode::MethodNode(SharedPtr<Method> method) : CallableNode(method, "Method") {
    getFlags().type = NodeValueType::Method;
}

MethodNode::MethodNode(SharedPtr<Callable> method) : CallableNode(method, "Method") {
    getFlags().type = NodeValueType::Method;
}

SharedPtr<Callable> MethodNode::getCallable() const {
    return std::get<SharedPtr<Callable>>(getValue());
}