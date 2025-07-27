#include <unordered_set>

#include "core/types.h"
#include "core/node.h"
#include "core/callables/argument_node.h"
#include "core/errors.h"
#include "utilities/helper_functions.h"
#include "core/scope.h"
#include "ast/ast_base.h"
#include "ast/ast.h"
#include "core/callables/functions/function.h"
#include "core/callables/functions/native_function.h"
#include "ast/ast_validate.h"

#include "core/evaluator.h"
#include "ast/ast_callable.h"
#include "ast/ast_function.h"
#include "core/callables/argument_node.h"





// String ParameterAssignment::toString() const {
//     return "ParameterAssignment(variable=" + getName() + ")";
// }


FunctionBody::FunctionBody(SharedPtr<Scope> scope) : CallableBody(scope) {
    DEBUG_FLOW(FlowLevel::NONE);
    if (!scope) {
        throw MerkError("FunctionBody::FunctionBody -> given scope is null");
    }
    DEBUG_LOG(LogLevel::DEBUG, "Creating FunctionBody with scope level: ", scope->getScopeLevel());
    if (!getScope()) {
        throw MerkError("FunctionBody::FunctionBody -> scope is null");
    }

    // setScope(scope);

    DEBUG_FLOW_EXIT();
}

FunctionBody::FunctionBody(UniquePtr<CodeBlock>&& block)
    : CallableBody(std::move(block)) {
    DEBUG_FLOW(FlowLevel::NONE);

    DEBUG_LOG(LogLevel::DEBUG, "Creating FunctionBody with scope level: ", scope->getScopeLevel());
    if (!getScope()) {throw MerkError("FunctionBody::FunctionBody move constructor -> scope is null");}
    DEBUG_FLOW_EXIT();
}


Node FunctionBody::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::NONE);
    Node val = Evaluator::evaluateFunction(getMutableChildren(), scope, instanceNode);
    
    DEBUG_FLOW_EXIT();
    return val;
} 



FunctionRef::FunctionRef(String name, SharedPtr<Scope> scope)
    : CallableRef(name, scope) {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);

        branch = "CallableRef";
    DEBUG_FLOW_EXIT();
}



FunctionDef::FunctionDef(String name, ParamList parameters, UniquePtr<FunctionBody> body, CallableType funcType, SharedPtr<Scope> scope)
    : CallableDef(name, std::move(parameters), std::move(body), funcType, scope) {
        DEBUG_FLOW(FlowLevel::NONE);

        DEBUG_LOG(LogLevel::DEBUG, "FuncType: ", callableTypeAsString(callType));
        DEBUG_FLOW_EXIT();
}


FunctionCall::FunctionCall(String functionName, UniquePtr<ArgumentType> arguments, SharedPtr<Scope> scope)
    : CallableCall(functionName, std::move(arguments), scope) {
        DEBUG_FLOW(FlowLevel::NONE);

        if (this->name.empty()) {throw MerkError("FunctionCall constructed with empty name");}
        branch = "CallableCall";
        DEBUG_FLOW_EXIT();
}


UniquePtr<BaseAST> FunctionDef::clone() const {
    DEBUG_FLOW(FlowLevel::HIGH);
    DEBUG_LOG(LogLevel::PERMISSIVE, "FunctionDef::clone -> CLONING FunctionDef");
    validateScope(getScope(), "FunctionDef::clone");
    validateScope(body->getScope(), "FunctionDef::clone", body->toString());
    // throw MerkError("FunctionDef is being cloned");
    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    DEBUG_LOG(LogLevel::PERMISSIVE, "FunctionDef::clone -> Clone Successful");
    
    auto clonedBody = static_unique_ptr_cast<FunctionBody>(std::move(clonedBodyBase));

    auto funcDef = std::make_unique<FunctionDef>(name, parameters, std::move(clonedBody), callType, getScope());
    
    DEBUG_FLOW_EXIT();
    return funcDef;
}



