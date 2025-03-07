#include <sstream>
#include <stdexcept>
#include <iostream>
#include <typeindex>
#include <optional>
#include <cassert>
#include <string>
#include <unordered_set>

#include "core/types.h"
#include "core/node.h"

#include "ast/ast.h"
#include "ast/ast_control.h"
#include "ast/exceptions.h"

#include "utilities/debugging_functions.h"
#include "utilities/helper_functions.h"
#include "utilities/streaming.h"

#include "core/errors.h"
#include "core/evaluator.h"
#include "core/scope.h"


bool validateLeftAndRightAST(const UniquePtr<ASTStatement>& left, const UniquePtr<ASTStatement>& right, const String& methodName, String op = "") {
    if (!left) {
        throw MerkError(methodName + ": Left ASTStatement operand is null.");
    }
    if (!right) {
        throw MerkError(methodName + ": Right ASTStatement operand is null.");
    }
    DEBUG_LOG(LogLevel::DEBUG, "| Left -> ", left->toString(), op, "Right ->", right->toString());
    return true;
}

BaseAST::BaseAST(SharedPtr<Scope> scope) : scope(scope) {}

void BaseAST::setScope(SharedPtr<Scope> newScope) {
    DEBUG_FLOW(FlowLevel::LOW);

    if (!newScope){
        throw MerkError("Trying to set a null scope in BaseAST.");
    }
    scope = newScope;
    DEBUG_FLOW_EXIT();
}



// Basic Constructors
LiteralValue::LiteralValue(LitNode value, SharedPtr<Scope> scope, bool isString, bool isBool)
    : ASTStatement(scope), value(value), _isString(isString), _isBool(isBool) {
        validateScope(scope, "LiteralValue::LiteralValue", value.toString());
    }

// Variable Constructors
VariableDeclaration::VariableDeclaration(String name, VarNode value, SharedPtr<Scope> scope, std::optional<std::type_index> typeTag, UniquePtr<ASTStatement> valueNode)
    : ASTStatement(scope), name(std::move(name)), variable(std::move(value)),
        typeTag(std::move(typeTag)), valueExpression(std::move(valueNode)) {
        validateScope(scope, "VariableDeclaration::VariableDeclaration", value.toString());
        DEBUG_LOG(LogLevel::DEBUG, highlight("[VarNode Constructor] Value:", Colors::purple), this->toString());



}

VariableAssignment::VariableAssignment(String name, UniquePtr<ASTStatement> valueExpression, SharedPtr<Scope> scope)
    : ASTStatement(scope), name(name), valueExpression(std::move(valueExpression)) {
        validateScope(scope, "VariableAssignment::VariableAssignment", name);
    }

VariableReference::VariableReference(const String& name, SharedPtr<Scope> scope)
    : ASTStatement(scope), name(name) {
        DEBUG_LOG(LogLevel::DEBUG, "-------------------- VariableReference: ", "Name: ", name);
        if (scope){
            DEBUG_LOG(LogLevel::DEBUG, "Scope: ", scope->getScopeLevel(), "Valid: ");
        }
        // validateScope(scope, "VariableReference::VariableReference", name);
    }

UnaryOperation::UnaryOperation(const String& op, UniquePtr<ASTStatement> operand, SharedPtr<Scope> scope)
    : ASTStatement(scope), op(op), operand(std::move(operand)) {
        validateScope(scope, "UnaryOperation::UnaryOperation", op + operand->toString());
    }



// Calculation Constructors
BinaryOperation::BinaryOperation(const String& op, UniquePtr<ASTStatement> left, UniquePtr<ASTStatement> right, SharedPtr<Scope> scope)
    : ASTStatement(scope), op(op), left(std::move(left)), right(std::move(right)) {
    validateScope(scope, "BinaryOperation::BinaryOperation", op);
}


// Variable Evaluations
Node VariableDeclaration::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);

    validateScope(scope, "VariableDeclaration");
    DEBUG_LOG(LogLevel::TRACE, highlight("[VariableDeclaration::evaluate]", Colors::orange), valueExpression->toString());

    Node val = Evaluator::evaluateVariableDeclaration(valueExpression.get(), variable, scope);

    DEBUG_FLOW_EXIT();
    return val;
}

Node VariableAssignment::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);

    validateScope(scope, "VariableAssignment::evaluate", name);

    Node val = Evaluator::evaluateVariableAssignment(name, valueExpression.get(), scope);

    DEBUG_FLOW_EXIT();
    return val;
}

Node VariableReference::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);

    validateScope(scope, "VariableReference::evluate", "Name = " + name);

    VarNode& varRef = Evaluator::evaluateVariableReference(name, scope); // ✅ Preserve reference
    Node val = Node(varRef);
    DEBUG_FLOW_EXIT();
    return val;
};


// Computation Evaluations
Node BinaryOperation::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);

    validateScope(scope, "BinaryOperation::evaluate", left->toString() + " " + op + " " + right->toString());
    validateLeftAndRightAST(left, right, "BinaryOperation", op);
    auto leftValue = left->evaluate(scope);
    auto rightValue = right->evaluate(scope);

    validateLeftAndRightNodes(leftValue, rightValue, "BinaryOperation", op);


    Node val = Evaluator::evaluateBinaryOperation(op, leftValue, rightValue, scope);
    DEBUG_FLOW_EXIT();
    return val;
};

Node UnaryOperation::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    validateScope(scope, "UnaryOperation::evaluate", op);

    Node operandValue = operand->evaluate(scope);
    Node val = Evaluator::evaluateUnaryOperation(op, operandValue);
    DEBUG_FLOW_EXIT();
    return val;
}


// Set Scope overrides for particular functionality and debugging
void VariableDeclaration::setScope(SharedPtr<Scope> newScope) {
    DEBUG_LOG(LogLevel::TRACE, highlight("Setting VariableDeclaration Scope", Colors::blue));
    scope = newScope;
}

void VariableReference::setScope(SharedPtr<Scope> newScope) {
    DEBUG_LOG(LogLevel::TRACE, highlight("Setting VariableReference Scope", Colors::blue));
    scope = newScope;
}

void VariableAssignment::setScope(SharedPtr<Scope> newScope) {
    DEBUG_LOG(LogLevel::TRACE, highlight("Setting VariableAssignment Scope", Colors::blue));
    scope = newScope;
    valueExpression->setScope(newScope);
}

void BinaryOperation::setScope(SharedPtr<Scope> newScope) {
    scope = newScope;
    left->setScope(newScope);
    right->setScope(newScope);
}


void UnaryOperation::setScope(SharedPtr<Scope> newScope) {
    scope = newScope;
    operand->setScope(newScope);
}


void Return::setScope(SharedPtr<Scope> newScope) {
    scope = newScope;
    if (returnValue){
        returnValue->setScope(newScope);
    }
    
}

