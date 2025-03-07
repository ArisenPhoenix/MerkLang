#include <iostream>
#include <stdexcept>
#include <variant>
#include <cassert>
#include <unordered_map>
#include <unordered_set>

#include "core/types.h"
#include "core/node.h"
#include "core/scope.h"
#include "core/evaluator.h"
#include "utilities/helper_functions.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"
#include "core/scope.h"
#include "ast/exceptions.h"
#include "core/errors.h"
#include "ast/ast_base.h"
#include "ast/ast.h"
#include "ast/ast_control.h"


// const bool debugEvaluator = false;


enum class Operator {
    Equals,
    NotEquals,
    LessThan,
    GreaterThan,
    LessThanOrEquals,
    GreaterThanOrEquals,
    Plus,
    Minus,
    Multiply,
    Divide,
    Modulo,
    Invalid
};

Operator stringToOperator(const String& op) {
    static const std::unordered_map<String, Operator> operatorMap = {
        {"==", Operator::Equals},
        {"!=", Operator::NotEquals},
        {"<", Operator::LessThan},
        {">", Operator::GreaterThan},
        {"<=", Operator::LessThanOrEquals},
        {">=", Operator::GreaterThanOrEquals},
        {"+", Operator::Plus},
        {"-", Operator::Minus},
        {"*", Operator::Multiply},
        {"/", Operator::Divide},
        {"%", Operator::Modulo}
    };

    auto it = operatorMap.find(op);
    return (it != operatorMap.end()) ? it->second : Operator::Invalid;
}

