#include <iostream>
#include <stdexcept>
#include <variant>
#include <cassert>
#include <unordered_map>
#include <unordered_set>

#include "core/node/Node.hpp"
#include "core/node/ArgumentNode.hpp"
#include "core/node/NodeStructures.hpp"

#include "core/types.h"
#include "core/errors.h"


#include "core/Scope.hpp"
#include "core/evaluators/Evaluator.hpp"
#include "core/Scope.hpp"

#include "utilities/helper_functions.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"

#include "ast/AstBase.hpp"
#include "ast/Ast.hpp"

#include "ast/Exceptions.hpp"
#include "ast/AstControl.hpp"
#include "ast/AstChain.hpp"
#include "ast/ast_validate.h"
#include "ast/AstMethod.hpp"
#include "ast/AstClass.hpp"
#include "ast/helpers.h"
#include "core/callables/functions/Function.hpp"
#include "core/callables/classes/Method.hpp"
#include "core/evaluators/TypeEvaluator.hpp"

// enum class Operator {
//     Equals,
//     NotEquals,
//     LessThan,
//     GreaterThan,
//     LessThanOrEquals,
//     GreaterThanOrEquals,
//     Plus,
//     Minus,
//     Multiply,
//     Divide,
//     Modulo,
//     Invalid
// };

// Operator stringToOperator(const String& op) {
//     static const std::unordered_map<String, Operator> operatorMap = {
//         {"==", Operator::Equals},
//         {"!=", Operator::NotEquals},
//         {"<", Operator::LessThan},
//         {">", Operator::GreaterThan},
//         {"<=", Operator::LessThanOrEquals},
//         {">=", Operator::GreaterThanOrEquals},
//         {"+", Operator::Plus},
//         {"-", Operator::Minus},
//         {"*", Operator::Multiply},
//         {"/", Operator::Divide},
//         {"%", Operator::Modulo}
//     };

//     auto it = operatorMap.find(op);
//     return (it != operatorMap.end()) ? it->second : Operator::Invalid;
// }

bool isDebug = Debugger::getInstance().getLogLevel() == LogLevel::DEBUG;

void evaluatingFor(const String& value, const String& methodName, int scopeLevel = -2) {
    MARK_UNUSED_MULTI(value, methodName);
    
    if (isDebug){
        String level;
        if (scopeLevel > -2){
            level = std::to_string(scopeLevel);
        } else {
            level = "[NOT USED]";
        }
        DEBUG_LOG(LogLevel::INFO, "DEBUG Evaluator: Evaluating ", methodName, " for ", value, ", scopeLevel: ", level);
    }
}

void evaluatingFor(const Node& value, const String& methodName, int scopeLevel = -2) {
    MARK_UNUSED_MULTI(value, methodName, scopeLevel);
    if (isDebug){
        String level;
        if (scopeLevel > -2){
            level = std::to_string(scopeLevel);
        } else {
            level = "[NOT USED]";
        }
        DEBUG_LOG(LogLevel::INFO, "DEBUG Evaluator: Evaluating ", methodName, " for ", value, ", scopeLevel: ", level);
    }
}

