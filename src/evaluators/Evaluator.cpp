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

    Node evaluateLiteral(Node value){
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        if (!value.isValid()) {throw MerkError("Literal Value is Not Valid");}
        if (value.isInstance()) {
            throw MerkError("evaluateLiteral result isInstance");
        }
        // if (value.isNull()) {
        //     throw MerkError("Value is null");
        // }
        return value;
    }



    Node evaluateVariableDeclaration(
    String& name,
    const ASTStatement* valueNode,
    DataTypeFlags varMeta,
    SharedPtr<Scope> scope,
    SharedPtr<ClassInstanceNode> instanceNode
){
    MARK_UNUSED_MULTI(instanceNode);
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    SharedPtr<Scope> instanceScope = instanceNode ? instanceNode->getInstanceScope() : scope;
    bool usingInstanceScope = instanceScope != scope;

    String varName = name;

    // 1) Evaluate RHS
    Node rhs = valueNode->evaluate(scope, instanceNode);

    auto& tr = scope->localTypes;

    // 2) Ensure declaredSig is always set (at least Any)
    // Only bind the annotation if it’s not empty/Any.
    const auto anyId = tr.any();

    const auto base = varMeta.fullType.getBaseType();
    if (!base.empty() && base != "Any") {
        varMeta.declaredSig = tr.bindResolvedType(varMeta.fullType, *scope);
        // scope->resolveTypeNameSig(base);
    } else {
        varMeta.declaredSig = anyId;
    }

    // 3) Stamp inferredSig on the RHS value (runtime)
    // (Do this BEFORE VarNode ctor if your VarNode ctor wants to use it.)
    rhs.getFlags().inferredSig = tr.inferFromValue(rhs);

    // 4) Enforce declared type against RHS (if annotation enforced)
    // If declaredSig is Any, allow anything.
    if (varMeta.declaredSig != anyId) {
        auto m = tr.matchValue(varMeta.declaredSig, rhs, {/*opt*/});
        // debugLog(true,
        // "DBG Dict() rhs type=", rhs.getTypeAsString(),
        // " isInstance=", rhs.isInstance(),
        // " baseType=", rhs.getFlags().fullType.getBaseType()
        // );

        if (!m.ok) {
            throw TypeMismatchError(
                tr.toString(varMeta.declaredSig),
                rhs.getTypeAsString(),
                "evaluateVariableDeclaration"
            );
        }
    }
    
    // 5) Construct VarNode (VarNode should NOT clobber inferredSig)
    auto finalVar = makeUnique<VarNode>(rhs, varMeta);
    auto returnVal = finalVar->getValueNode();
    // 6) Store it
    auto targetScope = (usingInstanceScope ? instanceScope : scope);

    
    targetScope->declareVariable(varName, std::move(finalVar));

    // 7) Debug
    // debugLog(true, "Var Declaration Results for var", name, ": ==========================================");
    // valueNode->printAST(std::cout, 0);
    // debugLog(true, "RETURN VAL: ", rhs.toString(),
    //          " declaredSig=", tr.toString(varMeta.declaredSig),
    //          " inferredSig=", tr.toString(rhs.getFlags().inferredSig));

    DEBUG_FLOW_EXIT();

    // IMPORTANT:
    // If var-decl is a statement in your language, returning Node() is fine.
    // If callers rely on the RHS value, return rhs.
    
    // return rhs;
    return returnVal;
}


    Node evaluateVariableAssignment(String name, ASTStatement* value, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode){
        MARK_UNUSED_MULTI(instanceNode);
        DEBUG_FLOW(FlowLevel::PERMISSIVE);

        Node finalVal = value->evaluate(scope, instanceNode);
        if (finalVal.toString() == "var") {throw MerkError("finalVal is var in evaluateVariableAssignment" );};
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
        auto instanceScope = instanceNode ? instanceNode->getInstanceScope() : nullptr;
        auto workingScope = scope;
        auto parent = scope->getParent();
        auto& variable = workingScope->getVariable(name);

        if (instanceScope) {
            scope->removeChildScope(workingScope);
        }
        scope->setParent(parent);

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
        return lastValue; // Return the last evaluated value
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
        return lastValue; // Return the last evaluated value
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

        // Evaluate 'elif' conditions
        for (const auto& elif : ifStatement.getElifs()) {
            auto val = elif->evaluate(conditionScope, instanceNode);
            if (val.isTruthy()) {
                return elif->getBody()->evaluate(conditionScope, instanceNode);
            }
        }

        // Execute 'else' block if all conditions fail
        if (ifStatement.getElse()) {
            DEBUG_FLOW_EXIT();
            return ifStatement.getElse()->evaluate(conditionScope, instanceNode);
        }

        DEBUG_FLOW_EXIT();
        
        return Node();  // Default return if no branch executes
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
        return Node(); // If condition is false, return default node
}

    Node evaluateElse(const CodeBlock& body, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode){
        MARK_UNUSED_MULTI(instanceNode);
        (void)instanceNode;
        DEBUG_FLOW(FlowLevel::LOW);
        auto val = body.evaluate(scope->createChildScope(), instanceNode); // Default to empty node if no body
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
        auto instanceScope = scope;

        if (!body) {
            DEBUG_LOG(LogLevel::INFO, "Error: WhileLoop body is nullptr!");
            DEBUG_FLOW_EXIT();
            throw MerkError("WhileLoop has no body");
        }

        while (true) {
            DEBUG_LOG(LogLevel::TRACE, "About To Evaluate While Loop Condition Result");
 
            Node conditionResult = condition.evaluate(instanceScope, instanceNode);
            DEBUG_LOG(LogLevel::INFO, "While Loop Condition Result: ", conditionResult);
            if (!conditionResult.toBool()) {
                DEBUG_LOG(LogLevel::TRACE, "Condition evaluated to false. Exiting loop.");
                break;  // Exit the loop if condition is false
            }

            DEBUG_LOG(LogLevel::TRACE, "Condition evaluated to true. Executing body.");

            try {
                body->evaluate(instanceScope, instanceNode);
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


    Node evaluateBinaryOperation(const String& op, const Node& leftValue, const Node& rightValue, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) {
        MARK_UNUSED_MULTI(instanceNode);
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        evaluatingFor(leftValue, "evaluateBinaryOperation", scope->getScopeLevel());
        evaluatingFor(rightValue, "evaluateBinaryOperation", scope->getScopeLevel());

        DEBUG_LOG(LogLevel::TRACE, "Evaluating BinaryOperation: ", leftValue, " ", op, " ", rightValue);
        Node val;
        if (op == "+") val = leftValue + rightValue;
        if (op == "-") val = leftValue - rightValue;
        if (op == "*") val = leftValue * rightValue;
        if (op == "/") val = leftValue / rightValue;
        if (op == "%") val = leftValue % rightValue;
        
        if (op == "and" || op == "&&") val = Node(leftValue.isTruthy() && rightValue.isTruthy());
        if (op == "or" || op == "||") val == Node(leftValue.isTruthy() || rightValue.isTruthy());

        if (op == "+=") val = leftValue += rightValue;
        if (op == "-=") val = leftValue -= rightValue;
        if (op == "*=") val = leftValue *= rightValue;
        if (op == "/=") val = leftValue /= rightValue;
        if (op == "++") val = leftValue += Node(1);

        // Relational operations
        if (op == "==") {val = Node(leftValue == rightValue);}
        if (op == "!=") val = Node(leftValue != rightValue);
        if (op == "<") val = Node(leftValue < rightValue);
        if (op == "<=") val = Node(leftValue <= rightValue);
        if (op == ">") val = Node(leftValue > rightValue);
        if (op == ">=") val = Node(leftValue >= rightValue);



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
        
        else if (op == "-"){
            return operand.negate();
        }

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