void evaluatingFor(const String& value, const String& methodName, int scopeLevel = -2) {
    if (Debugger::getInstance().getLevel() == LogLevel::DEBUG){
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
    if (Debugger::getInstance().getLevel() == LogLevel::DEBUG){
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


    Node evaluateLiteral(Node value, bool isString, bool isBool){
        DEBUG_FLOW();

        if (isBool) {
            DEBUG_LOG(LogLevel::INFO, "Creating A Literal Bool");
            DEBUG_FLOW_EXIT();
            return LitNode(value == Node(true));
        }
        if (isString) {
            DEBUG_LOG(LogLevel::INFO, "Creating A Literal String");
            DEBUG_FLOW_EXIT();
            return LitNode(value.getValue());
        }

        

        debugLog(true, "Node is not a String nor a Bool");
        try {
            int intValue = value.toInt(); 
            DEBUG_LOG(LogLevel::INFO, "Creating A Literal Number");
            DEBUG_FLOW_EXIT();
            return LitNode(intValue);
        } catch (const std::invalid_argument&) {
            DEBUG_FLOW_EXIT();
            throw MerkError("Invalid literal: " + value);
        } catch (const std::out_of_range&) {
            DEBUG_FLOW_EXIT();
            throw MerkError("Literal out of range: " + value);
        }
        
    }

    Node evaluateVariableDeclaration(const ASTStatement* valueNode, VarNode var, SharedPtr<Scope> scope){
        DEBUG_FLOW();
        VarNode resolvedVariable = VarNode(valueNode->evaluate(scope)); // Use the current scope
        scope->declareVariable(var.toString(), makeUnique<VarNode>(resolvedVariable, var.isConst, var.isMutable, var.isStatic)); // Declare variable in scope

        if (!resolvedVariable.isValid()) {
            throw MerkError("Invalid node returned during VariableDeclaration evaluation.");
        }
        DEBUG_FLOW_EXIT();
        return resolvedVariable;
    }

    Node evaluateVariableAssignment(String name, ASTStatement* value, SharedPtr<Scope> scope){
        DEBUG_FLOW();
        if (!scope->hasVariable(name)) {
            DEBUG_FLOW_EXIT();
            throw UndefinedVariableError(name, "VariableAssignmentNode::evaluate");
        }

        auto& var = scope->getVariable(name);
        DEBUG_LOG(LogLevel::INFO, "VariableAssignmentNode: Retrieved variable '", name, 
                "' with current value: ", var.getValue());

        // Evaluate the right-hand side
        auto newValue = value->evaluate(); // Evaluate the RHS
        DEBUG_LOG(LogLevel::INFO, "VariableAssignmentNode: New value for '", name, "': ", newValue);

        if (!newValue.isValid()){
            throw NullVariableError(name);
        }

        DEBUG_LOG(LogLevel::INFO, "========================");
        DEBUG_LOG(LogLevel::INFO, "Assigning: ", newValue, "To scope");
        DEBUG_LOG(LogLevel::INFO, "========================");

        // Update the variable in the scope
        scope->updateVariable(name, newValue);

        DEBUG_LOG(LogLevel::INFO, "VariableAssignmentNode updated: Name =", name,
                ", New Value =", newValue);

        DEBUG_FLOW_EXIT();
        return newValue; // Return the new value for debugging or chaining
    }

    VarNode& evaluateVariableReference(String name, SharedPtr<Scope> scope){
        DEBUG_FLOW();
        
        // Retrieve the variable by reference
        auto& variable = scope->getVariable(name);  // Keep it as VarNode reference

        // Ensure the variable is valid
        if (!variable.isValid()) {
            DEBUG_FLOW_EXIT();
            throw NullVariableError(name);
        }

        DEBUG_LOG(LogLevel::INFO, "VariableReferenceNode resolved: Name=", name, 
                ", Value=", variable.toString());

        DEBUG_FLOW_EXIT();
        return variable;
    }
    
    
    Node evaluateFunction(Vector<UniquePtr<BaseAST>>& children, SharedPtr<Scope> scope){
        DEBUG_FLOW(FlowLevel::LOW);
        Node lastValue;
        try {
            for (const auto& child : children) {
                DEBUG_LOG(LogLevel::INFO, 
                    highlight("Evaluating FunctionBody child:", Colors::orange), 
                    highlight(child->getAstTypeAsString(), Colors::green), 
                    "in CodeBlock scope:", 
                    highlight(std::to_string(scope->getScopeLevel()), Colors::blue)
                );
                                
                child->getScope()->debugPrint();
                lastValue = child->evaluate();
    
               
                if (!lastValue.isValid()){
                    DEBUG_LOG(LogLevel::INFO, "Invalid value returned from child node evaluation: ", highlight(lastValue.toString(), Colors::orange));
                    continue;
                }
            }
        } catch (const ReturnException& e) {
            DEBUG_LOG(LogLevel::INFO, highlight("Caught ReturnException. Returning value:", Colors::red), e.getValue());
            DEBUG_FLOW_EXIT();
            return e.getValue();  // ✅ Extract and return function's result
        }
        
        DEBUG_FLOW_EXIT();
        return lastValue; // Return the last evaluated value
    }

    Node evaluateBlock(const Vector<UniquePtr<BaseAST>>& children, SharedPtr<Scope> scope){
        DEBUG_FLOW();
        Node lastValue;

        for (const auto& child : children) {
            DEBUG_LOG(LogLevel::INFO, "Evaluating child node in CodeBlock: ", child->getAstTypeAsString());
            if (child.get()) {
                lastValue = child.get()->evaluate(scope);
                if (!lastValue.isValid()){
                    
                    DEBUG_LOG(LogLevel::INFO, "Invalid value returned from child node evaluation: ", lastValue.toString());
                    DEBUG_LOG(LogLevel::INFO, "Container for invalid child: ", child->getAstTypeAsString());
                    continue;
                }

            } else {
                DEBUG_LOG(LogLevel::INFO, "Null child node encountered in CodeBlock.");
                DEBUG_LOG(LogLevel::INFO, "Null child: ", child->getAstTypeAsString());
            }
        }

        DEBUG_FLOW_EXIT();
        return lastValue; // Return the last evaluated value
    }

    Node evaluateIf (const IfStatement& ifStatement, SharedPtr<Scope> scope) {
        DEBUG_FLOW(FlowLevel::LOW);

        if (ifStatement.getCondition()->evaluate(scope).toBool()) {
            DEBUG_FLOW_EXIT();
            return ifStatement.getBody()->evaluate(scope);
        }

        // Evaluate 'elif' conditions
        for (const auto& elif : ifStatement.getElifs()) {
            if (elif->getCondition()->evaluate(scope).toBool()) {
                DEBUG_FLOW_EXIT();
                // elif will execute if condition is true
                return elif->evaluate(scope);
            }
        }

        // Execute 'else' block if all conditions fail
        if (ifStatement.getElse()) {
            DEBUG_FLOW_EXIT();
            return ifStatement.getElse()->evaluate(scope);
        }

        DEBUG_FLOW_EXIT();
        return Node();  // Default return if no branch executes
    };


    Node evaluateElif (const ElifStatement& elifStatement, SharedPtr<Scope> scope) {
        DEBUG_FLOW(FlowLevel::LOW);
        if (!elifStatement.getCondition()) {
            DEBUG_FLOW_EXIT();
            throw MerkError("ElIfStatement missing condition in Evaluator::evaluateElif");
        }


        // 1️⃣ Evaluate the condition
        if (elifStatement.getCondition()->evaluate(scope).toBool()) {
            DEBUG_FLOW_EXIT();
            return elifStatement.getBody()->evaluate(scope); // 2️⃣ Execute body if condition is true
        } 

        DEBUG_FLOW_EXIT();
        return Node(); // If condition is false, return default node
}

    Node evaluateElse(const CodeBlock& body, SharedPtr<Scope> scope){
        // (void)scope;
        DEBUG_FLOW(FlowLevel::LOW);
        auto val = body.evaluate(scope); // Default to empty node if no body
        DEBUG_FLOW_EXIT();
        return val;
}

    Node evaluateBasicLoop(){
        DEBUG_FLOW();
        throw MerkError("LoopNode is a base class and cannot be evaluated directly.");
        DEBUG_FLOW_EXIT();
    }

    Node evaluateWhileLoop(const ConditionalBlock& condition, const BaseAST* body, SharedPtr<Scope> scope){
        DEBUG_FLOW();
        if (!body) {
            DEBUG_LOG(LogLevel::INFO, "Error: WhileLoop body is nullptr!");
            DEBUG_FLOW_EXIT();
            throw MerkError("WhileLoop has no body");
            // return Node(); // Return an empty node to prevent a crash
        }
        while (true) {
            // Evaluate the condition
            Node conditionResult = condition.evaluate(scope);
            DEBUG_LOG(LogLevel::INFO, "While Loop Condition Result: ", conditionResult);
            if (!conditionResult.toBool()) {
                DEBUG_LOG(LogLevel::INFO, "Condition evaluated to false. Exiting loop.");
                break;  // Exit the loop if condition is false
            }

            DEBUG_LOG(LogLevel::INFO, "Condition evaluated to true. Executing body.");

            try {
                // Evaluate the body
                body->evaluate(scope);
            } catch (const BreakException&) {
                DEBUG_LOG(LogLevel::INFO, "Break statement encountered. Exiting loop.");
                break;  // Exit the loop if a break statement is encountered
            }
        }

        DEBUG_LOG(LogLevel::INFO, "Finished evaluating WhileLoopNode.");
        DEBUG_FLOW_EXIT();
        return Node();  // Return a default Node, as WhileLoop doesn't produce a value
    }

    Node evaluateBinaryOperation(const String& op, const Node& leftValue, const Node& rightValue, SharedPtr<Scope> scope) {
        DEBUG_FLOW(FlowLevel::LOW);
        evaluatingFor(leftValue, "evaluateBinaryOperation", scope->getScopeLevel());
        evaluatingFor(rightValue, "evaluateBinaryOperation", scope->getScopeLevel());

        if (!leftValue.isValid() || !rightValue.isValid()) {
            DEBUG_FLOW_EXIT();
            throw MerkError("Invalid operands for BinaryOperationNode.");
        }

        DEBUG_LOG(LogLevel::INFO, "Evaluating BinaryOperation: ", leftValue, " ", op, " ", rightValue);
        Node val;
        // DEBUG_FLOW_EXIT();
        // Perform the operation using Node's overloaded operators
        if (op == "+") val = leftValue + rightValue;
        if (op == "-") val = leftValue - rightValue;
        if (op == "*") val = leftValue * rightValue;
        if (op == "/") val = leftValue / rightValue;
        if (op == "%") val = leftValue % rightValue;

        // if (op == "+=") val = leftValue.plusEquals(rightValue);
        // if (op == "-=") val = leftValue -= rightValue;
        // if (op == "*=") val = leftValue *= rightValue;
        // if (op == "/=") val = leftValue /= rightValue;

        // Relational operations
        if (op == "==") val = Node(leftValue == rightValue);
        if (op == "!=") val = Node(leftValue != rightValue);
        if (op == "<") val = Node(leftValue < rightValue);
        if (op == "<=") val = Node(leftValue <= rightValue);
        if (op == ">") val = Node(leftValue > rightValue);
        if (op == ">=") val = Node(leftValue >= rightValue);


        if (val.isValid()){
            DEBUG_FLOW_EXIT();
            return val;
        }
        DEBUG_FLOW_EXIT();
        throw MerkError("Unsupported operator: " + op);
    }

    Node evaluateUnaryOperation(const String& op, const Node& operand) {
        DEBUG_FLOW(FlowLevel::LOW);
        evaluatingFor(operand, "evaluateUnaryOperation");
        if (op == "!") {
            if (operand.isBool()) {
                return Node(!operand.toBool());
            }
            DEBUG_FLOW_EXIT();
            throw MerkError("Invalid type for '!': Operand must be boolean.");
        }
        DEBUG_FLOW_EXIT();
        throw MerkError("Unsupported unary operator: " + op);
    }

    [[noreturn]] Node evaluateBreak(SharedPtr<Scope> scope) {
        DEBUG_FLOW(FlowLevel::LOW);
        DEBUG_LOG(LogLevel::INFO, "Evaluating break statement in scope level: ", scope->getScopeLevel());
        DEBUG_LOG(LogLevel::INFO, "Throwing A Break for ", "scope level: ", scope->getScopeLevel());
        DEBUG_FLOW_EXIT();
        throw BreakException();
    }

    [[noreturn]] Node evaluateBreak() {
        DEBUG_FLOW(FlowLevel::VERY_LOW);
        DEBUG_FLOW_EXIT();
        throw BreakException();
    }


    Vector<Node> evaluateFunctionArguments(const Vector<UniquePtr<ASTStatement>>& argumentNodes, const SharedPtr<Scope>& scope) {
        DEBUG_FLOW(FlowLevel::MED);
        Vector<Node> evaluatedArguments;

        for (const auto& argNode : argumentNodes) {
            if (!argNode) {
                throw MerkError("Null argument node encountered during evaluation.");
            }

            // Evaluate each argument node in the given scope
            Node evaluatedArg = argNode->evaluate(scope);
            evaluatedArguments.push_back(evaluatedArg);
        }
        DEBUG_FLOW_EXIT();
        return evaluatedArguments;
    }



} // namespace Evaluator
