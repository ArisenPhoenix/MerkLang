#include <sstream>
#include <stdexcept>
#include <iostream>
#include <typeindex>
#include <optional>
#include <cassert>
#include <string>
#include <unordered_set>

#include "core/node/Node.hpp"
// #include "core/types.h"
#include "core/TypesFWD.hpp"

#include "ast/Ast.hpp"
#include "ast/AstControl.hpp"
#include "ast/Exceptions.hpp"

#include "utilities/debugging_functions.h"
#include "utilities/helper_functions.h"
#include "utilities/streaming.h"

#include "core/errors.h"
#include "core/Evaluator.h"
#include "core/Scope.hpp"
#include "utilities/debugger.h"


FreeVars FreeVarCollection::collectFreeVariables() const {
    return {};
}

EvalResult BaseAST::evaluateFlow(SharedPtr<Scope> scope,
                                SharedPtr<ClassInstanceNode> instanceNode) const
{
    // Default: run the old path and wrap it
    return EvalResult::Normal(evaluate(scope, instanceNode));
}

EvalResult BaseAST::evaluateFlow() const {
    return evaluateFlow(getScope(), nullptr);
}


BaseAST::~BaseAST() = default;
LiteralValue::~LiteralValue()  {
    // DEBUG_FLOW(FlowLevel::PERMISSIVE);
    // // value = Node(Null);
    // DEBUG_FLOW_EXIT();
    // auto data = value.getValueNode();
    // data.clear();
}

void ASTStatement::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    // scope = newScope;
}

SharedPtr<Scope> ASTStatement::getScope() const {
    
    if (auto s = scope.lock()) {
        return s;
    }
    return nullptr;
}


ASTStatement::ASTStatement(SharedPtr<Scope> scope) : scope(scope) {branch = "AST";}

bool validateLeftAndRightAST(const UniquePtr<ASTStatement>& left, const UniquePtr<ASTStatement>& right, const String& methodName, String op = "") {
    MARK_UNUSED_MULTI(op);
    if (!left) {throw MerkError(methodName + ": Left ASTStatement operand is null.");}
    if (!right) {throw MerkError(methodName + ": Right ASTStatement operand is null.");}
    DEBUG_LOG(LogLevel::DEBUG, "| Left -> ", left->toString(), op, "Right ->", right->toString());
    return true;
}

BaseAST::BaseAST(SharedPtr<Scope> scope) {(void)scope;}


// Basic Constructors
LiteralValue::LiteralValue(LitNode value, SharedPtr<Scope> scope)
    : ASTStatement(scope), value(value) {
        if (!value.isValid()) {
            throw MerkError("LitNode Became Invalid After LiteralValue construction");
        }
    }

// Variable Constructors
// VariableDeclaration::VariableDeclaration(String varName, DataTypeFlags valueTypes, SharedPtr<Scope> scope, std::optional<NodeValueType> typeTag, UniquePtr<ASTStatement> valueNode)
//     : ASTStatement(scope), name(std::move(varName)), variableMeta(valueTypes),
//         typeTag(std::move(typeTag)), valueExpression(std::move(valueNode)) {
//         // validateScope(scope, "VariableDeclaration::VariableDeclaration", value.toString());
//         DEBUG_LOG(LogLevel::TRACE, highlight("[VarNode Constructor] Value:", Colors::purple), this->toString());
// }

VariableDeclaration::VariableDeclaration(String varName, DataTypeFlags valueTypes, SharedPtr<Scope> scope, UniquePtr<ASTStatement> valueNode)
    : ASTStatement(scope), name(std::move(varName)), variableMeta(valueTypes), valueExpression(std::move(valueNode)) {
        // validateScope(scope, "VariableDeclaration::VariableDeclaration", value.toString());
        DEBUG_LOG(LogLevel::TRACE, highlight("[VarNode Constructor] Value:", Colors::purple), this->toString());
        if (name.empty()) {throw MerkError("Variable Declaration name is empty");}
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
    // validateScope(scope, "UnaryOperation::UnaryOperation", op );
}



// Calculation Constructors
BinaryOperation::BinaryOperation(const String& op, UniquePtr<ASTStatement> left, UniquePtr<ASTStatement> right, SharedPtr<Scope> scope)
    : ASTStatement(scope), op(op), left(std::move(left)), right(std::move(right)) {
    DEBUG_FLOW(FlowLevel::NONE);
    // if (!scope) {throw MerkError("The Scope Passed to BinaryOperation::BinaryOperation is null");}
    // validateScope(scope, "BinaryOperation::BinaryOperation", op);
    DEBUG_FLOW_EXIT();
}




Node LiteralValue::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::NONE);
    MARK_UNUSED_MULTI(scope);

    if (val) {throw MerkError("returning cached literal"); return Node(val->getInner()); }  // something of a cached value
    // validateScope(scope, "LiteralValue", toString());
    Node evaluated =  Evaluator::evaluateLiteral(value.getValueNode());

    DEBUG_FLOW_EXIT();
    return evaluated;
};

