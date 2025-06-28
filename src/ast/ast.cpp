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


FreeVars FreeVarCollection::collectFreeVariables() const {
    return {};
}

BaseAST::~BaseAST() = default;

void ASTStatement::setScope(SharedPtr<Scope> newScope) {
    scope = newScope;
}

ASTStatement::ASTStatement(SharedPtr<Scope> scope) : scope(scope) {branch = "AST";}

bool validateLeftAndRightAST(const UniquePtr<ASTStatement>& left, const UniquePtr<ASTStatement>& right, const String& methodName, String op = "") {
    (void)op;
    if (!left) {
        throw MerkError(methodName + ": Left ASTStatement operand is null.");
    }
    if (!right) {
        throw MerkError(methodName + ": Right ASTStatement operand is null.");
    }
    DEBUG_LOG(LogLevel::DEBUG, "| Left -> ", left->toString(), op, "Right ->", right->toString());
    return true;
}

BaseAST::BaseAST(SharedPtr<Scope> scope) {(void)scope;}


// Basic Constructors
LiteralValue::LiteralValue(LitNode value, SharedPtr<Scope> scope, bool isString, bool isBool)
    : ASTStatement(scope), value(value), _isString(isString), _isBool(isBool) {
        // validateScope(scope, "LiteralValue::LiteralValue", value.toString());
    }

// Variable Constructors
VariableDeclaration::VariableDeclaration(String name, VarNode value, SharedPtr<Scope> scope, std::optional<NodeValueType> typeTag, UniquePtr<ASTStatement> valueNode)
    : ASTStatement(scope), name(std::move(name)), variable(std::move(value)),
        typeTag(std::move(typeTag)), valueExpression(std::move(valueNode)) {
        // validateScope(scope, "VariableDeclaration::VariableDeclaration", value.toString());
        DEBUG_LOG(LogLevel::TRACE, highlight("[VarNode Constructor] Value:", Colors::purple), this->toString());
}

VariableDeclaration::VariableDeclaration(UniquePtr<VariableDeclaration> varDec) : ASTStatement(varDec->getScope()) {
    name = varDec->name;
    variable = varDec->variable;
    typeTag = varDec->typeTag;
    valueExpression = std::move(varDec->valueExpression);       
}

VariableReference::VariableReference(UniquePtr<VariableReference> varRef) : ASTStatement(varRef->getScope()) {
    name = varRef->name;
}


VariableAssignment::VariableAssignment(String name, UniquePtr<ASTStatement> valueExpression, SharedPtr<Scope> scope)
    : ASTStatement(scope), name(name), valueExpression(std::move(valueExpression)) {
        // validateScope(scope, "VariableAssignment::VariableAssignment", name);
    }

VariableAssignment::VariableAssignment(UniquePtr<VariableAssignment> varAssign) : ASTStatement(varAssign->getScope()) {
    name = varAssign->getName();
    valueExpression = std::move(varAssign->valueExpression);
}


VariableReference::VariableReference(const String& name, SharedPtr<Scope> scope)
    : ASTStatement(scope), name(name) {
    DEBUG_LOG(LogLevel::TRACE, "-------------------- VariableReference: ", "Name: ", name);
    if (scope){
        DEBUG_LOG(LogLevel::TRACE, "Scope: ", scope->getScopeLevel(), "Valid: ");
    }
}

UnaryOperation::UnaryOperation(const String& op, UniquePtr<ASTStatement> operand, SharedPtr<Scope> scope)
    : ASTStatement(scope), op(op), operand(std::move(operand)) {
    validateScope(scope, "UnaryOperation::UnaryOperation", op );
}



// Calculation Constructors
BinaryOperation::BinaryOperation(const String& op, UniquePtr<ASTStatement> left, UniquePtr<ASTStatement> right, SharedPtr<Scope> scope)
    : ASTStatement(scope), op(op), left(std::move(left)), right(std::move(right)) {
    DEBUG_FLOW(FlowLevel::NONE);
    if (!scope) {
        throw MerkError("The Scope Passed to BinaryOperation::BinaryOperation is null");
    }
    // validateScope(scope, "BinaryOperation::BinaryOperation", op);
    DEBUG_FLOW_EXIT();
}




Node LiteralValue::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::NONE);
    MARK_UNUSED_MULTI(scope);
    // validateScope(scope, "LiteralValue", toString());
    Node val =  Evaluator::evaluateLiteral(value, _isString, _isBool);

    DEBUG_FLOW_EXIT();
    return val;
};

// Variable Evaluations
Node VariableDeclaration::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::HIGH);

    // validateScope(scope, "VariableDeclaration");
    DEBUG_LOG(LogLevel::TRACE, highlight("[VariableDeclaration::evaluate]", Colors::orange), valueExpression->toString());
    Node val = Evaluator::evaluateVariableDeclaration(valueExpression.get(), variable, typeTag, scope, instanceNode);

    DEBUG_FLOW_EXIT();
    return val;
}

