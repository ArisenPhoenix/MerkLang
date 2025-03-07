#include "core/types.h"
#include "core/node.h"
#include "utilities/debugger.h"

#include "core/scope.h"
#include "ast/ast.h"
#include "ast/ast_control.h"

#include "core/functions/param_node.h"
#include "ast/ast_function.h"


Function::Function(String name, ParamList params, FunctionType functionType){
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    name = name;
    parameters = params;
    funcType = functionType;
    DEBUG_LOG(LogLevel::TRACE, "FuncType: ", functionTypeAsString(funcType));
    DEBUG_FLOW_EXIT();

}

FunctionSignature::FunctionSignature(SharedPtr<Function> function, FunctionType funcType)
      : function(std::move(function)), funcType(funcType) {
        DEBUG_FLOW(FlowLevel::VERY_LOW);
        
        
        // parameterTypes = function->parameters.getParameterTypes();
        DEBUG_FLOW_EXIT();
      }

FunctionSignature::~FunctionSignature(){
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    DEBUG_LOG(LogLevel::TRACE, "Destroying FunctionSignature: ", function->getName(), ", Use count: ", function.use_count());
    DEBUG_FLOW_EXIT();
}


Node FunctionSignature::call(const Vector<Node>& args, SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    auto val = function->execute(args, scope);
    DEBUG_FLOW_EXIT();
    return val;
}

void FunctionSignature::setFunctionType(FunctionType functionType) {funcType = functionType;}
bool FunctionSignature::getIsUserFunction() {return funcType != FunctionType::NATIVE;}
SharedPtr<Function> FunctionSignature::getFunction() const { return function;}

const Vector<NodeValueType>& FunctionSignature::getParameterTypes() const {
    if (parameterTypes.size() == 0){
        parameterTypes = function->parameters.getParameterTypes();
    } 
    return parameterTypes;
}

bool FunctionSignature::matches(const Vector<NodeValueType>& argTypes) const {
    DEBUG_FLOW(FlowLevel::LOW);
    debugLog(true, highlight("Entering: ", Colors::orange), highlight("FunctionSignature::matches", Colors::bold_blue));
    // First, check that the number of arguments matches.
    if (parameterTypes.size() != argTypes.size()) {
        
        DEBUG_LOG(LogLevel::INFO, "Parameter count does not match. Expected: ", parameterTypes.size(),
                  ", got: ", argTypes.size());
        DEBUG_FLOW_EXIT();
        return false;
    }

    // Now check each parameter type.
    for (size_t i = 0; i < parameterTypes.size(); ++i) {
        NodeValueType expected = parameterTypes[i];
        NodeValueType provided = argTypes[i];
        DEBUG_LOG(LogLevel::DEBUG, "Expected: ", nodeTypeToString(expected), "Provided: ", nodeTypeToString(provided));

        // Allow a parameter declared as "Any" to match any argument.
        if (expected == NodeValueType::Any) {
            continue;
        }
        // Optionally: Allow an argument of Uninitialized to match, if that is acceptable.
        if (provided == NodeValueType::Uninitialized) {
            continue;
        }

        // Otherwise, they must match exactly.
        if (expected != provided) {
            DEBUG_LOG(LogLevel::DEBUG, "Type mismatch on parameter ", i, ": expected ",
            nodeTypeToString(expected), ", got ", nodeTypeToString(provided));
            DEBUG_LOG(LogLevel::DEBUG, "Parameter Failed Expected Type Criteria, returning false");

            DEBUG_FLOW_EXIT();
            return false;
        }
    }

    DEBUG_LOG(LogLevel::DEBUG, "Parameter Did Not Fail Any Criteria, returning true");

    DEBUG_FLOW_EXIT();
    return true;
}


NativeFunction::NativeFunction(String name, std::function<Node(Vector<Node>&, SharedPtr<Scope>)> impl, ParamList parameters)
    : Function(std::move(name), std::move(parameters), FunctionType::NATIVE), nativeImpl(std::move(impl)) {}

Node NativeFunction::execute(Vector<Node> args, SharedPtr<Scope> scope) const {
    parameters.verifyArguments(args);
    return nativeImpl(args, scope);
}

UserFunction::UserFunction(String name, UniquePtr<FunctionBody> body, ParamList parameters, FunctionType funcType)
    : Function(std::move(name), std::move(parameters), funcType), body(std::move(body)) {
        DEBUG_FLOW(FlowLevel::LOW);
        DEBUG_FLOW_EXIT();
    }



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

Node UserFunction::execute(Vector<Node> args, SharedPtr<Scope> scope) const {
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
    return value;
}

SharedPtr<FunctionSignature> UserFunction::toFunctionSignature() const {
    DEBUG_FLOW(FlowLevel::LOW);

    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    auto clonedBody = static_unique_ptr_cast<FunctionBody>(std::move(clonedBodyBase));

    ParamList clonedParams;
    for (const auto& param : parameters) {
        clonedParams.addParameter(param.copy());  // Create a copy
    }

    SharedPtr<Function> func = std::make_shared<UserFunction>(getName(), std::move(clonedBody), clonedParams, funcType);
    DEBUG_LOG(LogLevel::ERROR, "FuncType: ", functionTypeAsString(funcType));
    DEBUG_FLOW_EXIT();
    SharedPtr<FunctionSignature> funcSig = std::make_shared<FunctionSignature>(func, funcType);
    return funcSig;
}

















