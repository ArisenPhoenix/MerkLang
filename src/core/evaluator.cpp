#include <iostream>
#include <stdexcept>
#include <variant>
#include <cassert>
#include <unordered_map>
#include <unordered_set>

#include "core/types.h"
#include "core/errors.h"

#include "core/node.h"
#include "core/scope.h"
#include "core/evaluator.h"
#include "core/scope.h"
#include "core/classes/method.h"
#include "core/helpers/class_helpers.h"

#include "utilities/helper_functions.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"

#include "ast/ast_base.h"
#include "ast/ast.h"

#include "ast/exceptions.h"
#include "ast/ast_control.h"
#include "ast/ast_chain.h"
#include "ast/ast_class.h"
#include "ast/ast_validate.h"



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

bool isDebug = Debugger::getInstance().getLogLevel() == LogLevel::DEBUG;

void evaluatingFor(const String& value, const String& methodName, int scopeLevel = -2) {
    #if !ENABLE_DEBUG_LOGGING
        MARK_UNUSED_MULTI(value, methodName);
    #endif
    
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
    #if !ENABLE_DEBUG_LOGGING
        MARK_UNUSED_MULTI(value, methodName);
    #endif
    (void)value;
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

    Node evaluateLiteral(Node value, bool isString, bool isBool){
        (void)isString;
        (void)isBool;
        DEBUG_FLOW(FlowLevel::LOW);
        return value;
    }

    Node evaluateVariableDeclaration(const ASTStatement* valueNode, VarNode var, SharedPtr<Scope> scope){
        DEBUG_FLOW(FlowLevel::LOW);
        DEBUG_LOG(LogLevel::TRACE, "Evaluating Variable Declaration");
        // scope->debugPrint();
        VarNode resolvedVariable = VarNode(valueNode->evaluate(scope)); // Use the current scope
        size_t before = scope->getContext().getVariables().size();
        scope->declareVariable(var.toString(), makeUnique<VarNode>(resolvedVariable, var.isConst, var.isMutable, var.isStatic)); // Declare variable in scope

        if (!resolvedVariable.isValid()) {
            throw MerkError("Invalid node returned during VariableDeclaration evaluation.");
        }

        if (scope->getContext().getVariables().size() == before) {
            throw MerkError("No Varialbes Declared");
        } 
        // else {
        //     DEBUG_LOG(LogLevel::PERMISSIVE, "Variable", var.toString(), "Was Declared In The Below Scope: ");
        //     scope->debugPrint();
        //     // scope->printChildScopes();
        // }
        DEBUG_FLOW_EXIT();
        return resolvedVariable;
    }

    Node evaluateVariableAssignment(String name, ASTStatement* value, SharedPtr<Scope> scope){
        // (void)scope;
        DEBUG_FLOW(FlowLevel::LOW);
        if (!scope->hasVariable(name)) {
            DEBUG_FLOW_EXIT();
            throw UndefinedVariableError(name, "VariableAssignmentNode::evaluate");
        }
        
        auto newValue = value->evaluate(scope); // Evaluate the RHS
        DEBUG_LOG(LogLevel::TRACE, "VariableAssignmentNode: New value for '", name, "': ", newValue);

        if (!newValue.isValid()){
            throw NullVariableError(name);
        }

        DEBUG_LOG(LogLevel::TRACE, "========================");
        DEBUG_LOG(LogLevel::TRACE, "Assigning: ", newValue, "To scope");
        DEBUG_LOG(LogLevel::TRACE, "========================");

        // Update the variable in the scope
        scope->updateVariable(name, newValue);

        DEBUG_LOG(LogLevel::TRACE, "VariableAssignmentNode updated: Name =", name, ", New Value =", newValue);

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

    Node evaluateClassBody(SharedPtr<Scope> classCapturedScope, SharedPtr<Scope> classScope, SharedPtr<Scope> generatedScope, String accessor, Vector<UniquePtr<BaseAST>>& children) {
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        if (!classCapturedScope) {
            throw MerkError("Class Captured Scope Was Not Set On Body");
        }

        if (!classScope) {
            throw MerkError("Class Scope Was Not Set On Body");
        }

        if (accessor.empty()) {
            throw MerkError("Accessor Was Not Set On Body");
        }


        generatedScope->owner = generateScopeOwner("Class", "Body");
        DEBUG_LOG(LogLevel::TRACE, highlight("Moving to Apply Accessor Scope Fix", Colors::yellow));
        Vector<String> methods;
        for (const auto& child : children) {
            switch (child->getAstType()) {
                case AstType::VariableDeclaration:                // protected variables stored in captured scope, 
                    child->evaluate(classScope->getParent());
                break;
                case AstType::ClassMethodDef:
                    if (child->getAstType() == AstType::ClassMethodDef){
                        auto* methodDef = static_cast<MethodDef*>(child.get());
                        DEBUG_LOG(LogLevel::TRACE, highlight("Handling Method: " + methodDef->getName(), Colors::yellow));

                        auto methodScope = classScope->createChildScope();
                        if (!methodScope){
                            throw MerkError("generated methodscope is null for method in ClassDef::evaluate");
                        }
                                        // for providing its own classScope from which to work with
                        methodDef->setClassScope(classScope);
                        methodDef->getBody()->getScope()->owner = generateScopeOwner("MethodDefBody", methodDef->getName());
                        methodDef->setScope(methodScope);
                        if (methodDef->getClassScope().get() != classScope.get()) {
                            throw MerkError("method class Scope is not the same as cls->classScope");
                        }

                        Vector<Chain*> nonStaticElements = applyAccessorScopeFix(methodDef, classScope, accessor);
                        
                        stripImplicitAccessor(methodDef, accessor);
                        methods.emplace_back(methodDef->getName());
                        // methodDef->setNonStaticElements(nonStaticElements);
                        // setNonStaticElements(nonStaticElements);
                        methodDef->evaluate(classScope);                    
                    } 
                    break;

                // case AstType::VariableAssignment:
                // case AstType::ParameterAssignment:
                case AstType::ClassDefinition:
                    break;

                default:
                    // child->evaluate(classScope);
                    DEBUG_LOG(LogLevel::ERROR, "Unhandled AST type in ClassBody:", child->getAstTypeAsString());
                    throw MerkError("Unexpected AST statement in ClassBody: " + child->getAstTypeAsString());
            }
        }


        for (auto& method : methods){
            DEBUG_LOG(LogLevel::TRACE, "Assuming Method ", highlight(method, Colors::blue), "Was Registered");
            if (!classScope->hasFunction(method)){
                throw MerkError("Method: " + method + " Was Not Registered To Class Scope");
            }
        }

        DEBUG_LOG(LogLevel::TRACE, highlight("Finished Applying Accessor Scope Fix", Colors::yellow));
        DEBUG_FLOW_EXIT();
        return Node();
    }
    
    Node evaluateMethod(Vector<UniquePtr<BaseAST>>& children, SharedPtr<Scope> methodScope, SharedPtr<ClassInstanceNode> instanceNode){
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        if (!instanceNode){throw MerkError("Evaluator::evaluateMethod has no instanceNode");}

        // MARK_UNUSED_MULTI(scope);
        DEBUG_LOG(LogLevel::PERMISSIVE, "Num Children in MethodBody: ", children.size());
        auto spaces = String(10, '\n');
        debugLog(true, spaces);

        Node lastValue;
        for (const auto& child : children) {
            // DEBUG_LOG(LogLevel::PERMISSIVE, 
            //     highlight("Evaluating MethodBody child:", Colors::orange), 
            //     highlight(child->getAstTypeAsString(), Colors::green), 
            //     "in CodeBlock scope:", 
            //     highlight(std::to_string(scope->getScopeLevel()), Colors::blue)
            // );

            // if (child->getAstType() == AstType::ChainOperation) {
            //     // child->printAST(std::cout, 10);
            //     // for (auto& astChild : child->getAllAst(true)) {
            //     //     auto currentScope = astChild->getScope();
            //     //     // currentScope->debugPrint();
            //     //     // currentScope->printChildScopes();
            //     // }
            //     auto thisChild = static_unique_ptr_cast<ChainOperation>(child);

            // }
                            
            // child->getScope()->debugPrint();
            lastValue = child->evaluate(methodScope, instanceNode);

            // DEBUG_LOG(LogLevel::PERMISSIVE, "current LastValue: ", lastValue);
            if (!lastValue.isValid()){
                // DEBUG_LOG(LogLevel::PERMISSIVE, "Invalid value returned from child node evaluation: ", highlight(lastValue.toString(), Colors::orange));
                continue;
            }
        }
        
        methodScope->debugPrint();
        methodScope->printChildScopes();
        debugLog(true, spaces);

        DEBUG_FLOW_EXIT();

        return lastValue; // Return the last evaluated value
    }

    Node evaluateFunction(Vector<UniquePtr<BaseAST>>& children, SharedPtr<Scope> scope){
        DEBUG_FLOW(FlowLevel::LOW);
        MARK_UNUSED_MULTI(scope);

        Node lastValue;
        // try {
            for (const auto& child : children) {

                DEBUG_LOG(LogLevel::TRACE, 
                    highlight("Evaluating FunctionBody child:", Colors::orange), 
                    highlight(child->getAstTypeAsString(), Colors::green), 
                    "in CodeBlock scope:", 
                    highlight(std::to_string(scope->getScopeLevel()), Colors::blue)
                );
                                
                // child->getScope()->debugPrint();
                lastValue = child->evaluate();
    
               
                if (!lastValue.isValid()){
                    DEBUG_LOG(LogLevel::TRACE, "Invalid value returned from child node evaluation: ", highlight(lastValue.toString(), Colors::orange));
                    continue;
                }
            }
        // } catch (const ReturnException& e) {
        //     DEBUG_LOG(LogLevel::INFO, highlight("Caught ReturnException. Returning value:", Colors::red), e.getValue());
        //     DEBUG_FLOW_EXIT();
        //     return e.getValue();  // Extract and return function's result
        // }
        
        DEBUG_FLOW_EXIT();
        return lastValue; // Return the last evaluated value
    }

    Node evaluateBlock(const Vector<UniquePtr<BaseAST>>& children, SharedPtr<Scope> scope){
        DEBUG_FLOW(FlowLevel::HIGH);
        Node lastValue;

        for (const auto& child : children) {
            // DEBUG_LOG(LogLevel::INFO, "Evaluating child node in CodeBlock: ", child->getAstTypeAsString());
            if (child.get()) {

                DEBUG_LOG(LogLevel::TRACE, "Found Child: ", child->getAstTypeAsString());
                lastValue = child.get()->evaluate(scope);
                DEBUG_LOG(LogLevel::DEBUG, "Child Evaluated To: ", lastValue.toString());
                if (!lastValue.isValid()){
                    
                    DEBUG_LOG(LogLevel::TRACE, "Invalid value returned from child node evaluation: ", lastValue.toString());
                    DEBUG_LOG(LogLevel::TRACE, "Container for invalid child: ", child->getAstTypeAsString());
                    continue;
                }

            } else {
                DEBUG_LOG(LogLevel::TRACE, highlight("Null child node encountered in CodeBlock.", Colors::red));
                DEBUG_LOG(LogLevel::TRACE, "Null child: ", child->getAstTypeAsString());
            }
        }

        DEBUG_FLOW_EXIT();
        return lastValue; // Return the last evaluated value
    }




    Node evaluateIf (const IfStatement& ifStatement, SharedPtr<Scope> scope) {
        DEBUG_FLOW(FlowLevel::LOW);
        DEBUG_LOG(LogLevel::TRACE, "evaluateIf");
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

        
        // Evaluate the condition
        if (elifStatement.getCondition()->evaluate(scope).toBool()) {
            DEBUG_FLOW_EXIT();
            return elifStatement.getBody()->evaluate(scope); // Execute body if condition is true
        } 

        DEBUG_FLOW_EXIT();
        return Node(); // If condition is false, return default node
}

    Node evaluateElse(const CodeBlock& body, SharedPtr<Scope> scope){
        (void)scope;
        DEBUG_FLOW(FlowLevel::LOW);
        auto val = body.evaluate(); // Default to empty node if no body
        DEBUG_FLOW_EXIT();
        return val;
}

    Node evaluateBasicLoop(){
        DEBUG_FLOW(FlowLevel::LOW);
        throw MerkError("LoopNode is a base class and cannot be evaluated directly.");
        DEBUG_FLOW_EXIT();
    }

    Node evaluateWhileLoop(const ConditionalBlock& condition, const BaseAST* body, SharedPtr<Scope> scope){
        (void)scope;
        DEBUG_FLOW(FlowLevel::LOW);
        if (!body) {
            DEBUG_LOG(LogLevel::INFO, "Error: WhileLoop body is nullptr!");
            DEBUG_FLOW_EXIT();
            throw MerkError("WhileLoop has no body");
            // return Node(); // Return an empty node to prevent a crash
        }
        while (true) {
            // Evaluate the condition
            DEBUG_LOG(LogLevel::TRACE, "About To Evaluate While Loop Condition Result");
 
            Node conditionResult = condition.evaluate();
            DEBUG_LOG(LogLevel::INFO, "While Loop Condition Result: ", conditionResult);
            if (!conditionResult.toBool()) {
                DEBUG_LOG(LogLevel::TRACE, "Condition evaluated to false. Exiting loop.");
                break;  // Exit the loop if condition is false
            }

            DEBUG_LOG(LogLevel::TRACE, "Condition evaluated to true. Executing body.");

            try {
                // Evaluate the body
                body->evaluate();
            } catch (const ContinueException&){
                DEBUG_LOG(LogLevel::TRACE, "Continue statement encountered. Skipping to next iteration.");
                continue;
            } catch (const BreakException&) {
                DEBUG_LOG(LogLevel::TRACE, "Break statement encountered. Exiting loop.");
                break;  // Exit the loop if a break statement is encountered
            }
        }
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

        // if (op == "and") val = leftValue && rightValue;
        // if (op == "or") val == leftValue || rightValue;

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
        MARK_UNUSED_MULTI(operand);
        evaluatingFor(operand, "evaluateUnaryOperation");
        Node val;
        if (op == "!" or op == "not") {
            if (operand.isBool()) {
                return Node(!operand.toBool());
            }
            DEBUG_FLOW_EXIT();
            throw MerkError("Invalid type for '!': Operand must be boolean.");
        } else if (op == "-"){
            return operand.negate();
        }
        
        DEBUG_FLOW_EXIT();
        throw MerkError("Unsupported unary operator: " + op);
    }

    [[noreturn]] Node evaluateBreak(SharedPtr<Scope> scope) {
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

    Node evaluateClassCall(SharedPtr<Scope> callScope, String className, Vector<Node> argValues) {
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        

        auto classOpt = callScope->getClass(className);
        if (!classOpt.has_value()) {throw MerkError("Class not found: " + className);}

        auto classSig = classOpt.value();

        auto classTemplate = classSig->getClassDef();
        auto capturedScope = classTemplate->getCapturedScope();
        auto capturedClone = capturedScope->clone(true);  // clone it safely
        auto classScope = classTemplate->getClassScope();

        SharedPtr<Scope> instanceScope = capturedClone->makeInstanceScope(classTemplate->getClassScope());
        capturedClone->appendChildScope(instanceScope);
        if (!instanceScope){throw MerkError("InstanceScope creation failed in ClassCall::evaluate()");}

        
        // callScope->appendChildScope(instanceScope);

        auto captured = instanceScope->getParent();
        capturedClone->owner = generateScopeOwner("InstanceCaptured", classTemplate->getName());
        if (!captured){throw MerkError("Captured Scope Does Not Exist When Instantiating class: " + classTemplate->getName());}
        if (!captured->has(instanceScope)){captured->printChildScopes();instanceScope->printChildScopes();throw MerkError("Instance Scope does not live in captured Scope");} 
        else {DEBUG_LOG(LogLevel::PERMISSIVE, "Instance Scope Lives in Captured Scope");}
        auto params = classTemplate->getParameters().clone();
        DEBUG_LOG(LogLevel::PERMISSIVE, "PASSED PARAMS For ClassCall::evaluate");

        instanceScope->owner = generateScopeOwner("ClassInstance", classTemplate->getName());
        SharedPtr<ClassInstance> instance = makeShared<ClassInstance>(classTemplate->getQualifiedName(), captured, instanceScope, params, classTemplate->getQualifiedAccessor());
        // SharedPtr<ClassInstanceNode> classInstance = makeShared<ClassInstanceNode>(instance)
        DEBUG_LOG(LogLevel::PERMISSIVE, "ABOUT to construct instance");

        instance->construct(argValues, instance); 
        DEBUG_LOG(LogLevel::PERMISSIVE, "Instance Constructed");

        DEBUG_FLOW_EXIT();
        return ClassInstanceNode(instance);
    }
    Node evaluateMethodDef(
        SharedPtr<Scope> passedScope, 
        SharedPtr<Scope> ownScope, 
        SharedPtr<Scope> classScope, 
        String methodName, 
        CallableBody* body, 
        ParamList parameters, 
        CallableType callType, 
        CallableType methodType) {
        DEBUG_FLOW();
        if (!passedScope){
            throw MerkError("Provided Scope to MethodDef::evaluate is null");
        }

        if (!ownScope){
            throw MerkError("MethodDef::evaluate, scope is null");
        }

        if (!classScope) {
            throw MerkError("Class Scope was not supplied to Method: " + methodName);
        }


        auto freeVarNames = body->collectFreeVariables();
        DEBUG_LOG(LogLevel::TRACE,"MethodDef::evaluate | ", "CallType: ", callableTypeAsString(callType), "SubType: ", callableTypeAsString(methodType));
        
        if (callType == CallableType::FUNCTION){
            FreeVars tempFreeVars = freeVarNames;
            for (auto& param : parameters){
                auto it = tempFreeVars.find(param.getName()); // find a matching param name
                if (it != tempFreeVars.end()){                // indicates a match
                    tempFreeVars.erase(it);
                }
            }

            
            if (tempFreeVars.size() > 0){
                std::ostringstream oss;
                for (auto& var : tempFreeVars){
                    DEBUG_LOG(LogLevel::TRACE, highlight("'", Colors::yellow), highlight(var, Colors::purple), highlight("'", Colors::yellow), " ");
                }
                throw MerkError("The Following Vars: " + oss.str() + "; were defined outside of function defined using function");
            }
        } 
        

        SharedPtr<Scope> defScope = passedScope->detachScope(freeVarNames);

        if (!defScope){
            throw MerkError("defScope created in MethodDef::evaluate is null");
        }

        defScope->isCallableScope = true;
        defScope->owner = generateScopeOwner("MethodDef", methodName); 

        if (!body->getScope()){
            DEBUG_LOG(LogLevel::ERROR, "Body's Scope is null in FunctionDef::evaluate()");
            throw MerkError("Scope not present in FunctionDef::evaluate(scope)");
        }

        UniquePtr<BaseAST> clonedBodyBase = body->clone();
        auto clonedBody = static_unique_ptr_cast<MethodBody>(std::move(clonedBodyBase));
        // clonedBody->setNonStaticElements(nonStaticElements);
        
        if (!clonedBody){
            throw MerkError("ClonedBody in MethodDef::evaluate is null");
        }
    
        if (!clonedBody->getScope()){
            DEBUG_LOG(LogLevel::ERROR, "Body's Scope is null in FunctionDef::evaluate()");
            throw MerkError("Scope not present in FunctionDef::evaluate(scope) of clonedBody");
        }

        DEBUG_LOG(LogLevel::TRACE, "FunctionDef Defining Scope: ", passedScope->getScopeLevel());
        
        SharedPtr<Method> method = makeShared<Method>(methodName, parameters, std::move(clonedBody), ownScope, callType);
        
        if (!method){
            throw MerkError("Method created in MethodDef::evaluate is null");
        }
        
        method->setSubType(methodType);
        method->setScope(defScope);
        method->setCapturedScope(defScope);

        auto methodSig = method->toCallableSignature();

        classScope->registerFunction(methodName, methodSig);

        DEBUG_LOG(LogLevel::TRACE, highlight("Registered Method: " + methodName, Colors::yellow), "into: ", classScope.get(), "Owner: ", classScope->owner);

        DEBUG_LOG(LogLevel::TRACE, "Method", methodName, "registered to ", classScope->owner);

        if (!defScope){
            DEBUG_FLOW_EXIT();
            throw MerkError("Defining Scope for FunctionDef::evaluate is null");
        }

        if (!method->getBody()){
            DEBUG_FLOW_EXIT();
            throw MerkError("Function body is null in FunctionDef::evaluate");
        }

        // Wrap it in a FunctionNode (for returning it as a value)
        MethodNode methodNode(method);

        DEBUG_FLOW_EXIT();
        return methodNode;
    }

} // namespace Evaluator