namespace Evaluator {

namespace {
constexpr bool kEnableFastIntExprAssign = false;

bool tryApplyIntBinary(const String& op, int lhs, int rhs, int& out) {
    if (op == "+") { out = lhs + rhs; return true; }
    if (op == "-") { out = lhs - rhs; return true; }
    if (op == "*") { out = lhs * rhs; return true; }
    if (op == "/") {
        if (rhs == 0) throw MerkError("Division by zero");
        out = lhs / rhs;
        return true;
    }
    if (op == "%") {
        if (rhs == 0) throw MerkError("Modulo by zero");
        out = lhs % rhs;
        return true;
    }
    return false;
}

bool tryEvalIntExprFast(const ASTStatement* expr,
                        SharedPtr<Scope> scope,
                        SharedPtr<ClassInstanceNode> instanceNode,
                        int& out) {
    if (!expr || !scope) return false;

    switch (expr->getAstType()) {
        case AstType::VariableReference: {
            const auto* ref = static_cast<const VariableReference*>(expr);
            const Node& n = scope->getVariable(ref->getName()).getValueNode();
            if (!n.isInt()) return false;
            out = n.toInt();
            return true;
        }
        case AstType::Literal: {
            Node n = expr->evaluate(scope, instanceNode);
            if (!n.isInt()) return false;
            out = n.toInt();
            return true;
        }
        case AstType::UnaryOperation: {
            const auto* u = static_cast<const UnaryOperation*>(expr);
            int v = 0;
            if (!tryEvalIntExprFast(u->getOperand(), scope, instanceNode, v)) return false;
            if (u->getOperator() == "-") {
                out = -v;
                return true;
            }
            return false;
        }
        case AstType::BinaryOperation: {
            const auto* b = static_cast<const BinaryOperation*>(expr);
            int lhs = 0;
            int rhs = 0;
            if (!tryEvalIntExprFast(b->getLeftSide(), scope, instanceNode, lhs)) return false;
            if (!tryEvalIntExprFast(b->getRightSide(), scope, instanceNode, rhs)) return false;
            return tryApplyIntBinary(b->getOperator(), lhs, rhs, out);
        }
        default:
            return false;
    }
}
} // namespace

    Node evaluateLiteral(Node value){
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        if (!value.isValid()) {throw MerkError("Literal Value is Not Valid");}
        if (value.isInstance()) {
            throw MerkError("evaluateLiteral result isInstance");
        }
        return value;
    }

    Node evaluateVariableDeclaration(String& name, const ASTStatement* valueNode, DataTypeFlags varMeta, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode ){
    MARK_UNUSED_MULTI(instanceNode);
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    SharedPtr<Scope> instanceScope = instanceNode ? instanceNode->getInstanceScope() : scope;
    bool usingInstanceScope = instanceScope != scope;

    String varName = name;

    Node rhs = valueNode->evaluate(scope, instanceNode);

    auto& tr = scope->localTypes;

    const auto anyId = tr.any();

    const auto base = varMeta.fullType.getBaseType();
    if (!base.empty() && base != "Any") {
        varMeta.declaredSig = tr.bindResolvedType(varMeta.fullType, *scope);
    } else {
        varMeta.declaredSig = anyId;
    }

    rhs.getFlags().inferredSig = tr.inferFromValue(rhs);

    if (varMeta.declaredSig != anyId) {
        auto m = tr.matchValue(varMeta.declaredSig, rhs, {/*opt*/});
        if (!m.ok) {
            throw TypeMismatchError(
                tr.toString(varMeta.declaredSig),
                rhs.getTypeAsString(),
                "evaluateVariableDeclaration"
            );
        }
    }
    
    auto finalVar = makeUnique<VarNode>(rhs, varMeta);
    auto returnVal = finalVar->getValueNode();
    auto targetScope = (usingInstanceScope ? instanceScope : scope);

    targetScope->declareVariable(varName, std::move(finalVar));

    DEBUG_FLOW_EXIT();
    return returnVal;
}


    Node evaluateVariableAssignment(String name, ASTStatement* value, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode){
        DEBUG_FLOW(FlowLevel::PERMISSIVE);

        int fastInt = 0;
        if (kEnableFastIntExprAssign && tryEvalIntExprFast(value, scope, instanceNode, fastInt)) {
            Node finalVal(fastInt);
            if (instanceNode) {
                instanceNode->getInstanceScope()->updateVariable(name, finalVal);
            } else {
                scope->updateVariable(name, finalVal);
            }
            DEBUG_FLOW_EXIT();
            return finalVal;
        }

        Node finalVal = value->evaluate(scope, instanceNode);
        if (!finalVal.isValid()) {throw MerkError("finalVal is invalid in evaluateVariableAssignment");}
        DEBUG_LOG(LogLevel::TRACE, "========================");
        DEBUG_LOG(LogLevel::TRACE, "Assigning: ", finalVal.toString() + " META: " + finalVal.getFlags().toString());
        DEBUG_LOG(LogLevel::TRACE, "========================");        
        if (instanceNode) {
            instanceNode->getInstanceScope()->updateVariable(name, finalVal);
        } else {
            scope->updateVariable(name, finalVal);
        }
        
        DEBUG_FLOW_EXIT();
        return finalVal;
    }