// Variable Evaluations
Node VariableDeclaration::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::HIGH);

    // validateScope(scope, "VariableDeclaration");
    DEBUG_LOG(LogLevel::TRACE, highlight("[VariableDeclaration::evaluate]", Colors::orange), valueExpression->toString());
    // variable.data.fullType = type;
    // const_cast<VarNode>(variable);
    // variable.setFullType(type);
    // variable.setFullType(type);
    String varName = name;

    Node val = Evaluator::evaluateVariableDeclaration(varName, valueExpression.get(), variableMeta, scope, instanceNode);

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
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    // validateScope(scope, "VariableReference::evluate", "Name = " + name);
    // if (name == "var") { throw MerkError("Tried to Reference Variable named 'var'"); }

    VarNode& varRef = Evaluator::evaluateVariableReference(name, scope, instanceNode); 
    varRef.getValueNode().getValue().valueless_by_exception();
    DEBUG_FLOW_EXIT();
    return varRef.getValueNode();
};

LitNode LiteralValue::getValue() {return value;}
// Computation Evaluations
Node BinaryOperation::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::NONE);

    // validateScope(scope, "BinaryOperation::evaluate", left->toString() + " " + op + " " + right->toString());
    // validateLeftAndRightAST(left, right, "BinaryOperation", op);
    // if (left->getAstType() == AstType::Literal && right->getAstType() == AstType::Literal) {
    //     auto leftValue = static_cast<LiteralValue*>(left.get());
    //     auto rightValue = static_cast<LiteralValue*>(right.get());
    //     if (op == "+") {
    //         return leftValue->getValue() + rightValue->getValue();
    //     }
    //     if (op == "*") {
    //         return leftValue->getValue() * rightValue->getValue();
    //     }
        
    // }
    auto leftValue = left->evaluate(scope, instanceNode);

    auto rightValue = right->evaluate(scope, instanceNode);

    if ((leftValue.isNull() || rightValue.isNull()) && op == "==") {
        throw MerkError("About To Attempt Operation with lhs: " + leftValue.toString() + " And rhs: " + rightValue.toString() + " AST IS: " + toString());
    }
    // if (leftValue.nodeType == "LitNode" && rightValue.nodeType == "LitNode") {
    //     if (op == "+") {return leftValue + rightValue;}
    //     if (op == "*") {return leftValue * rightValue;}
    // }
    // DEBUG_LOG(LogLevel::PERMISSIVE, "RightValue Type: ", right->getAstTypeAsString(), "RightValue Type: ", rightValue);
    // throw MerkError("See above");

    // validateLeftAndRightNodes(leftValue, rightValue, "BinaryOperation", op);

    // if (op == "==") {
    //     DEBUG_LOG(LogLevel::PERMISSIVE, "LEFT VALUE BEING EVALUATED: ", leftValue.toString(), "RIGHT VALUE BEING EVALUATED: ", rightValue.toString());
    //     throw MerkError("==");
    // }
    Node val = Evaluator::evaluateBinaryOperation(op, leftValue, rightValue, scope, instanceNode);
    DEBUG_LOG(LogLevel::DEBUG, "Output Value: ", val);
    DEBUG_FLOW_EXIT();
    return val;
};

Node UnaryOperation::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    
    // validateScope(scope, "UnaryOperation::evaluate", op);
    Node operandValue = operand->evaluate(scope, instanceNode);
    Node val = Evaluator::evaluateUnaryOperation(op, operandValue, instanceNode);
    DEBUG_FLOW_EXIT();
    return val;
}


// Set Scope overrides for particular functionality and debugging
void VariableDeclaration::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    DEBUG_LOG(LogLevel::TRACE, highlight("Setting" + getAstTypeAsString() + " Scope", Colors::blue));
    // scope = newScope;
    // valueExpression->setScope(newScope);
}

void VariableReference::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    DEBUG_LOG(LogLevel::TRACE, highlight("Setting" + getAstTypeAsString() + " Scope", Colors::blue));
    // scope = newScope;
}

void VariableAssignment::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    DEBUG_LOG(LogLevel::TRACE, highlight("Setting" + getAstTypeAsString() + " Scope", Colors::blue));
    
    // valueExpression->setScope(newScope);
}

void BinaryOperation::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    DEBUG_FLOW(FlowLevel::NONE);

    // if (!newScope) {throw MerkError("BinaryOperation::setScope -> scope is null");}
    // scope = newScope;
    
    // left->setScope(newScope);
    // right->setScope(newScope);
    DEBUG_FLOW_EXIT();
}


void UnaryOperation::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    // scope = newScope;
    // operand->setScope(newScope);
}


void Return::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    DEBUG_FLOW(FlowLevel::NONE);
    // scope = newScope;
    // if (!getScope()) {throw MerkError("Return::setScope -> getScope() returned null");}
    // if (returnValue) {returnValue->setScope(newScope);}
    DEBUG_FLOW_EXIT();
}







Node Continue::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    MARK_UNUSED_MULTI(scope, instanceNode);
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




void CallableCall::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    // scope = newScope;
    // for (auto& arg : arguments) {
    //     arg->setScope(newScope);
    // }
    // arguments->setScope(newScope);
}

void CallableDef::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    // scope = newScope;
    // body->setScope(newScope);
}