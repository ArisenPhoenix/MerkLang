#include "utilities/debugger.h"
#include "core/node/ArgumentNode.hpp"
// #include "core/types.h"
#include "core/TypesFWD.hpp"
#include "utilities/debugging_functions.h"
#include "ast/AstBase.hpp"
#include "ast/Ast.hpp"
#include "ast/Exceptions.hpp"
#include "ast/AstControl.hpp"
#include "core/callables/Callable.hpp"
#include "ast/AstChain.hpp"


FreeVars CodeBlock::collectFreeVariables() const {
    DEBUG_FLOW();
    if (isFreeVarsCacheValid()) {
        return freeVars;
    }
    freeVars.clear();
    FreeVars localDecls;

    // First pass: gather declared variables
    for (const auto& child : children) {
        if (child->getAstType() == AstType::VariableDeclaration) {
            const auto* decl = static_cast<const VariableDeclaration*>(child.get());
            localDecls.insert(decl->getName());
        }
    }

    // Second pass: recurse into children
    for (const auto& child : children) {
        FreeVars childFree = child->collectFreeVariables();

        for (const auto& var : childFree) {
            if (localDecls.find(var) == localDecls.end()) {
                freeVars.insert(var);
            }
        }
    }
    localDecls.clear();
    markFreeVarsClean();
    DEBUG_FLOW_EXIT();
    return freeVars;
}


FreeVars VariableReference::collectFreeVariables() const {
    DEBUG_FLOW();
    FreeVars vars;
    vars.insert(getName());
    return vars;
}

FreeVars VariableDeclaration::collectFreeVariables() const {
    DEBUG_FLOW();
    freeVars.clear();

    auto* expression = getRawExpression();
    if (expression){
        freeVars.merge(expression->collectFreeVariables());
    }

    DEBUG_FLOW_EXIT();
    return freeVars;
}


FreeVars VariableAssignment::collectFreeVariables() const {
    DEBUG_FLOW();
    freeVars.clear();

    auto expr = getRawExpression(); 
    if (expr) {
        freeVars.merge(expr->collectFreeVariables());
    }

    DEBUG_FLOW_EXIT();
    return freeVars;
}

FreeVars ElseStatement::collectFreeVariables() const {
    DEBUG_FLOW();
    if (isFreeVarsCacheValid()) {
        return freeVars;
    }
    freeVars.clear();
    freeVars.merge(getBody()->collectFreeVariables());

    markFreeVarsClean();
    DEBUG_FLOW_EXIT();
    return freeVars;

}

FreeVars ElifStatement::collectFreeVariables() const {
    DEBUG_FLOW();
    if (isFreeVarsCacheValid()) {
        return freeVars;
    }
    freeVars.clear();
    freeVars.merge(getBody()->collectFreeVariables());

    markFreeVarsClean();
    DEBUG_FLOW_EXIT();
    return freeVars;

}

FreeVars IfStatement::collectFreeVariables() const {
    DEBUG_FLOW();
    if (isFreeVarsCacheValid()) {
        return freeVars;
    }
    freeVars.clear();
    localAssign.clear();
  
        
    for (const auto& nestedFree : getBody()->collectFreeVariables()){
        freeVars.insert(nestedFree);
    }


    for (auto& elif : getElifs()) {
        freeVars.merge(elif->collectFreeVariables());
    }

    if (getElse()){
        freeVars.merge(getElse()->collectFreeVariables());
    }

    markFreeVarsClean();
    DEBUG_FLOW_EXIT();
    return freeVars;
}




FreeVars Throw::collectFreeVariables() const {
    DEBUG_FLOW();
    freeVars.clear();

    if (getValue()){
        freeVars.merge(getValue()->collectFreeVariables());
    }

    DEBUG_FLOW_EXIT();
    return freeVars;
}



FreeVars Return::collectFreeVariables() const {
    DEBUG_FLOW();
    freeVars.clear();

    if (getValue()){
        freeVars.merge(getValue()->collectFreeVariables());
    }

    DEBUG_FLOW_EXIT();
    return freeVars;
}


FreeVars WhileLoop::collectFreeVariables() const {
    DEBUG_FLOW();
    if (isFreeVarsCacheValid()) {
        return freeVars;
    }
    freeVars.clear();

    if (condition) {
        freeVars.merge(condition->collectFreeVariables());
    }

    if (body) {
        freeVars.merge(body->collectFreeVariables());
    }
    markFreeVarsClean();
    DEBUG_FLOW_EXIT();
    return freeVars;
}

FreeVars ForLoop::collectFreeVariables() const {
    if (isFreeVarsCacheValid()) {
        return freeVars;
    }

    freeVars.clear();
    if (startExpr) {
        freeVars.merge(startExpr->collectFreeVariables());
    }
    if (endExpr) {
        freeVars.merge(endExpr->collectFreeVariables());
    }
    if (stepExpr) {
        freeVars.merge(stepExpr->collectFreeVariables());
    }
    if (body) {
        freeVars.merge(body->collectFreeVariables());
    }

    // Loop variable is assigned by this node, so it is not a free variable.
    freeVars.erase(loopVariable);
    markFreeVarsClean();
    return freeVars;
}


FreeVars BinaryOperation::collectFreeVariables() const {
    DEBUG_FLOW();
    if (isFreeVarsCacheValid()) {
        return freeVars;
    }
    freeVars.clear();

    auto left = getLeftSide();
    if (left){
        freeVars.merge(left->collectFreeVariables());
    }
    
    if (right){
        freeVars.merge(right->collectFreeVariables());
    }

    markFreeVarsClean();
    DEBUG_FLOW_EXIT();
    return freeVars;
}



FreeVars Argument::collectFreeVariables() const {
    if (isKeyword()) {
        FreeVars vars = key->collectFreeVariables();
        return vars;
    }

    return value->collectFreeVariables();
}

FreeVars Arguments::collectFreeVariables() const {
    if (isFreeVarsCacheValid()) {
        return freeVars;
    }
    freeVars.clear();
    FreeVars freeVars;
    for (auto& arg : arguments) {
        freeVars.merge(arg.collectFreeVariables());
    }
    this->freeVars = std::move(freeVars);
    markFreeVarsClean();
    return this->freeVars;
}

FreeVars CallableDef::collectFreeVariables() const {
    DEBUG_FLOW();
    if (isFreeVarsCacheValid()) {
        return freeVars;
    }
    freeVars.clear();

    std::unordered_set<String> paramNames;
    for (const auto& param : parameters) {
        paramNames.insert(param.getName());
    }

    FreeVars nestedFree = body->collectFreeVariables();
    for (const auto& var : nestedFree) {
        if (paramNames.find(var) == paramNames.end()) {
            freeVars.insert(var);
        }
    }

    markFreeVarsClean();
    DEBUG_FLOW_EXIT();
    return freeVars;
}

FreeVars CallableCall::collectFreeVariables() const {
    DEBUG_FLOW();
    if (isFreeVarsCacheValid()) {
        return freeVars;
    }
    freeVars.clear();
    freeVars.merge(arguments->collectFreeVariables());

    markFreeVarsClean();
    DEBUG_FLOW_EXIT();
    return freeVars;
}

FreeVars Chain::collectFreeVariables() const {
    DEBUG_FLOW();
    if (isFreeVarsCacheValid()) {
        return freeVars;
    }
    freeVars.clear();

    if (!elements.empty()) {
        auto& element = getElements()[0];
        freeVars.merge(element.object->collectFreeVariables());
    }

    markFreeVarsClean();
    DEBUG_FLOW_EXIT();
    return freeVars;
}

