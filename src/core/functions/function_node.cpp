#include "core/types.h"
#include "core/node.h"
#include "utilities/debugger.h"

#include "core/scope.h" 
#include "ast/ast.h"
#include "ast/exceptions.h"
#include "ast/ast_control.h"

#include "core/functions/param_node.h"
#include "ast/ast_function.h"

#include "ast/ast_callable.h"


FunctionBody::FunctionBody(SharedPtr<Scope> scope) : CallableBody(scope) {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    DEBUG_LOG(LogLevel::DEBUG, "Creating FunctionBody with scope level: ", scope->getScopeLevel());
    if (!scope){
        throw MerkError("FunctionBody Must Have A Valid Scope: ");
    }

    DEBUG_FLOW_EXIT();
}

Function::Function(String name, ParamList params, CallableType funcType)
    : Callable(name, params, CallableType::FUNCTION)
{
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    

    DEBUG_LOG(LogLevel::TRACE, "FuncType: ", callableTypeAsString(funcType));
    DEBUG_FLOW_EXIT();
}

UserFunction::UserFunction(String name, UniquePtr<FunctionBody> body, ParamList parameters, CallableType funcType)
    : Function(name, parameters, CallableType::FUNCTION), body(std::move(body)) {
        DEBUG_FLOW(FlowLevel::LOW);
        
        setSubType(funcType);
            
        setCallableType(CallableType::FUNCTION);
        DEBUG_FLOW_EXIT();
}

FunctionBody* UserFunction::getBody() const {return body.get();}

void UserFunction::setCapturedScope(SharedPtr<Scope> scope) {
    if (!scope) {
        throw MerkError("Cannot set a null scope in UserFunction.");
    }
    capturedScope = scope;
   
    body->setScope(capturedScope);
}

SharedPtr<Scope> UserFunction::getCapturedScope() const {
    return capturedScope;
}

Node UserFunction::execute(Vector<Node> args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    (void)args;
    DEBUG_FLOW();
    if (!scope){
        throw MerkError("Starting Scope Null in: UserFunction::execute");
    }

    Node value;
    try {
        value = body->evaluate(scope);
    } catch (const ReturnException& e) {
        DEBUG_LOG(LogLevel::DEBUG, highlight("Caught ReturnException In Function Execution. Returning value:", Colors::red), e.getValue());
        DEBUG_FLOW_EXIT();
        return e.getValue();  // Extract and return function's result
    }

    DEBUG_FLOW_EXIT();
    throw MerkError("Function did not return a value.");
}

FunctionBody::~FunctionBody(){DEBUG_LOG(LogLevel::TRACE, highlight("Destroying FunctionBody", Colors::orange)); getScope().reset();} 

SharedPtr<CallableSignature> UserFunction::toCallableSignature() {
    DEBUG_FLOW(FlowLevel::LOW);

    SharedPtr<CallableSignature> funcSig = makeShared<CallableSignature>(
        shared_from_this(), getCallableType()
    );

    funcSig->setSubType(getSubType());

    if (funcSig->getCallableType() == CallableType::DEF) {
        throw MerkError("Primary Callable Type is: " + callableTypeAsString(funcSig->getCallableType()));
    }

    funcSig->setParameters(parameters.clone());
 
    DEBUG_FLOW_EXIT();
    return funcSig;
}

void UserFunction::setScope(SharedPtr<Scope> newScope) const {
    newScope->owner = generateScopeOwner("UserFunction", name);
    getBody()->setScope(newScope);
}


FunctionNode::FunctionNode(SharedPtr<Function> function) : CallableNode(function, "Function") {data.type = NodeValueType::Function;}

FunctionNode::FunctionNode(SharedPtr<Callable> function) : CallableNode(function, "Function") {data.type = NodeValueType::Function;}
SharedPtr<Callable> FunctionNode::getCallable() const {return std::get<SharedPtr<Function>>(data.value);}
// FunctionNode::FunctionNode(SharedPtr<NativeFunction> native): FunctionNode(static_pointer_cast<Callable>(native)) {}