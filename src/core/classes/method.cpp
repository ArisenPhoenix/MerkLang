#include "core/types.h"
#include "utilities/debugger.h"
#include "core/errors.h"
#include "core/evaluator.h"
#include "ast/exceptions.h" 
#include "ast/ast_class.h"
#include "ast/ast_callable.h"
#include "ast/ast_callable.h"
#include "core/classes/method.h"

// Constructor implementation.





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
    body = std::move(method.body);
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
    // getBody()->getScope()->owner = generateScopeOwner("Method", name);
}

SharedPtr<CallableSignature> Method::toCallableSignature() {
    DEBUG_FLOW(FlowLevel::LOW);
    DEBUG_LOG(LogLevel::TRACE, "Callable Type: ", callableTypeAsString(this->callType));
    DEBUG_LOG(LogLevel::TRACE, "subType: ", callableTypeAsString(this->subType));

    SharedPtr<CallableSignature> methodSig = std::make_shared<CallableSignature>(
        shared_from_this(), getCallableType()
    );

    methodSig->setSubType(getSubType());

    if (methodSig->getCallableType() == CallableType::DEF) {
        throw MerkError("Primary Callable Type is: " + callableTypeAsString(methodSig->getCallableType()));
    }
    DEBUG_FLOW_EXIT();
    return methodSig;
}


// Execute: Create a new method activation scope from the captured scope, bind parameters, and evaluate the body.
Node Method::execute(Vector<Node> args, SharedPtr<Scope> callScope) const {
    // (void) callScope;
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    DEBUG_LOG(LogLevel::PERMISSIVE, "Executing Method with below scope");

    SharedPtr<Scope> methodScope = callScope->makeCallScope();
    // callScope->appendChildScope(methodScope);
    // setClassScope(callScope);
    methodScope->debugPrint();
    methodScope->printChildScopes();
    parameters.verifyArguments(args);
    for (size_t i = 0; i < parameters.size(); ++i) {
        methodScope->declareVariable(parameters[i].getName(), makeUnique<VarNode>(args[i]));
    }
    try {
        DEBUG_FLOW_EXIT();
        // body->setScope(methodScope);
        return body->evaluate();
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

// void Method::setClassScope(SharedPtr<Scope> newClassScope) const {
//     if (!newClassScope) {
//         throw MerkError("Cannot Set newClassScope to a null Scope");
//     }
//     classScope = newClassScope;
// } 

String Method::toString() const {return "Method";}

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