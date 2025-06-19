#include "ast/ast_base.h"
#include "ast/ast.h"
#include "ast/ast_chain.h"
#include "ast/ast_control.h"
#include "ast/ast_callable.h"
#include "ast/ast_function.h"
#include "ast/ast_class.h"


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
        // all.insert(all.end(), values.begin(), values.end());

    }
    return all; 
}

Vector<const BaseAST*> ConditionalBlock::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all;
    if (includeSelf) {
        all.push_back(this);
    }

    // Assuming ConditionalBlock wraps a condition and a body
    if (condition) {
        auto cond = condition->getAllAst(includeSelf);
        mergeVectors(all, cond);

        // all.insert(all.end(), cond.begin(), cond.end());
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

        // all.insert(all.end(), values.begin(), values.end());

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

        // all.insert(all.end(), childNodes.begin(), childNodes.end());
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

        // all.insert(all.end(), bodyNodes.begin(), bodyNodes.end());
    }

    for (const auto& elif : getElifs()) {
        auto elifs = elif->getAllAst(includeSelf);
        mergeVectors(all, elifs);

        // all.insert(all.end(), elifs.begin(), elifs.end());
    }

    if (getElse()) {
        auto elseNodes = getElse()->getBody()->getAllAst(includeSelf);
        mergeVectors(all, elseNodes);

        // all.insert(all.end(), elseNodes.begin(), elseNodes.end());
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

        // all.insert(all.end(), cond.begin(), cond.end());
    }

    if (body) {
        auto bodyNodes = body->getAllAst(includeSelf);
        mergeVectors(all, bodyNodes);

        // all.insert(all.end(), bodyNodes.begin(), bodyNodes.end());
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

        // all.insert(all.end(), cond.begin(), cond.end());
    }

    if (body) {
        auto bodyNodes = body->getAllAst(includeSelf);
        mergeVectors(all, bodyNodes);

        // all.insert(all.end(), bodyNodes.begin(), bodyNodes.end());
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

        // all.insert(all.end(), operands.begin(), operands.end());

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

        // all.insert(all.end(), elems.begin(), elems.end());
    }

    return all; 
}

Vector<const BaseAST*> CallableCall::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all = {};

    if (includeSelf){
        all.push_back(this);
    }


    if (!arguments.empty()) {
        for (auto& arg : arguments){
            auto args = arg->getAllAst(includeSelf);
            mergeVectors(all, args);

            // all.insert(all.end(), args.begin(), args.end());

        }
    }

    return all; 
}


Vector<const BaseAST*> BinaryOperation::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> all = {};
    DEBUG_LOG(LogLevel::PERMISSIVE, "BinaryOperation left type: ", left->getAstTypeAsString());
    DEBUG_LOG(LogLevel::PERMISSIVE, "BinaryOperation right type: ", right->getAstTypeAsString());

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
    DEBUG_LOG(LogLevel::PERMISSIVE, "Return AST type of returnValue: ", returnValue->getAstTypeAsString());

    if (includeSelf){
        all.push_back(this);
    }
    if (returnValue){
        auto returns = returnValue->getAllAst(includeSelf);
        mergeVectors(all, returns);

    }

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
    DEBUG_LOG(LogLevel::PERMISSIVE, "ChainOperation getAllAst: lhs type = ", lhs->getAstTypeAsString());

    if (rhs) {
        DEBUG_LOG(LogLevel::PERMISSIVE, "ChainOperation getAllAst: rhs type = ", rhs->getAstTypeAsString());

        auto rhsNodes = rhs->getAllAst(includeSelf);
        mergeVectors(all, rhsNodes);

    }

    return all;
}




// Vector<const BaseAST*> ChainOperation::getAllAst(bool includeSelf) const {
//     Vector<const BaseAST*> result;
//     if (includeSelf) result.push_back(this);
//     auto lhsNodes = lhs->getAllAst(includeSelf);
//     auto rhsNodes = rhs ? rhs->getAllAst(includeSelf) : Vector<const BaseAST*>();
//     result.insert(result.end(), lhsNodes.begin(), lhsNodes.end());
//     result.insert(result.end(), rhsNodes.begin(), rhsNodes.end());
//     return result;
// }