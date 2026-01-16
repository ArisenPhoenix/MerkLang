#include <unordered_set>

// #include "core/types.h"
#include "core/TypesFWD.hpp"
#include "core/node/Node.hpp"
#include "core/node/ArgumentNode.hpp"
#include "core/errors.h"
#include "utilities/helper_functions.h"
#include "core/Scope.hpp"
#include "ast/AstBase.hpp"
#include "ast/Ast.hpp"
#include "ast/Exceptions.hpp"
#include "core/callables/functions/Function.hpp"
#include "core/callables/functions/NativeFunction.hpp"
#include "ast/ast_validate.h"

#include "core/Evaluator.hpp"
#include "core/FlowEvaluator.hpp"
#include "ast/AstCallable.hpp"
#include "ast/AstFunction.hpp"





// String ParameterAssignment::toString() const {
//     return "ParameterAssignment(variable=" + getName() + ")";
// }


FunctionBody::FunctionBody(SharedPtr<Scope> scope) : CallableBody(scope) {
    DEBUG_FLOW(FlowLevel::NONE);
    if (!scope) { throw MerkError("FunctionBody::FunctionBody -> given scope is null"); }
    if (!getScope()) { throw MerkError("FunctionBody::FunctionBody -> scope is null"); }

    // setScope(scope);

    DEBUG_FLOW_EXIT();
}

FunctionBody::FunctionBody(UniquePtr<CodeBlock>&& block)
    : CallableBody(std::move(block)) {
    DEBUG_FLOW(FlowLevel::NONE);
    if (!getScope()) {throw MerkError("FunctionBody::FunctionBody move constructor -> scope is null");}
    DEBUG_FLOW_EXIT();
}


Node FunctionBody::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::NONE);
    // setScope(scope);
    Node val = Evaluator::evaluateFunction(getMutableChildren(), scope, instanceNode);
    
    DEBUG_FLOW_EXIT();
    return val;
}

EvalResult FunctionBody::evaluateFlow(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::NONE);
    // setScope(scope);
    auto val = FlowEvaluator::evaluateBlock(getMutableChildren(), scope, instanceNode);
    
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

        // DEBUG_LOG(LogLevel::DEBUG, "FuncType: ", callableTypeAsString(callType));
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
    UniquePtr<BaseAST> clonedBodyBase = body->clone();    
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

        for (auto& param : parameters){
            auto it = tempFreeVars.find(param.getName()); // find a matching param name
            if (it != tempFreeVars.end()){                // indicates a match
                tempFreeVars.erase(it);
            }
        }
        
        if (tempFreeVars.size() > 0){
            throw MerkError("The Following Vars: " + highlight(joinUnorderedSetStrings(tempFreeVars, ", "), Colors::yellow) + "; were defined outside of function defined using function");
        }
    }   
    

    SharedPtr<Scope> defScope = scope->buildFunctionDefScope(freeVarNames, name);
    // SharedPtr<Scope> defScope = scope->isolateScope(freeVarNames);
    if (!defScope) {throw MerkError("defScope for FunctionDef::evaluate is null");}
    defScope->owner = generateScopeOwner("FunctionDef", name);
    
    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    auto clonedBody = static_unique_ptr_cast<FunctionBody>(std::move(clonedBodyBase));
    clonedBody->setScope(getScope());

    DEBUG_LOG(LogLevel::DEBUG, "FunctionDef Defining Scope: ", scope->getScopeLevel());
    
    DEBUG_LOG(LogLevel::DEBUG, "FunctionDef::evaluate -> parameters -> ", parameters.toString());

    SharedPtr<Function> func = makeShared<UserFunction>(name, std::move(clonedBody), parameters, callType);
    
    auto funcSig = func->toCallableSignature();

    scope->registerFunction(name, funcSig);
    func->setCapturedScope(defScope);
    
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
    DEBUG_FLOW(FlowLevel::PERMISSIVE); 
    if (!scope) {throw MerkError("scope passed to FunctionCall::evaluate is null");}
    if (name == "showScope") {scope->debugPrint(); return Node(Null);}
    auto evaluatedArgs = handleArgs(scope, instanceNode);
    SharedPtr<CallableSignature> optSig;

    auto sigOpt = scope->getFunction(name, evaluatedArgs);
    if (sigOpt.has_value()) {
        optSig = sigOpt.value();
    } else {
        auto& var = scope->getVariable(name);
        if (var.isFunctionNode()) {
            auto funcNode = var.toFunctionNode();
            optSig = funcNode.getFunction(name, evaluatedArgs);
        }
        
        // throw MerkError("Got FuncSigOpts");
        if (DynamicNode::getTypeFromValue(var.getValueNode().getValue()) == NodeValueType::Function) {
            throw MerkError("IS A FUNCTION>>>>YAAAAAAY");
        }
    }


    SharedPtr<Function> func = std::static_pointer_cast<Function>(optSig->getCallable());

    if (func->getSubType() == CallableType::NATIVE) {
        func->parameters.verifyArguments(evaluatedArgs); // as opposed to placing them within the callScope
        if (func->getName() == "DEBUG_LOG") {
            printAST(std::cout, 0);
        }
        return func->execute(evaluatedArgs, scope, instanceNode);
    }

    
    SharedPtr<Scope> callScope;
    auto captured = func->getCapturedScope();
    if (!captured) { throw MerkError("Function has No CapturedScope 1"); }

    
    if (captured->getContext().getVariables().size() == 0) {
        callScope = captured;
        scope->appendChildScope(callScope, false);

    } else {
        callScope = scope->buildFunctionCallScope(func, func->getName());
    }
    

    // Node value;
    // try {
    //     value = func->execute(evaluatedArgs, callScope);
    //     if (func->getRequiresReturn()) { throw MerkError("Function did not return a value."); }
        
    // } catch (const ReturnException& e) {
    //     DEBUG_FLOW_EXIT();
    //     value = e.getValue();  // Extract and return function's result
    // }

    Node value = func->execute(evaluatedArgs, callScope);
    scope->removeChildScope(callScope);
    return value;

    
    // scope->removeChildScope(callScope);
    // // func->setCapturedScope(callScope);
    // DEBUG_FLOW_EXIT();
    // return value;
}

Node FunctionRef::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {    
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    auto sigOpts = scope->getFunction(name);
    if (!sigOpts.has_value()) { throw FunctionNotFoundError(name); }
    auto sigs = sigOpts.value();
    // for (auto& func: optSigs) {}
    // SharedPtr<Function> funcs = std::static_pointer_cast<Function>(optSig.front()->getCallable());


    // DEBUG_FLOW_EXIT();
    auto val = FunctionNode(name, sigs);

    DEBUG_FLOW_EXIT();
    // throw MerkError("Did Not Create FunctionNode in FunctionRef::evaluate");
    
    return val;
}

