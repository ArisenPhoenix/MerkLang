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
    // body = std::move(method.body);
    // body = makeUnique<MethodBody>(*method.body); // Deep copy
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
    return sig;
}

void Method::setScope(SharedPtr<Scope> newScope) const {
    getBody()->setScope(newScope);
}


SharedPtr<CallableSignature> Method::toCallableSignature() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    // DEBUG_LOG(LogLevel::ERROR, "Callable Type: ", callableTypeAsString(this->callType));
    // DEBUG_LOG(LogLevel::ERROR, "subType: ", callableTypeAsString(getSubType()));
    
    SharedPtr<CallableSignature> methodSig = makeShared<CallableSignature>(
        shared_from_this(), getCallableType()
    );
    
    methodSig->setSubType(getSubType());
    
    if (methodSig->getCallableType() == CallableType::DEF) {
        throw MerkError("Primary Callable Type is: " + callableTypeAsString(methodSig->getCallableType()));
    }
    
    // DEBUG_LOG(LogLevel::ERROR, funcSig->getParameterTypes());
    DEBUG_FLOW_EXIT();
    return methodSig;
}

Node Method::execute(Vector<Node> args, SharedPtr<Scope> callScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    (void)args;
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!instanceNode) {throw MerkError("An Instance In Method::execute was not provided");}
    // DEBUG_LOG(LogLevel::PERMISSIVE, highlight("+++++++++++++++++++ EXECUTING METHOD " + name + " **********************", Colors::orange));

    // DEBUG_LOG(LogLevel::PERMISSIVE, "EXECUTING METHOD ", name);
    // DEBUG_LOG(LogLevel::PERMISSIVE, "METHOD", name, "CALL SCOPE BEFORE PARAMS ADDED");
    callScope->owner = generateScopeOwner("MethodExecutor", name);
    // callScope->debugPrint();
    // callScope->printChildScopes();

    // auto params = parameters.clone();
    // params.verifyArguments(args);
    // for (size_t i = 0; i < params.size(); ++i) {
    //     callScope->declareVariable(params[i].getName(), makeUnique<VarNode>(args[i]));
    // }

    
    
    try {

        if (!callScope){throw MerkError("Method " + name +" Has No Captured Scope:");}

        auto capturedScope = getCapturedScope();

        if (!callScope){throw MerkError("Method " + name +" Has No Call Scope:");}

        if (!instanceNode) {throw MerkError("An Instance In Method::execute was not provided just before body->evaluate");}

        // // capturedScope->appendChildScope(callScope);

        // DEBUG_LOG(LogLevel::PERMISSIVE, "Method", name, "Captured Full Scope");
        // // capturedScope->debugPrint();
        // // capturedScope->printChildScopes();
        // DEBUG_LOG(LogLevel::PERMISSIVE, "METHOD", name, "CALL SCOPE AFTER PARAMS ADDED");
        // callScope->debugPrint();
        // callScope->printChildScopes();

        Node val = body->evaluate(callScope, instanceNode);
        DEBUG_FLOW_EXIT();
        return val;
    } catch (const ReturnException& e) {
        DEBUG_FLOW_EXIT();
        return e.getValue();
    }
}

// setCapturedScope: Updates the captured scope and updates the method body's scope accordingly.
void Method::setCapturedScope(SharedPtr<Scope> scope) {
    if (!scope) {
        throw MerkError("Cannot set a null scope in Method::setCapturedScope.");
    }
    capturedScope = scope;
    capturedScope->owner = generateScopeOwner("MethodCapturedScope", name);
    body->setScope(capturedScope);
}

// getCapturedScope: Returns the currently captured scope.
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