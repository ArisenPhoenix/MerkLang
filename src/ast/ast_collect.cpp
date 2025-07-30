#include "utilities/debugger.h"
#include "core/types.h"
#include "utilities/debugging_functions.h"
#include "ast/ast_base.h"
#include "ast/ast.h"
#include "ast/ast_control.h"
#include "ast/ast_function.h"
#include "ast/ast_method.h"
#include "ast/ast_callable.h"
#include "ast/ast_chain.h"

FreeVars CodeBlock::collectFreeVariables() const {
    DEBUG_FLOW();
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
    freeVars.merge(getBody()->collectFreeVariables());

    DEBUG_FLOW_EXIT();
    return freeVars;

}

FreeVars ElifStatement::collectFreeVariables() const {
    DEBUG_FLOW();
    freeVars.merge(getBody()->collectFreeVariables());

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
    FreeVars freeVars;
    for (auto& arg : arguments) {
        freeVars.merge(arg.collectFreeVariables());
    }
    return freeVars;
}

FreeVars IfStatement::collectFreeVariables() const {
    DEBUG_FLOW();
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

    DEBUG_FLOW_EXIT();
    return freeVars;
}

FreeVars CallableDef::collectFreeVariables() const {
    DEBUG_FLOW();
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

    DEBUG_FLOW_EXIT();
    return freeVars;
}

FreeVars CallableCall::collectFreeVariables() const {
    DEBUG_FLOW();
    freeVars.clear();
    freeVars.merge(arguments->collectFreeVariables());

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
    freeVars.clear();

    if (condition) {
        freeVars.merge(condition->collectFreeVariables());
    }

    if (body) {
        freeVars.merge(body->collectFreeVariables());
    }
    DEBUG_FLOW_EXIT();
    return freeVars;
}


FreeVars BinaryOperation::collectFreeVariables() const {
    DEBUG_FLOW();
    freeVars.clear();

    auto left = getLeftSide();
    if (left){
        freeVars.merge(left->collectFreeVariables());
    }
    
    if (right){
        freeVars.merge(right->collectFreeVariables());
    }

    DEBUG_FLOW_EXIT();
    return freeVars;
}


FreeVars Chain::collectFreeVariables() const {
    DEBUG_FLOW();
    freeVars.clear();

    if (!elements.empty()) {
        auto& element = getElements()[0];
        freeVars.merge(element.object->collectFreeVariables());
    }

    DEBUG_FLOW_EXIT();
    return freeVars;
}

FreeVars ChainOperation::collectFreeVariables() const {
    DEBUG_FLOW();
    freeVars.clear();
    if (getLeftSide()) {
        freeVars.merge(getLeftSide()->collectFreeVariables());
    }

    if (getRightSide()) {
        freeVars.merge(getRightSide()->collectFreeVariables());
    }

    DEBUG_FLOW_EXIT();
    return freeVars;
}


