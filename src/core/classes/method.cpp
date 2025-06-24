#include "core/types.h"
#include "utilities/debugger.h"
#include "core/errors.h"
#include "core/evaluator.h"
#include "ast/exceptions.h" 
#include "ast/ast_class.h"
#include "ast/ast_chain.h"
#include "ast/ast_callable.h"
#include "ast/ast_callable.h"
#include "core/classes/method.h"


Method::Method(String name, ParamList params, UniquePtr<MethodBody> body, SharedPtr<Scope> scope, CallableType callType, bool requiresReturn)
    : Callable(std::move(name), std::move(params), CallableType::METHOD, requiresReturn), body(std::move(body)), capturedScope(scope)
{
    DEBUG_LOG(LogLevel::TRACE, "Method created: ", getName());
    setSubType(callType);
}

Method::Method(Function&& function)
    : Callable(function.getName(), std::move(function.parameters), CallableType::METHOD, false) {
    body = std::make_unique<MethodBody>(function.getCapturedScope());

    for (auto& child : function.getBody()->getChildren()){
        body->addChild(child->clone());
    }

    setCapturedScope(function.getCapturedScope());
    setCallableType(CallableType::METHOD);
    setSubType(function.getSubType());
}

Method::Method(Method& method) : Callable(method) {
    body = static_unique_ptr_cast<MethodBody>(method.body->clone());
    capturedScope = method.capturedScope;
    capturedScope->owner = "Method(" + method.getName() + ")";
    setCallableType(CallableType::METHOD);
    setSubType(method.getSubType());
}

SharedPtr<CallableSignature> Method::toCallableSignature(SharedPtr<Method> method) {
    auto sig = makeShared<CallableSignature>(method, CallableType::METHOD);
    sig->setSubType(method->getSubType());
    getCapturedScope()->owner = generateScopeOwner("MethodSignature", name);
    sig->setParameters(parameters.clone());
    return sig;
}

void Method::setScope(SharedPtr<Scope> newScope) const {
    getBody()->setScope(newScope);
}


SharedPtr<CallableSignature> Method::toCallableSignature() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    
    SharedPtr<CallableSignature> methodSig = makeShared<CallableSignature>(
        shared_from_this(), getCallableType()
    );
    
    methodSig->setSubType(getSubType());
    
    if (methodSig->getCallableType() == CallableType::DEF) {
        throw MerkError("Primary Callable Type is: " + callableTypeAsString(methodSig->getCallableType()));
    }
    methodSig->setParameters(parameters.clone());

    // DEBUG_LOG(LogLevel::ERROR, funcSig->getParameterTypes());
    DEBUG_FLOW_EXIT();
    return methodSig;
}

Node Method::execute(Vector<Node> args, SharedPtr<Scope> callScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!instanceNode) {throw MerkError("An Instance In Method::execute was not provided");}

    callScope->owner = generateScopeOwner("MethodExecutor", name);
    
    placeArgsInCallScope(args, callScope);
    
    try {

        if (!callScope){throw MerkError("Method " + name +" Has No Captured Scope:");}

        auto capturedScope = getCapturedScope();

        if (!callScope){throw MerkError("Method " + name +" Has No Call Scope:");}

        String matches = callScope == capturedScope ? "true" : "false";

        Node val = body->evaluate(callScope, instanceNode);
        DEBUG_FLOW_EXIT();
        return val;
    } catch (const ReturnException& e) {
        DEBUG_FLOW_EXIT();
        return e.getValue();
    }
}

void Method::setCapturedScope(SharedPtr<Scope> scope) {
    if (!scope) {
        throw MerkError("Cannot set a null scope in Method::setCapturedScope.");
    }
    capturedScope = scope;
    capturedScope->owner = generateScopeOwner("MethodCapturedScope", name);
    body->setScope(capturedScope);
}

SharedPtr<Scope> Method::getCapturedScope() const {
    return capturedScope;
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


bool Method::getIsStatic() {return isStatic;}
Vector<Chain*> Method::getNonStaticElements() {
    return getBody()->getNonStaticElements();
    
}


String Method::toString() const {
    std::ostringstream params;
    for (auto& param : parameters) {
        params << param.toShortString();
    }

    return "Method<"+name+">(" + params.str() + ")" ;

}

MethodBody* Method::getBody() {return body.get();}

MethodBody* Method::getBody() const {return body.get();}










MethodNode::MethodNode(SharedPtr<Method> method) : CallableNode(method, "Method") {
    data.type = NodeValueType::Method;
}

MethodNode::MethodNode(SharedPtr<Callable> method) : CallableNode(method, "Method") {
    data.type = NodeValueType::Method;
}




SharedPtr<Callable> MethodNode::getCallable() const {
    return std::get<SharedPtr<Method>>(data.value);
}