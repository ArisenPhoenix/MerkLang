#include "core/types.h"
#include "utilities/debugger.h"
#include "core/errors.h"
#include "ast/exceptions.h"
#include "ast/ast_class.h"
#include "ast/ast_callable.h"
#include "ast/ast_callable.h"
#include "core/classes/method.h"

// Constructor implementation.
Method::Method(String name, ParamList params, UniquePtr<MethodBody> body, SharedPtr<Scope> scope, bool requiresReturn)
    : Callable(std::move(name), std::move(params), CallableType::METHOD, requiresReturn), body(std::move(body->toMethodBody())), capturedScope(scope)
{
    DEBUG_LOG(LogLevel::DEBUG, "Method created: ", getName());
    this->subType = callType;

}

Method::Method(Function&& function)
    : Callable(function.getName(), std::move(function.parameters), CallableType::METHOD, false) {
        // MethodBody newBody = MethodBody(function.getCapturedScope());
        body = std::make_unique<MethodBody>(function.getCapturedScope());

        for (auto& child : function.getBody()->getChildren()){
            body->addChild(child->clone());
        }

        setCapturedScope(function.getCapturedScope());

}

Method::Method(Method& method) : Callable(method) {
    body = std::move(method.body);
    capturedScope = method.capturedScope;
}



// Method::Method(String name, ParamList params, UniquePtr<MethodBody> body, SharedPtr<Scope> scope, String accessor, bool requiresReturn)
//     : Callable(name, params, std::move(body), scope, requiresReturn), accessor(accessor) {
//     this->callType = CallableType::METHOD;
// }

// Execute: Create a new method activation scope from the captured scope, bind parameters, and evaluate the body.
Node Method::execute(Vector<Node> args, SharedPtr<Scope> callScope) const {
    (void) callScope;
    // Create a new child scope from the captured scope.
    SharedPtr<Scope> methodScope = capturedScope->createChildScope();
    // Verify that the arguments match the method's parameter list.
    parameters.verifyArguments(args);
    // Bind each parameter into the method activation scope.
    for (size_t i = 0; i < parameters.size(); ++i) {
        methodScope->declareVariable(parameters[i].getName(), makeUnique<VarNode>(args[i]));
    }
    // Evaluate the method body in the new activation scope.
    try {
        return body->clone()->evaluate(methodScope);
    } catch (const ReturnException& e) {
        return e.getValue();
    }
}

// setCapturedScope: Updates the captured scope and updates the method body's scope accordingly.
void Method::setCapturedScope(SharedPtr<Scope> scope) {
    if (!scope) {
        throw MerkError("Cannot set a null scope in Method::setCapturedScope.");
    }
    capturedScope = scope;
    body->setScope(capturedScope);
}

// getCapturedScope: Returns the currently captured scope.
SharedPtr<Scope> Method::getCapturedScope() const {
    return capturedScope;
}


String Method::getAccessor() {return accessor;}
void Method::setAccessor(String access) {accessor = access;}

String Method::toString() const {return "Method";}

// toFunctionSignature: Returns a FunctionSignature wrapping this callable (using shared_from_this()).
// SharedPtr<CallableSignature> Method::toCallableSignature() const {

//     auto clonedBodyBase = body->clone();
//     auto clonedMethodBody = static_unique_ptr_cast<MethodBody>(std::move(clonedBodyBase));

//     ParamList clonedParams;
//     for (const auto& param : parameters) {
//         clonedParams.addParameter(param.copy());  // Create a copy
//     }
//     // Create a new runtime Method with the cloned body.
//     SharedPtr<Method> method = std::make_shared<Method>(getName(), clonedParams, std::move(clonedMethodBody), getCapturedScope(), requiresReturn);
    
//     // Wrap the Method in a FunctionSignature and return it.
//     SharedPtr<CallableSignature> methodSig = std::make_shared<CallableSignature>(method, callType);
//     return methodSig;
// }
// Method::Method(String name, ParamList params, UniquePtr<MethodBody> body, SharedPtr<Scope> scope, bool requiresReturn)


SharedPtr<CallableSignature> Method::toCallableSignature() {
    DEBUG_FLOW(FlowLevel::LOW);

    auto clonedBodyBase = body->clone();
    auto clonedMethodBody = static_unique_ptr_cast<MethodBody>(std::move(clonedBodyBase));

    ParamList clonedParams;
    for (const auto& param : parameters) {
        clonedParams.addParameter(param.copy());
    }

    SharedPtr<Method> method = std::make_shared<Method>(
        getName(),
        clonedParams,
        std::move(clonedMethodBody),
        getCapturedScope(),
        requiresReturn
    );

    method->setAccessor(accessor);  // optional, if you store this elsewhere

    SharedPtr<CallableSignature> methodSig = std::make_shared<CallableSignature>(method, callType);
    methodSig->setSubType(subType);  // Also set the subtype, if relevant

    DEBUG_FLOW_EXIT();
    return methodSig;
}

