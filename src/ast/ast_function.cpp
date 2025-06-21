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

        String branch = "CallableRef";
        
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
        branch = "CallableCall";
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

Node FunctionBody::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    Node val = Evaluator::evaluateFunction(getMutableChildren(), scope, instanceNode);
    
    DEBUG_FLOW_EXIT();
    return val;
} 

Node FunctionDef::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    auto freeVarNames = body->collectFreeVariables();
    DEBUG_LOG(LogLevel::PERMISSIVE, "Callable Type For Function", name, callableTypeAsString(callType));

    if (callType == CallableType::FUNCTION){
        FreeVars tempFreeVars = freeVarNames;
        // DEBUG_LOG(LogLevel::PERMISSIVE, "freeVarNames before param check: ", highlight(joinStrings(freeVarNames, ", "), Colors::bg_cyan));
        for (auto& freeVar : freeVarNames){
            std::cout << freeVar;
        }

        for (auto& param : parameters){
            auto it = tempFreeVars.find(param.getName()); // find a matching param name
            if (it != tempFreeVars.end()){                // indicates a match
                tempFreeVars.erase(it);
            }
        }
        
        // DEBUG_LOG(LogLevel::PERMISSIVE, "Remaining tempFreeVars after param strip: ", highlight(joinUnorderedSetStrings(tempFreeVars, ", "), Colors::red));
        for (auto& freeVar : tempFreeVars){
            std::cout << freeVar;
        }
        if (tempFreeVars.size() > 0){
            std::ostringstream oss;
            for (auto& var : tempFreeVars){
                DEBUG_LOG(LogLevel::PERMISSIVE, var);
                oss << highlight("'", Colors::yellow) << highlight(var, Colors::purple) << highlight("'", Colors::yellow) << " ";
            }
            throw MerkError("The Following Vars: " + oss.str() + "; were defined outside of function defined using function");
        }
    }   
  
    SharedPtr<Scope> defScope = scope->isolateScope(freeVarNames);
    defScope->isCallableScope = true;
    defScope->owner = generateScopeOwner("FunctionDef", name);
    
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
    
    DEBUG_LOG(LogLevel::DEBUG, "FunctionDef::evaluate -> parameters -> ", parameters.toString());

    SharedPtr<Function> func = makeShared<UserFunction>(name, std::move(clonedBody), parameters, callType);
    
    auto funcSig = func->toCallableSignature();

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
    DEBUG_LOG(LogLevel::PERMISSIVE, "DEF SCOPE");
    defScope->debugPrint();

    DEBUG_LOG(LogLevel::PERMISSIVE, "DEF SCOPE PARENT:");

    if (defScope->getParent()){
        defScope->getParent()->debugPrint();
    } else {
        DEBUG_LOG(LogLevel::PERMISSIVE, "NONE");
    }

    // Wrap it in a FunctionNode (for returning it as a value)
    FunctionNode funcNode(func);

    DEBUG_FLOW_EXIT();
    return funcNode;
}

Node FunctionCall::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::HIGH); 
    scope->owner = generateScopeOwner("FuncCall", name);
    Vector<Node> evaluatedArgs = handleArgs(scope);

    if (!scope->hasFunction(name)){
        throw MerkError("Function: " + name + " Couldn't Be Found");
    }
    DEBUG_LOG(LogLevel::ERROR, highlight("Found Function " + name, Colors::yellow));
    
    auto optSig = scope->getFunction(name, evaluatedArgs);

    if (!optSig){
        throw FunctionNotFoundError(name);
    }
    SharedPtr<Function> func = std::static_pointer_cast<Function>(optSig->getCallable());
    // SharedPtr<Function> func = std::static_pointer_cast<Function>(optSig->get().getCallable());

    // SharedPtr<Function> func = optSig->get().getCallable();
    func->getCapturedScope()->owner = generateScopeOwner("FuncCall", name);
    SharedPtr<Scope> capturedScope = func->getCapturedScope();
    // callScope->debugPrint();

    auto callScope = capturedScope->makeCallScope();
    DEBUG_LOG(LogLevel::PERMISSIVE, "CALL SCOPE");

    callScope->debugPrint();

    DEBUG_LOG(LogLevel::PERMISSIVE, "CALL SCOPE Parent");

    if (callScope->getParent()){
        callScope->getParent()->debugPrint();
    } else {
        DEBUG_LOG(LogLevel::PERMISSIVE, "NONE");
    }

    callScope->owner = "FunctionCall:evaluate (" + name + ")";

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

    
    
    func->setCapturedScope(callScope);
    
    if (!func->getBody()->getScope()){
        throw ScopeError("FunctionCall func->getBoby()->getScope  created an unusable scope");
    }
   

    Node value = func->execute(evaluatedArgs, callScope);
    scope->appendChildScope(callScope, "FunctionCall::evaluate");
    DEBUG_FLOW_EXIT();
    return value; 
}

Node FunctionRef::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {

    // DEBUG_FLOW(FlowLevel::HIGH);
    
    auto optSig = scope->getFunction(name);
    if (optSig.size() == 0) {
        throw RunTimeError("Function '" + name + "' not found.");
    }
    // SharedPtr<Function> func = static_shared_ptr_cast<Function>(optSig->get().getCallable());

    // This will be a vector
    // SharedPtr<Function> funcs = std::static_pointer_cast<Function>(optSig.front());
    SharedPtr<Function> funcs = std::static_pointer_cast<Function>(optSig.front()->getCallable());


    // DEBUG_FLOW_EXIT();
    return FunctionNode(funcs);
}

