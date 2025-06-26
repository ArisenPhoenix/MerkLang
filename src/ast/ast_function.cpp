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

#include "core/evaluator.h"
#include "ast/ast_callable.h"
#include "ast/ast_function.h"





// String ParameterAssignment::toString() const {
//     return "ParameterAssignment(variable=" + getName() + ")";
// }


FunctionBody::FunctionBody(SharedPtr<Scope> scope) : CallableBody(scope) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
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
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    DEBUG_LOG(LogLevel::DEBUG, "Creating FunctionBody with scope level: ", scope->getScopeLevel());
    if (!getScope()) {
        throw MerkError("FunctionBody::FunctionBody move constructor -> scope is null");
    }
    DEBUG_FLOW_EXIT();
}


Node FunctionBody::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    Node val = Evaluator::evaluateFunction(getMutableChildren(), scope, instanceNode);
    
    DEBUG_FLOW_EXIT();
    return val;
} 



FunctionRef::FunctionRef(String name, SharedPtr<Scope> scope)
    : CallableRef(name, scope) {
        branch = "CallableRef";
}



FunctionDef::FunctionDef(String name, ParamList parameters, UniquePtr<FunctionBody> body, CallableType funcType, SharedPtr<Scope> scope)
    : CallableDef(name, std::move(parameters), std::move(body), funcType, scope) {
        DEBUG_LOG(LogLevel::DEBUG, "FuncType: ", callableTypeAsString(callType));
}


FunctionCall::FunctionCall(String functionName, Vector<UniquePtr<ASTStatement>> arguments, SharedPtr<Scope> scope)
    : CallableCall(functionName, std::move(arguments), scope) {
        if (this->name.empty()) {
            throw MerkError("FunctionCall constructed with empty name");
        }
        branch = "CallableCall";
}


UniquePtr<BaseAST> FunctionDef::clone() const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    DEBUG_LOG(LogLevel::PERMISSIVE, "FunctionDef::clone -> CLONING FunctionDef");
    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    DEBUG_LOG(LogLevel::PERMISSIVE, "FunctionDef::clone -> Clone Successful");

    auto clonedBody = static_unique_ptr_cast<FunctionBody>(std::move(clonedBodyBase));

    auto funcDef = std::make_unique<FunctionDef>(name, parameters, std::move(clonedBody), callType, getScope());

    DEBUG_FLOW_EXIT();
    return funcDef;
}



Node FunctionDef::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
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
    if (!defScope) {
        throw MerkError("defScope for FunctionDef::evaluate is null");
    }
    defScope->isCallableScope = true;
    defScope->owner = generateScopeOwner("FunctionDef", name);
    
    // Create a new UserFunction instance
    DEBUG_LOG(LogLevel::PERMISSIVE, "FunctionDef::evaluate -> cloning function body");

    UniquePtr<BaseAST> clonedBodyBase = body->clone();

    if (!body->getScope()){
        DEBUG_LOG(LogLevel::ERROR, "Body's Scope is null in FunctionDef::evaluate()");
        throw MerkError("Scope not present in FunctionDef::evaluate(scope)");
    }
    DEBUG_LOG(LogLevel::PERMISSIVE, "FunctionDef::evaluate -> ClonedBodyBase Was Cloned Successfully");

    auto clonedBody = static_unique_ptr_cast<FunctionBody>(std::move(clonedBodyBase));
    DEBUG_LOG(LogLevel::PERMISSIVE, "New Body Was Properly Cast to FunctionBody");
 
    if (!clonedBody->getScope()){
        DEBUG_LOG(LogLevel::ERROR, "Body's Scope is null in FunctionDef::evaluate()");
        throw MerkError("Scope not present in FunctionDef::evaluate(scope) of clonedBody");
    }

    DEBUG_LOG(LogLevel::DEBUG, "FunctionDef Defining Scope: ", scope->getScopeLevel());
    
    DEBUG_LOG(LogLevel::DEBUG, "FunctionDef::evaluate -> parameters -> ", parameters.toString());

    SharedPtr<Function> func = makeShared<UserFunction>(name, std::move(clonedBody), parameters, callType);
    DEBUG_LOG(LogLevel::PERMISSIVE, "SharedPtr<Function> Was Properly Created");

    
    auto funcSig = func->toCallableSignature();

    scope->registerFunction(name, funcSig);
    DEBUG_LOG(LogLevel::PERMISSIVE, "FunctionDef::evaluate -> funcSig registerd");

    if (!defScope){
        DEBUG_FLOW_EXIT();
        throw MerkError("Defining Scope for FunctionDef::evaluate is null");
    }
    
    if (!func->getBody()){
        DEBUG_FLOW_EXIT();
        throw MerkError("Function body is null in FunctionDef::evaluate");
    }

    func->setCapturedScope(defScope);
    DEBUG_LOG(LogLevel::PERMISSIVE, "FunctionDef::evaluate -> Captured Scope was Set for function");

    if (!func->getCapturedScope()) {throw MerkError("The Captured Definition Scope for the function created was not set properly");}
    DEBUG_LOG(LogLevel::PERMISSIVE, "FunctionDef::evaluate -> func's captured scope was set successfully");

    FunctionNode funcNode(func);

    DEBUG_FLOW_EXIT();
    return funcNode;
}

Node FunctionCall::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE); 

    if (!scope) {
        throw MerkError("scope passed to FunctionCall::evaluate is null");
    }
    
    Vector<Node> evaluatedArgs = handleArgs(scope);

    if (!scope->hasFunction(name)){
        throw MerkError("Function: " + name + " Couldn't Be Found");
    }
    
    auto optSig = scope->getFunction(name, evaluatedArgs);
    
    if (!optSig){
        throw FunctionNotFoundError(name);
    }

    DEBUG_LOG(LogLevel::PERMISSIVE, "Found Function", name);

    SharedPtr<Function> func = std::static_pointer_cast<Function>(optSig->getCallable());


    if (func->getSubType() == CallableType::NATIVE) {
        func->parameters.verifyArguments(evaluatedArgs); // as opposed to placing them within the callScope

        return func->execute(evaluatedArgs, scope);
    }
    
    SharedPtr<Scope> capturedScope = func->getCapturedScope();
    if (!capturedScope) {throw MerkError("Scope Is Not Valid In UserFunction::execute->function");}
    capturedScope->owner = generateScopeOwner("FuncCall", name);
    

    auto callScope = capturedScope->makeCallScope();
    if (!callScope) {throw MerkError("Scope Is Not Valid In UserFunction::execute->function");}
    callScope->owner = "FunctionCall:evaluate (" + name + ")";

    
    
    DEBUG_LOG(LogLevel::TRACE, "******************************* UserFunction Scope Set *******************************");

    func->placeArgsInCallScope(evaluatedArgs, callScope);

    func->setCapturedScope(callScope);
    
    if (!func->getBody()->getScope()){throw ScopeError("FunctionCall func->getBoby()->getScope  created an unusable scope");}
   
    scope->appendChildScope(callScope, "FunctionCall::evaluate");
    if (!callScope) {
        throw MerkError("FunctionCall:evaluate callScope being passed to func->execute is null");
    }

    Node value = func->execute(evaluatedArgs, callScope);
    DEBUG_FLOW_EXIT();
    return value; 
}

Node FunctionRef::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {    
    auto optSig = scope->getFunction(name);
    if (optSig.size() == 0) {
        throw RunTimeError("Function '" + name + "' not found.");
    }

    SharedPtr<Function> funcs = std::static_pointer_cast<Function>(optSig.front()->getCallable());


    // DEBUG_FLOW_EXIT();
    return FunctionNode(funcs);
}