Node FunctionDef::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    auto freeVarNames = body->collectFreeVariables();
    
    if (callType == CallableType::FUNCTION){
        FreeVars tempFreeVars = freeVarNames;

        for (auto& freeVar : freeVarNames){
            std::cout << freeVar;
        }

        for (auto& param : parameters){
            auto it = tempFreeVars.find(param.getName()); // find a matching param name
            if (it != tempFreeVars.end()){                // indicates a match
                tempFreeVars.erase(it);
            }
        }
        
        for (auto& freeVar : tempFreeVars){
            std::cout << freeVar;
        }
        if (tempFreeVars.size() > 0){
            throw MerkError("The Following Vars: " + highlight(joinUnorderedSetStrings(tempFreeVars, ", "), Colors::yellow) + "; were defined outside of function defined using function");
        }
    }   
  
    SharedPtr<Scope> defScope = scope->isolateScope(freeVarNames);
    if (!defScope) {throw MerkError("defScope for FunctionDef::evaluate is null");}
    defScope->owner = generateScopeOwner("FunctionDef", name);

    if (!body->getScope()){throw MerkError("Scope not present in FunctionDef::evaluate(scope)");}
    DEBUG_LOG(LogLevel::PERMISSIVE, "Cloning FunctionBody =====================================================================");
    body->printAST(std::cout);

    
    UniquePtr<BaseAST> clonedBodyBase = body->clone();

    auto clonedBody = static_unique_ptr_cast<FunctionBody>(std::move(clonedBodyBase));
    // clonedBody->setScope(getScope());
    if (!clonedBody->getScope()){throw MerkError("Scope not present in FunctionDef::evaluate(scope) of clonedBody");}

    DEBUG_LOG(LogLevel::DEBUG, "FunctionDef Defining Scope: ", scope->getScopeLevel());
    
    DEBUG_LOG(LogLevel::DEBUG, "FunctionDef::evaluate -> parameters -> ", parameters.toString());

    SharedPtr<Function> func = makeShared<UserFunction>(name, std::move(clonedBody), parameters, callType);
    
    auto funcSig = func->toCallableSignature();

    scope->registerFunction(name, funcSig);
    func->setCapturedScope(defScope);

    if (!defScope){
        DEBUG_FLOW_EXIT();
        throw MerkError("Defining Scope for FunctionDef::evaluate is null");
    }
    
    if (!func->getBody()){
        DEBUG_FLOW_EXIT();
        throw MerkError("Function body is null in FunctionDef::evaluate");
    }

    if (!func->getCapturedScope()) {throw MerkError("The Captured Definition Scope for the function created was not set properly");}

    FunctionNode funcNode(func);

    DEBUG_FLOW_EXIT();
    return funcNode;
}

Node FunctionCall::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::HIGH); 
    // throw MerkError("Hit it");
    if (!scope) {throw MerkError("scope passed to FunctionCall::evaluate is null");}
    
    auto evaluatedArgs = handleArgs(scope, instanceNode);

    if (!scope->hasFunction(name)){throw MerkError("Function: " + name + " Couldn't Be Found");}
    
    auto optSig = scope->getFunction(name, evaluatedArgs);
    
    if (!optSig){throw FunctionNotFoundError(name);}

    SharedPtr<Function> func = std::static_pointer_cast<Function>(optSig->getCallable());


    if (func->getSubType() == CallableType::NATIVE) {
        func->parameters.clone().verifyArguments(evaluatedArgs); // as opposed to placing them within the callScope
        return func->execute(evaluatedArgs, scope, instanceNode);
    }

    SharedPtr<Scope> callScope = scope->buildFunctionCallScope(func, func->getName());

    if (!callScope) {throw MerkError("Scope Is Not Valid In UserFunction::execute->function");}
    
    DEBUG_LOG(LogLevel::TRACE, "******************************* UserFunction Scope Set *******************************");
    
    if (!func->getBody()->getScope()){throw ScopeError("FunctionCall func->getBoby()->getScope  created an unusable scope");}
   
    if (!callScope) { throw MerkError("FunctionCall:evaluate callScope being passed to func->execute is null"); }
    
    Node value = func->execute(evaluatedArgs, callScope);


    // cleanup
    scope->removeChildScope(callScope);

    DEBUG_FLOW_EXIT();
    return value; 
}

Node FunctionRef::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {    
    auto optSig = scope->getFunction(name);
    if (optSig.size() == 0) { throw RunTimeError("Function '" + name + "' not found.");}

    SharedPtr<Function> funcs = std::static_pointer_cast<Function>(optSig.front()->getCallable());


    // DEBUG_FLOW_EXIT();
    return FunctionNode(funcs);
}

