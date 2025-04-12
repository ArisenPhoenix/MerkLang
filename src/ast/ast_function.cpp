#include <unordered_set>

#include "core/types.h"
#include "core/node.h"
#include "core/errors.h"
#include "utilities/helper_functions.h"
#include "core/scope.h"
#include "ast/ast_base.h"
// #include "ast/ast_control.h"
#include "ast/ast.h"
#include "core/functions/function_node.h"
#include "core/evaluator.h"
#include "ast/ast_callable.h"
#include "ast/ast_function.h" 

String ParameterAssignment::toString() const {
    return "ParameterAssignment(variable=" + getName() + ")";

    
}



FunctionRef::FunctionRef(String name, SharedPtr<Scope> scope)
    : CallableRef(name, scope) {
        DEBUG_FLOW();

        String branch = "Functional";
        
        DEBUG_FLOW_EXIT();
}




FunctionBody::FunctionBody(UniquePtr<CodeBlock>&& block)
    : CallableBody(std::move(block)) {
}

FunctionDef::FunctionDef(String name, ParamList parameters, UniquePtr<FunctionBody> body, CallableType funcType, SharedPtr<Scope> scope)
    : CallableDef(name, std::move(parameters), std::move(body), funcType, scope) {
        DEBUG_FLOW();
        DEBUG_LOG(LogLevel::ERROR, "FuncType: ", callableTypeAsString(callType));
        DEBUG_FLOW_EXIT();
}


FunctionCall::FunctionCall(String functionName, Vector<UniquePtr<ASTStatement>> arguments, SharedPtr<Scope> scope)
    : CallableCall(functionName, std::move(arguments), scope) {
        // DEBUG_FLOW(FlowLevel::MED); 
        branch = "Functional";
        // DEBUG_FLOW_EXIT();
}


UniquePtr<BaseAST> FunctionDef::clone() const {
    DEBUG_FLOW(FlowLevel::LOW);

    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    auto clonedBody = static_unique_ptr_cast<FunctionBody>(std::move(clonedBodyBase));

    auto funcDef = std::make_unique<FunctionDef>(name, parameters, std::move(clonedBody), callType, getScope());

    DEBUG_FLOW_EXIT();
    return funcDef;
}

Node FunctionBody::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    // DEBUG_LOG(LogLevel::DEBUG, "debugPrint");
    Node val = Evaluator::evaluateFunction(getMutableChildren(), scope);
    
    DEBUG_FLOW_EXIT();
    return val;
}

Node FunctionDef::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    auto freeVarNames = body->collectFreeVariables();

    if (callType == CallableType::FUNCTION){
        FreeVars tempFreeVars = freeVarNames;
        for (auto& param : parameters){
            auto it = tempFreeVars.find(param.getName()); // find a matching param name
            if (it != tempFreeVars.end()){                // indicates a match
                tempFreeVars.erase(it);
            }
        }

        if (tempFreeVars.size() > 0){
            std::ostringstream oss;
            for (auto& var : tempFreeVars){
                oss << highlight("'", Colors::yellow) << highlight(var, Colors::purple) << highlight("'", Colors::yellow) << " ";
            }
            throw MerkError("The Following Vars: " + oss.str() + "; were defined outside of function defined using function");
        }
    }
    

    // DEBUG_LOG(LogLevel::ERROR, highlight("Free Var Names:", Colors::pink), std::to_string(freeVarNames.size()));
    // for (auto& var : freeVarNames) {
        // DEBUG_LOG(LogLevel::ERROR, "Variable:", highlight(var, Colors::pink));
    // }   
  
    SharedPtr<Scope> defScope = scope->detachScope(freeVarNames);
    defScope->isCallableScope = true;
    
    // Create a new UserFunction instance
    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    
    if (!body->getScope()){
        DEBUG_LOG(LogLevel::ERROR, "Body's Scope is null in FunctionDef::evaluate()");
        throw MerkError("Scope not present in FunctionDef::evaluate(scope)");
    }

    auto clonedBody = static_unique_ptr_cast<FunctionBody>(std::move(clonedBodyBase));
 
    if (!clonedBody->getScope()){
        DEBUG_LOG(LogLevel::ERROR, "Body's Scope is null in FunctionDef::evaluate()");
        throw MerkError("Scope not present in FunctionDef::evaluate(scope) of clonedBody");
    }

    DEBUG_LOG(LogLevel::DEBUG, "FunctionDef Defining Scope: ", scope->getScopeLevel());
    
    
    SharedPtr<Function> func = std::make_shared<UserFunction>(name, std::move(clonedBody), parameters, callType);
    
    auto funcSig = func->toCallableSignature();

    // if (funcSig->getCallableType() == CallableType::DEF) {
    //     throw MerkError("Primary Callable Type is: " + callableTypeAsString(funcSig->getCallableType()));
    // }
    // Register the function in the current scope's function registry:
    scope->registerFunction(name, funcSig);

    if (!defScope){
        DEBUG_FLOW_EXIT();
        throw MerkError("Defining Scope for FunctionDef::evaluate is null");
    }
    
    if (!func->getBody()){
        DEBUG_FLOW_EXIT();
        throw MerkError("Function body is null in FunctionDef::evaluate");
    }

    func->setCapturedScope(defScope);

    // Wrap it in a FunctionNode (for returning it as a value)
    FunctionNode funcNode(func);

    DEBUG_FLOW_EXIT();
    return funcNode;
}

Node FunctionCall::evaluate(SharedPtr<Scope> scope) const {
    // DEBUG_FLOW(FlowLevel::HIGH); 

    Vector<Node> evaluatedArgs = handleArgs(scope);
    
    auto optSig = scope->getFunction(name, evaluatedArgs);

    if (!optSig){
        throw FunctionNotFoundError(name);
    }

    SharedPtr<Function> func = std::static_pointer_cast<Function>(optSig->get().getCallable());

    // SharedPtr<Function> func = optSig->get().getCallable();
    SharedPtr<Scope> callScope = func->getCapturedScope()->clone()->createChildScope();

    if (!callScope){
        throw MerkError("Scope Is Not Valid In UserFunction::execute->function");
    }
    
    DEBUG_LOG(LogLevel::DEBUG, "******************************* UserFunction Scope Set *******************************");

    func->parameters.verifyArguments(evaluatedArgs);
    // For each parameter, declare it in the new scope.
    for (size_t i = 0; i < func->parameters.size(); ++i) {
        VarNode paramVar(evaluatedArgs[i]);
        callScope->declareVariable(func->parameters[i].getName(), makeUnique<VarNode>(paramVar));
    }

    scope->appendChildScope(callScope);
    
    func->setCapturedScope(callScope);
    
    if (!func->getBody()->getScope()){
        throw ScopeError("FunctionCall func->getBoby()->getScope  created an unusable scope");
    }
   

    Node value = func->execute(evaluatedArgs, callScope);
    // DEBUG_FLOW_EXIT();
    return value; 
}

Node FunctionRef::evaluate(SharedPtr<Scope> scope) const {

    // DEBUG_FLOW(FlowLevel::HIGH);
    
    auto optSig = scope->getFunction(name);
    if (!optSig) {
        throw RunTimeError("Function '" + name + "' not found.");
    }
    // SharedPtr<Function> func = static_shared_ptr_cast<Function>(optSig->get().getCallable());

    // SharedPtr<Function> func = optSig->get().getCallable();
    SharedPtr<Function> func = std::static_pointer_cast<Function>(optSig->get().getCallable());

    // DEBUG_FLOW_EXIT();
    return FunctionNode(func);
}

