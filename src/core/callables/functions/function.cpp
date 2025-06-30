#include "core/types.h"
#include "core/node.h"
#include "utilities/debugger.h"

#include "core/scope.h" 
#include "ast/ast.h"
#include "ast/exceptions.h"
#include "ast/ast_control.h"

#include "core/callables/param_node.h"
#include "ast/ast_function.h"

#include "core/callables/invocalble.h"

#include "ast/ast_callable.h"




Function::Function(String name, ParamList params, CallableType funcType, bool requiresReturn, bool isStatic)
    : Invocable(name, params, CallableType::FUNCTION, requiresReturn, isStatic)
{
    DEBUG_FLOW(FlowLevel::NONE);
    DEBUG_LOG(LogLevel::TRACE, "FuncType: ", callableTypeAsString(funcType));
    DEBUG_FLOW_EXIT();
}

UserFunction::UserFunction(String name, UniquePtr<FunctionBody> body, ParamList parameters, CallableType funcType)
    : Function(name, parameters, CallableType::FUNCTION), body(std::move(body)) {
        DEBUG_FLOW(FlowLevel::NONE);

        setSubType(funcType);
        setCallableType(CallableType::FUNCTION);
        DEBUG_FLOW_EXIT();
}



Node UserFunction::execute(Vector<Node> args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    (void)args;
    DEBUG_FLOW(FlowLevel::NONE);
    if (!scope){throw MerkError("UserFunction::execute -> Starting Scope Null in: ");}

    placeArgsInCallScope(args, scope);

    Node value;
    try {
        value = body->evaluate(scope, instanceNode);
    } catch (const ReturnException& e) {
        DEBUG_LOG(LogLevel::DEBUG, highlight("Caught ReturnException In Function Execution. Returning value:", Colors::red), e.getValue());
        DEBUG_FLOW_EXIT();
        return e.getValue();  // Extract and return function's result
    }

    DEBUG_FLOW_EXIT();
    throw MerkError("Function did not return a value.");
}

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
    DEBUG_FLOW(FlowLevel::NONE);
    if (!newScope) {throw MerkError("No newScope in UserFunction::setScope");}
    newScope->owner = generateScopeOwner("UserFunction", name);
    // getThisBody()->setScope(newScope);
    auto children = body->getAllAst();
    body->setScope(newScope);

    DEBUG_FLOW_EXIT();
    // getBody()->setScope(newScope);
}

void UserFunction::setCapturedScope(SharedPtr<Scope> newScope) {
    capturedScope = newScope;
    setScope(newScope);
}

String UserFunction::toString() const {return "<Function: " + getName() + ">";}


FunctionNode::FunctionNode(SharedPtr<Function> function) : CallableNode(function, "Function") {data.type = NodeValueType::Function;}

FunctionNode::FunctionNode(SharedPtr<Callable> function) : CallableNode(function, "Function") {data.type = NodeValueType::Function;}

SharedPtr<Callable> FunctionNode::getCallable() const {return std::get<SharedPtr<Function>>(data.value);}


CallableBody* UserFunction::getInvocableBody() {return body.get();}
CallableBody* UserFunction::getBody() const {return body.get();}
// FunctionBody* UserFunction::getThisBody() {return body.get();}
FunctionBody* UserFunction::getThisBody() const {return body.get();}
UniquePtr<CallableBody> UserFunction::getBody() {return static_unique_ptr_cast<CallableBody>(body->clone());}