    VarNode& evaluateVariableReference(String name, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode){
        MARK_UNUSED_MULTI(instanceNode);
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        auto& variable = scope->getVariable(name);
        DEBUG_FLOW_EXIT();
        return variable;
    }

    Node evaluateFunction(Vector<UniquePtr<BaseAST>>& children, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode){
        MARK_UNUSED_MULTI(instanceNode);
        DEBUG_FLOW(FlowLevel::NONE);

        auto instanceScope = instanceNode ? instanceNode->getInstanceScope() : scope;

        Node lastValue;
            for (const auto& child : children) {

                DEBUG_LOG(LogLevel::TRACE, 
                    highlight("Evaluating FunctionBody child:", Colors::orange), 
                    highlight(child->getAstTypeAsString(), Colors::green), 
                    "in CodeBlock scope:", 
                    highlight(std::to_string(scope->getScopeLevel()), Colors::blue)
                );
                                
                lastValue = child->evaluate(instanceScope, instanceNode);
    
               
                if (!lastValue.isValid()){
                    DEBUG_LOG(LogLevel::TRACE, "Invalid value returned from child node evaluation: ", highlight(lastValue.toString(), Colors::orange));
                    continue;
                }
            }

        
        
        DEBUG_FLOW_EXIT();
        return lastValue;
    }


    Node evaluateBlock(const Vector<UniquePtr<BaseAST>>& children, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode){
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        Node lastValue;

        for (const auto& child : children) {
            if (child.get()) {

                DEBUG_LOG(LogLevel::TRACE, "Found Child: ", child->getAstTypeAsString());
                lastValue = child.get()->evaluate(scope, instanceNode);
                if (!lastValue.isValid()){
                    continue;
                }

            } else {
                DEBUG_LOG(LogLevel::TRACE, highlight("Null child node encountered in CodeBlock.", Colors::red));
                DEBUG_LOG(LogLevel::TRACE, "Null child: ", child->getAstTypeAsString());
                throw MerkError("CHILD OF evaluateIf is null");
            }
        }

        DEBUG_FLOW_EXIT();
        return lastValue;
    }



    Node evaluateIf (const IfStatement& ifStatement, SharedPtr<Scope> conditionScope, SharedPtr<ClassInstanceNode> instanceNode) {
        MARK_UNUSED_MULTI(instanceNode);
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        DEBUG_LOG(LogLevel::TRACE, "evaluateIf");
        DEBUG_LOG(LogLevel::TRACE, "Condition Ast Below: ");
        if (ifStatement.getCondition()->evaluate(conditionScope, instanceNode).isTruthy()) {

            DEBUG_FLOW_EXIT();
            return ifStatement.getBody()->evaluate(conditionScope, instanceNode);
        }

        for (const auto& elif : ifStatement.getElifs()) {
            auto val = elif->evaluate(conditionScope, instanceNode);
            if (val.isTruthy()) {
                return elif->getBody()->evaluate(conditionScope, instanceNode);
            }
        }
        
        // auto else_ = ifStatement.getElse();
        if (ifStatement.getElse()) {
            DEBUG_FLOW_EXIT();
            return ifStatement.getElse()->evaluate(conditionScope, instanceNode);
        }

        DEBUG_FLOW_EXIT();
        
        return Node(); 
    };


    Node evaluateElif (const ElifStatement& elifStatement, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) {
        MARK_UNUSED_MULTI(instanceNode);
        DEBUG_FLOW(FlowLevel::LOW);

        if (!elifStatement.getCondition()) {
            DEBUG_FLOW_EXIT();
            throw MerkError("ElIfStatement missing condition in Evaluator::evaluateElif");
        }

        return Node(elifStatement.getCondition()->evaluate(scope, instanceNode).isTruthy());

        DEBUG_FLOW_EXIT();
        return Node();
}

    Node evaluateElse(const CodeBlock& body, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode){
        MARK_UNUSED_MULTI(instanceNode);
        DEBUG_FLOW(FlowLevel::LOW);
        // Keep else behavior consistent with if/elif branches and avoid per-hit scope churn.
        auto val = body.evaluate(scope, instanceNode);
        DEBUG_FLOW_EXIT();
        return val;
}

