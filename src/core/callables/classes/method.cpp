#include "core/types.h"
#include "utilities/debugger.h"
#include "core/errors.h"
#include "core/evaluator.h"
#include "ast/exceptions.h" 
#include "ast/ast_base.h"
#include "ast/ast_class.h"
#include "ast/ast_chain.h"
#include "ast/ast_callable.h"
#include "core/callables/classes/method.h"


Method::Method(String name, ParamList params, CallableType definedType, bool requiresReturn, bool isStatic)
    : Invocable(name, params, CallableType::METHOD, requiresReturn, isStatic)
{
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    DEBUG_LOG(LogLevel::TRACE, "FuncType: ", callableTypeAsString(definedType));
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


// bool Method::getIsStatic() {return isStatic;}


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
    capturedScope->owner = "UserMethod(" + method.getName() + ")";
    setCallableType(CallableType::METHOD);
    setSubType(method.getSubType());

    if (subType == CallableType::METHOD) {throw MerkError("Method Sub Type cannot be Method from UserMethod&");}
}

SharedPtr<CallableSignature> UserMethod::toCallableSignature(SharedPtr<UserMethod> method) {
    auto sig = makeShared<CallableSignature>(method, CallableType::METHOD);
    sig->setSubType(method->getSubType());
    getCapturedScope()->owner = generateScopeOwner("MethodSignature", name);
    sig->setParameters(parameters.clone());
    return sig;
}

void UserMethod::setScope(SharedPtr<Scope> newScope) const {
    newScope->owner = generateScopeOwner("UserMethod", name);
    getBody()->setScope(newScope);
}


SharedPtr<CallableSignature> UserMethod::toCallableSignature() {
    DEBUG_FLOW(FlowLevel::HIGH);

    
    SharedPtr<CallableSignature> methodSig = makeShared<CallableSignature>(shared_from_this(), getCallableType());
    
    methodSig->setSubType(getSubType());
    
    if (methodSig->getCallableType() == CallableType::DEF) {throw MerkError("Primary Callable Type is: " + callableTypeAsString(methodSig->getCallableType()));}
    methodSig->setParameters(parameters.clone());

    // DEBUG_LOG(LogLevel::ERROR, funcSig->getParameterTypes());
    DEBUG_FLOW_EXIT();
    return methodSig;
}

Node UserMethod::execute(Vector<Node> args, SharedPtr<Scope> callScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    (void)args;
    DEBUG_FLOW(FlowLevel::HIGH);
    if (!instanceNode) {throw MerkError("An Instance In UserMethod::execute was not provided");}
    
    callScope->owner = generateScopeOwner("MethodExecutor", name);
    
    if (callScope == instanceNode->getInstanceScope()) {throw MerkError("callScope cannot be the same as instanceScope");}
    
    placeArgsInCallScope(args, callScope);
    
    try {

        if (!callScope) {throw MerkError("Method " + name +" Has No Captured Scope:");}

        auto capturedScope = getCapturedScope();

        if (!callScope) {throw MerkError("Method " + name +" Has No Call Scope:");}

        String matches = callScope == capturedScope ? "true" : "false";

        Node val = body->evaluate(callScope, instanceNode);

        DEBUG_FLOW_EXIT();
        return Node();
    } catch (const ReturnException& e) {
        auto val = e.getValue();
        DEBUG_FLOW_EXIT();      
        return val;
    }
}

CallableBody* UserMethod::getInvocableBody() {return body.get();}
CallableBody* UserMethod::getBody() const {return body.get();}
MethodBody* UserMethod::getThisBody() {return body.get();}
UniquePtr<CallableBody> UserMethod::getBody() {return static_unique_ptr_cast<CallableBody>(body->clone());}



MethodNode::MethodNode(SharedPtr<Method> method) : CallableNode(method, "Method") {
    data.type = NodeValueType::Method;
}

MethodNode::MethodNode(SharedPtr<Callable> method) : CallableNode(method, "Method") {
    data.type = NodeValueType::Method;
}




SharedPtr<Callable> MethodNode::getCallable() const {
    return std::get<SharedPtr<Method>>(data.value);
}