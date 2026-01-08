#include "ast/AstBase.hpp"
#include "ast/Ast.hpp"
#include "ast/Exceptions.hpp"

#include "ast/AstChain.hpp"
#include "ast/AstControl.hpp"
#include "ast/AstCallable.hpp"
#include "core/node/ArgumentNode.hpp"
#include "core/callables/Callable.hpp"



template <typename T>
void mergeVectors(Vector<T>& destination, const Vector<T>& source) {
    destination.insert(destination.end(), source.begin(), source.end());
}



Vector<const BaseAST*> BaseAST::getAllAst(bool includeSelf) const {
    if (includeSelf){
        return {const_cast<BaseAST*>(this)};
    }
    return {};
}

Vector<const BaseAST*> ASTStatement::getAllAst(bool includeSelf) const {
    if (includeSelf){
        return {const_cast<ASTStatement*>(this)};
    }
    return {}; 
}

Vector<const BaseAST*> VariableAssignment::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all;

    if (includeSelf){
        all.push_back(this);
    }

    if (valueExpression) {
        auto values = valueExpression->getAllAst(includeSelf);
        mergeVectors(all, values);
    }
    return all; 
}

Vector<const BaseAST*> ConditionalBlock::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all;
    if (includeSelf) {
        all.push_back(this);
    }

    if (condition) {
        auto cond = condition->getAllAst(includeSelf);
        mergeVectors(all, cond);

    }

    return all;
}


Vector<const BaseAST*> VariableDeclaration::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all;

    if (includeSelf){
        all.push_back(this);
    }

    if (valueExpression) {
        auto values = valueExpression->getAllAst(includeSelf);
        mergeVectors(all, values);
    }
    return all; 
}

Vector<const BaseAST*> CodeBlock::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all;
    if (includeSelf){
        all.push_back(this);
    }

    for (const auto& child : children) {
        auto childNodes = child->getAllAst(includeSelf);
        mergeVectors(all, childNodes);
    }
    return all;

}


Vector<const BaseAST*> IfStatement::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all;
    if (includeSelf) {
        all.push_back(this);
    }

    if (condition) {
        auto cond = condition->getAllAst(includeSelf);
        all.insert(all.end(), cond.begin(), cond.end());
    }

    if (body) {
        auto bodyNodes = body->getAllAst(includeSelf);
        mergeVectors(all, bodyNodes);
    }

    for (const auto& elif : getElifs()) {
        auto elifs = elif->getAllAst(includeSelf);
        mergeVectors(all, elifs);
    }

    if (getElse()) {
        auto elseNodes = getElse()->getBody()->getAllAst(includeSelf);
        mergeVectors(all, elseNodes);
    }

    return all;
}


Vector<const BaseAST*> ElifStatement::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all;

    if (includeSelf) {
        all.push_back(this);
    }

    if (condition) {
        auto cond = condition->getAllAst(includeSelf);
        mergeVectors(all, cond);
    }

    if (body) {
        auto bodyNodes = body->getAllAst(includeSelf);
        mergeVectors(all, bodyNodes);
    }
    return all; 
}


Vector<const BaseAST*> ElseStatement::getAllAst(bool includeSelf) const {
    return getBody()->getAllAst(includeSelf);
}



Vector<const BaseAST*> WhileLoop::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all = {};
    if (includeSelf) {
        all.push_back(this);
    }

    if (condition) {
        auto cond = condition->getAllAst(includeSelf);
        mergeVectors(all, cond);
    }

    if (body) {
        auto bodyNodes = body->getAllAst(includeSelf);
        mergeVectors(all, bodyNodes);
    }

    return all;
}

Vector<const BaseAST*> UnaryOperation::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all = {};

    if (includeSelf){
        all.push_back(this);
    }

    if (operand) {
        auto operands = operand->getAllAst(includeSelf);
        mergeVectors(all, operands);
    }
    return all; 
}


Vector<const BaseAST*> BinaryOperation::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all = {};

    if (includeSelf){
        all.push_back(this);
    }

    if (left){
        auto lefts = left->getAllAst(includeSelf);
        mergeVectors(all, lefts);
    }

    if (right){
        auto rights = right->getAllAst(includeSelf);
        mergeVectors(all, rights);
    }

    return all;
}


Vector<const BaseAST*> Return::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all = {};
    if (includeSelf){
        all.push_back(this);
    }
    if (returnValue){
        auto returns = returnValue->getAllAst(includeSelf);
        mergeVectors(all, returns);

    }

    return all; 
}


Vector<const BaseAST*> Throw::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all = {};
    if (includeSelf){
        all.push_back(this);
    }
    if (expr){
        auto returns = expr->getAllAst(includeSelf);
        mergeVectors(all, returns);

    }

    return all; 
}




Vector<const BaseAST*> CallableDef::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all = {};

    if (includeSelf){
        all.push_back(this);
    }

    if (body) {
        auto elems = body->getAllAst(includeSelf);
        mergeVectors(all, elems);
    }

    return all; 
}

Vector<const BaseAST*> CallableCall::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all = {};

    if (includeSelf){
        all.push_back(this);
    }


    // if (!arguments.empty()) {
    //     for (auto& arg : arguments){
    //         auto args = arg->getAllAst(includeSelf);
    //         mergeVectors(all, args);
    //     }
    // }
    mergeVectors(all, arguments->getAllAst(includeSelf));

    return all; 
}


Vector<const BaseAST*> Chain::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all;

    if (includeSelf)
        all.push_back(this);

    for (const auto& element : elements) {
        if (element.object) {
            auto sub = element.object->getAllAst(includeSelf);  // or false if you're wrapping a primitive
            mergeVectors(all, sub);
        }
    }

    return all;
}



Vector<const BaseAST*> ChainOperation::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all;
    
    if (includeSelf)
        all.push_back(this);

    auto lhsNodes = lhs->getAllAst(includeSelf);
    mergeVectors(all, lhsNodes);

    if (rhs) {
        auto rhsNodes = rhs->getAllAst(includeSelf);
        mergeVectors(all, rhsNodes);

    }

    return all;
}


Vector<const BaseAST*> Arguments::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all;

    if (includeSelf) {all.push_back(this);}

    for (auto& arg: arguments) {
        mergeVectors(all, arg.getAllAst(includeSelf));
    }
    return all;
};

Vector<const BaseAST*> Argument::getAllAst(bool includeSelf) const {

    return value->getAllAst(includeSelf);
}