    Node evaluateBasicLoop(){
        DEBUG_FLOW(FlowLevel::LOW);
        throw MerkError("LoopNode is a base class and cannot be evaluated directly.");
        DEBUG_FLOW_EXIT();
    }

    Node evaluateWhileLoop(const ConditionalBlock& condition, const BaseAST* body, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode){
        DEBUG_FLOW(FlowLevel::LOW);

        if (!body) {
            DEBUG_LOG(LogLevel::INFO, "Error: WhileLoop body is nullptr!");
            DEBUG_FLOW_EXIT();
            throw MerkError("WhileLoop has no body");
        }

        while (true) {
            DEBUG_LOG(LogLevel::TRACE, "About To Evaluate While Loop Condition Result");
 
            Node conditionResult = condition.evaluate(scope, instanceNode);
            DEBUG_LOG(LogLevel::INFO, "While Loop Condition Result: ", conditionResult);
            if (!conditionResult.toBool()) {
                DEBUG_LOG(LogLevel::TRACE, "Condition evaluated to false. Exiting loop.");
                break;
            }

            DEBUG_LOG(LogLevel::TRACE, "Condition evaluated to true. Executing body.");

            try {
                body->evaluate(scope, instanceNode);
            } catch (const ContinueException&){
                DEBUG_LOG(LogLevel::TRACE, "Continue statement encountered. Skipping to next iteration.");
                continue;
            } catch (const BreakException&) {
                DEBUG_LOG(LogLevel::TRACE, "Break statement encountered. Exiting loop.");
                break;  // Exit the loop if a break statement is encountered
            }
        }
        DEBUG_FLOW_EXIT();
        return Node();
    }

    Node evaluateForLoop(const ForLoop& forLoop, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) {
        DEBUG_FLOW(FlowLevel::LOW);
        if (!scope) throw MerkError("evaluateForLoop: scope is null");

        const ASTStatement* startExpr = forLoop.getStartExpr();
        const ASTStatement* endExpr = forLoop.getEndExpr();
        const ASTStatement* stepExpr = forLoop.getStepExpr();
        const CodeBlock* body = forLoop.getBody();
        if (!startExpr || !endExpr || !stepExpr || !body) {
            throw MerkError("evaluateForLoop: for-loop AST is incomplete");
        }

        const Node startNode = startExpr->evaluate(scope, instanceNode);
        const Node endNode = endExpr->evaluate(scope, instanceNode);
        const Node stepNode = stepExpr->evaluate(scope, instanceNode);

        const int start = startNode.toInt();
        const int end = endNode.toInt();
        const int step = stepNode.toInt();
        if (step == 0) {
            throw MerkError("for-loop step cannot be 0");
        }

        const String& loopVar = forLoop.getLoopVariable();
        if (!scope->hasVariable(loopVar)) {
            DataTypeFlags loopFlags;
            loopFlags.isConst = false;
            loopFlags.isMutable = true;
            loopFlags.isStatic = false;
            loopFlags.type = NodeValueType::Int;
            scope->declareVariable(loopVar, makeUnique<VarNode>(Node(start), loopFlags));
        } else {
            scope->updateVariable(loopVar, Node(start));
        }

        int i = start;
        const auto inRange = [&]() { return step > 0 ? (i < end) : (i > end); };

        while (inRange()) {
            scope->updateVariable(loopVar, Node(i));
            try {
                body->evaluate(scope, instanceNode);
            } catch (const ContinueException&) {
                i += step;
                continue;
            } catch (const BreakException&) {
                break;
            }
            i += step;
        }

        DEBUG_FLOW_EXIT();
        return Node();
    }