Node VariableAssignment::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::HIGH);

    // validateScope(scope, "VariableAssignment::evaluate", name);

    Node val = Evaluator::evaluateVariableAssignment(name, valueExpression.get(), scope, instanceNode);

    DEBUG_FLOW_EXIT();
    return val;
}

Node VariableReference::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::HIGH);

    // validateScope(scope, "VariableReference::evluate", "Name = " + name);

    VarNode& varRef = Evaluator::evaluateVariableReference(name, scope, instanceNode); 
    DEBUG_FLOW_EXIT();
    return varRef;
};


// Computation Evaluations
Node BinaryOperation::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::NONE);

    validateScope(scope, "BinaryOperation::evaluate", left->toString() + " " + op + " " + right->toString());
    validateLeftAndRightAST(left, right, "BinaryOperation", op);
    auto leftValue = left->evaluate(scope, instanceNode);

    auto rightValue = right->evaluate(scope, instanceNode);

    validateLeftAndRightNodes(leftValue, rightValue, "BinaryOperation", op);


    Node val = Evaluator::evaluateBinaryOperation(op, leftValue, rightValue, scope, instanceNode);
    DEBUG_LOG(LogLevel::DEBUG, "Output Value: ", val);
    DEBUG_FLOW_EXIT();
    return val;
};

Node UnaryOperation::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    validateScope(scope, "UnaryOperation::evaluate", op);
    
    Node operandValue = operand->evaluate(scope, instanceNode);
    Node val = Evaluator::evaluateUnaryOperation(op, operandValue, instanceNode);
    DEBUG_FLOW_EXIT();
    return val;
}


// Set Scope overrides for particular functionality and debugging
void VariableDeclaration::setScope(SharedPtr<Scope> newScope) {
    DEBUG_LOG(LogLevel::TRACE, highlight("Setting" + getAstTypeAsString() + " Scope", Colors::blue));
    scope = newScope;
}

void VariableReference::setScope(SharedPtr<Scope> newScope) {
    (void)newScope;
    DEBUG_LOG(LogLevel::TRACE, highlight("Setting" + getAstTypeAsString() + " Scope", Colors::blue));
    scope = newScope;
}

void VariableAssignment::setScope(SharedPtr<Scope> newScope) {
    DEBUG_LOG(LogLevel::TRACE, highlight("Setting" + getAstTypeAsString() + " Scope", Colors::blue));
    scope = newScope;
    valueExpression->setScope(newScope);
}

void BinaryOperation::setScope(SharedPtr<Scope> newScope) {
    DEBUG_FLOW(FlowLevel::NONE);

    if (!newScope) {
        throw MerkError("BinaryOperation::setScope -> scope is null");
    }
    scope = newScope;
    left->setScope(newScope);
    right->setScope(newScope);
    DEBUG_FLOW_EXIT();
}


void UnaryOperation::setScope(SharedPtr<Scope> newScope) {
    scope = newScope;
    operand->setScope(newScope);
}


void Return::setScope(SharedPtr<Scope> newScope) {
    DEBUG_FLOW(FlowLevel::NONE);

    scope = newScope;
    if (returnValue){
        returnValue->setScope(newScope);
    }
    DEBUG_FLOW_EXIT();
}

Node Return::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const  {
    DEBUG_FLOW(FlowLevel::NONE);
    if (!returnValue) {
        throw RunTimeError("Return statement must have a value.");
    }

    auto value = returnValue->evaluate(scope, instanceNode);

    DEBUG_LOG(LogLevel::DEBUG, "Value after return evaluation: ", value, "Type: ", value.getTypeAsString());

    DEBUG_FLOW_EXIT();
    throw ReturnException(value);  // Immediately exit function with value

}



Return::Return(SharedPtr<Scope> scope, UniquePtr<ASTStatement> value)
    : ASTStatement(scope), returnValue(std::move(value)) {
        validateScope(scope, "Return::Return", returnValue->toString());
    }



Continue::Continue(SharedPtr<Scope> scope) : ASTStatement(scope) {}

Node Continue::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    // (void)scope;
    MARK_UNUSED_MULTI(scope);
    DEBUG_FLOW(FlowLevel::NONE);
    throw ContinueException();
}

String Continue::toString() const { return "Continue"; }
void Continue::printAST(std::ostream& os, int indent) const {
    os << std::string(indent, ' ') << highlight("Continue", Colors::cyan) << "\n";
}

UniquePtr<BaseAST> Continue::clone() const {
    return makeUnique<Continue>(getScope());
}