    Node evaluateBinaryOperation(const String& op, const Node& leftValue, const Node& rightValue, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) {
        MARK_UNUSED_MULTI(instanceNode);
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        evaluatingFor(leftValue, "evaluateBinaryOperation", scope->getScopeLevel());
        evaluatingFor(rightValue, "evaluateBinaryOperation", scope->getScopeLevel());

        DEBUG_LOG(LogLevel::TRACE, "Evaluating BinaryOperation: ", leftValue, " ", op, " ", rightValue);

        // Hot-path for integer arithmetic/comparison in tight loops.
        if (leftValue.isInt() && rightValue.isInt()) {
            const int li = leftValue.toInt();
            const int ri = rightValue.toInt();
            if (op == "+") return Node(li + ri);
            if (op == "-") return Node(li - ri);
            if (op == "*") return Node(li * ri);
            if (op == "/") {
                if (ri == 0) throw MerkError("Division by zero");
                return Node(li / ri);
            }
            if (op == "%") {
                if (ri == 0) throw MerkError("Modulo by zero");
                return Node(li % ri);
            }
            if (op == "==") return Node(li == ri);
            if (op == "!=") return Node(li != ri);
            if (op == "<") return Node(li < ri);
            if (op == "<=") return Node(li <= ri);
            if (op == ">") return Node(li > ri);
            if (op == ">=") return Node(li >= ri);
        }

        Node val;
        if (op == "+") val = leftValue + rightValue;
        else if (op == "-") val = leftValue - rightValue;
        else if (op == "*") val = leftValue * rightValue;
        else if (op == "/") val = leftValue / rightValue;
        else if (op == "%") val = leftValue % rightValue;
        
        else if (op == "and" || op == "&&") val = Node(leftValue.isTruthy() && rightValue.isTruthy());
        else if (op == "or" || op == "||") val = Node(leftValue.isTruthy() || rightValue.isTruthy());

        else if (op == "+=") val = leftValue += rightValue;
        else if (op == "-=") val = leftValue -= rightValue;
        else if (op == "*=") val = leftValue *= rightValue;
        else if (op == "/=") val = leftValue /= rightValue;
        else if (op == "++") val = leftValue += Node(1);

        // Relational operations
        else if (op == "==") {val = Node(leftValue == rightValue);}
        else if (op == "!=") val = Node(leftValue != rightValue);
        else if (op == "<") val = Node(leftValue < rightValue);
        else if (op == "<=") val = Node(leftValue <= rightValue);
        else if (op == ">") val = Node(leftValue > rightValue);
        else if (op == ">=") val = Node(leftValue >= rightValue);



        if (val.isValid()) {
            DEBUG_FLOW_EXIT();
            return val;
        }

        DEBUG_LOG(LogLevel::TRACE,"BINARY OPERATION RESULT: ", val.toString());
        DEBUG_FLOW_EXIT();
        throw MerkError("Unsupported operator: " + op);
    }

    Node evaluateUnaryOperation(const String& op, const Node& operand, SharedPtr<ClassInstanceNode> instanceNode) {
        MARK_UNUSED_MULTI(instanceNode);
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        MARK_UNUSED_MULTI(operand);
        evaluatingFor(operand, "evaluateUnaryOperation");
        Node val;
        if (op == "!" || op == "not") {
            if (operand.isBool() || operand.toBool()) {
                return Node(!operand.toBool());
            }
            DEBUG_FLOW_EXIT();
            throw MerkError("Invalid type for '!': Operand must be boolean.");
        } else if (op == "?") {
            return Node(!operand.isNull());
        } 
        
        else if (op == "-") { return operand.negate(); }

        return Node();
        
        DEBUG_FLOW_EXIT();
        throw MerkError("Unsupported unary operator: " + op);
    }

    [[noreturn]] Node evaluateBreak(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) {
        MARK_UNUSED_MULTI(instanceNode);
        DEBUG_FLOW(FlowLevel::LOW);
        MARK_UNUSED_MULTI(scope);
        DEBUG_LOG(LogLevel::TRACE, "Evaluating break statement in scope level: ", scope->getScopeLevel());
        DEBUG_LOG(LogLevel::TRACE, "Throwing A Break for ", "scope level: ", scope->getScopeLevel());
        DEBUG_FLOW_EXIT();
        throw BreakException();
    }

    [[noreturn]] Node evaluateBreak() {
        DEBUG_FLOW(FlowLevel::VERY_LOW);
        DEBUG_FLOW_EXIT();
        throw BreakException();
    }

} // namespace Evaluator
