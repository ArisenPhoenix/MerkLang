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
#include "core/Evaluator.hpp"
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
#include "core/helpers/class_helpers.h"
#include "core/callables/functions/Function.hpp"
#include "core/callables/classes/Method.hpp"

#include "core/types/Type.hpp"


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

    Node evaluateVariableDeclaration(String& name, const ASTStatement* valueNode, DataTypeFlags varMeta, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode){
        MARK_UNUSED_MULTI(instanceNode);
        DEBUG_FLOW(FlowLevel::PERMISSIVE);    
        SharedPtr<Scope> instanceScope = instanceNode ? instanceNode->getInstanceScope() : scope;
        bool usingInstanceScope = instanceScope != scope;
        
        String varName = name;
        
        auto resolvedVariable = valueNode->evaluate(scope, instanceNode);
        // VarNode finalVar;
        // auto final = VarNode(resolvedVariable, var.isConst, var.isMutable, typeTag, var.isStatic);
        // if (varName != "dict3") { throw MerkError("VAR DEC -1"); }
        // if (var.getFullType().getBaseType().size()) {
            
            
        // }
        auto finalVar = makeUnique<VarNode>(resolvedVariable, varMeta);
        // if (finalVar->getValueNode().isNull()) {throw MerkError("Variable Declared is Null");}
        
        auto returnVal = finalVar->getValueNode();
        // if (!finalVar->getValueNode().isValid()) {
        //     throw MerkError("VALUE NODE IS NOT VALID");
        // }
        // else {
            
        //     finalVar = VarNode(resolvedVariable, var.getIsConst(), var.getIsMutable(), typeTag, var.getIsStatic());
        // }
        // auto final = VarNode(resolvedVariable, var.isConst, var.isMutable, var.getFullType(), var.isStatic);
        // auto thing = VarNode(finalVar);
        // auto varNode = makeUnique<VarNode>(finalVar);
        
        if (usingInstanceScope && instanceScope->hasMember(varName)) {
            
            instanceScope->declareVariable(varName, std::move(finalVar));
        } 

        else {
            scope->declareVariable(varName, std::move(finalVar));
        }
        if (name.empty() || varName.empty()) {throw MerkError("VarName Is empty in evaluateVariableDeclaration");}
        // if (name == "list") {throw MerkError("Evaluated VariableDeclaration for list");}
        // scope->declareVariable(varName, std::move(varNode));
        
        DEBUG_FLOW_EXIT();
        if (!returnVal.isValid()) {throw MerkError("VariableDeclaration returnVal is not valid");}
        // return returnVal;
        return Node();
    }

    Node evaluateVariableAssignment(String name, ASTStatement* value, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode){
        MARK_UNUSED_MULTI(instanceNode);
        DEBUG_FLOW(FlowLevel::PERMISSIVE);

        Node finalVal = value->evaluate(scope, instanceNode);
        if (finalVal.toString() == "var") {throw MerkError("finalVal is var in evaluateVariableAssignment" );};
        DEBUG_LOG(LogLevel::TRACE, "========================");
        DEBUG_LOG(LogLevel::TRACE, "Assigning: ", finalVal.toString() + " META: " + finalVal.getFlags().toString());
        DEBUG_LOG(LogLevel::TRACE, "========================");
        scope->updateVariable(name, finalVal);
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
        // if (variable.getValueNode().isNull()) {
        //     throw MerkError("Variable " + name + " Is Null For Some Reason");
        // }
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
    
    Node evaluateMethodBody(Vector<UniquePtr<BaseAST>>& children, SharedPtr<Scope> methodScope, SharedPtr<ClassInstanceNode> instanceNode){
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        if (!instanceNode){throw MerkError("Evaluator::evaluateMethod has no instanceNode");}

        

        Node lastValue;
        for (const auto& child : children) {
            
            if (child->getAstType() == AstType::ChainOperation) {
                auto chain = static_unique_ptr_cast<ChainOperation>(child->clone());
                for (auto& elem: chain->getLeft()->getElements()) {
                    if (elem.object->getAstType() == AstType::ClassMethodCall) {
                        DEBUG_LOG(LogLevel::TRACE, highlight("Evaluating a nested method call ========================================================================================", Colors::bg_bright_red));

                        auto current = static_unique_ptr_cast<MethodCall>(elem.object->clone());                        
                    }
                }

            }

            lastValue = child->evaluate(methodScope, instanceNode);
            
        }

        DEBUG_FLOW_EXIT();
        return lastValue; // Return the last evaluated value
    }

    Node evaluateClassBody(SharedPtr<Scope> classCapturedScope, SharedPtr<Scope> classScope, SharedPtr<Scope> generatedScope, String accessor, Vector<UniquePtr<BaseAST>>& children, SharedPtr<ClassInstanceNode> instanceNode) {
        MARK_UNUSED_MULTI(classCapturedScope, instanceNode);
        DEBUG_FLOW(FlowLevel::PERMISSIVE);

        if (!classCapturedScope) {throw MerkError("Class Captured Scope Was Not Set On Body");}

        if (!classScope) {throw MerkError("Class Scope Was Not Set On Body");}

        if (accessor.empty()) {throw MerkError("Accessor Was Not Set On Body");}


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
                        DEBUG_LOG(LogLevel::TRACE, "CREATING METHOD CALL SCOPE");
                        auto methodScope = classScope->createChildScope();
                        DEBUG_LOG(LogLevel::TRACE, "CREATED METHOD CALL SCOPE");
                        if (!methodScope){throw MerkError("generated methodscope is null for method in ClassDef::evaluate");} 

                        methodDef->setClassScope(classScope);
                        if (methodDef->getClassScope().get() != classScope.get()) {throw MerkError("method class Scope is not the same as cls->classScope");}

                        Vector<Chain*> nonStaticElements = applyAccessorScopeFix(methodDef, classScope, accessor);
                        
                        stripImplicitAccessor(methodDef, accessor);
                        methods.emplace_back(methodDef->getName());
                        // methodDef->setNonStaticElements(nonStaticElements);
                        methodDef->evaluate(classScope, instanceNode);
                    } 
                    break;

                // case AstType::VariableAssignment:
                // case AstType::ParameterAssignment:
                case AstType::ClassDefinition:
                    // Will be handled later
                    break;

                default:
                    // child->evaluate(classScope);
                    DEBUG_LOG(LogLevel::ERROR, "Unhandled AST type in ClassBody:", child->getAstTypeAsString());
                    throw MerkError("Unexpected AST statement in ClassBody: " + child->getAstTypeAsString());
            }
        }

        DEBUG_LOG(LogLevel::TRACE, highlight("Finished Applying Accessor Scope Fix", Colors::yellow));
        DEBUG_FLOW_EXIT();
        return Node();
    }
    

    Node evaluateClassCall(SharedPtr<Scope> callScope, String className, ArgumentList argValues, SharedPtr<ClassInstanceNode> instanceNode) {
        MARK_UNUSED_MULTI(instanceNode);
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        auto classOpt = callScope->getClass(className);
        if (!classOpt.has_value()) {throw MerkError("Class not found: " + className);}

        auto classSig = classOpt.value();
        if (!classSig) {throw MerkError("Class signature is null for class: " + className);}
        
        auto classTemplate = classSig->getClassDef();
        
        SharedPtr<Scope> instanceScope = callScope->buildInstanceScope(classTemplate, classTemplate->getName());

        if (!instanceScope){throw MerkError("InstanceScope creation failed in ClassCall::evaluate()");}

        auto capturedScope = instanceScope->getParent();

        if (!capturedScope){throw MerkError("Captured Scope Does Not Exist When Instantiating class: " + classTemplate->getName());}

        auto params = classTemplate->getParameters().clone();
        

        SharedPtr<ClassInstance> instance = makeShared<ClassInstance>(classTemplate, capturedScope, instanceScope);
        
        instance->construct(argValues, instance);
        
        
        DEBUG_FLOW_EXIT();
        auto node = ClassInstanceNode(instance);

        return node;
    }
    
    Node evaluateMethodDef(
        SharedPtr<Scope> passedScope, 
        SharedPtr<Scope> ownScope, 
        SharedPtr<Scope> classScope, 
        String methodName, 
        MethodBody* body, 
        ParamList parameters, 
        CallableType callType, 
        SharedPtr<ClassInstanceNode> instanceNode) {
        
        MARK_UNUSED_MULTI(instanceNode);
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        if (!passedScope){throw MerkError("Provided Scope to MethodDef::evaluate is null");}
        if (!ownScope){throw MerkError("MethodDef::evaluate, scope is null");}
        if (!classScope) {throw MerkError("Class Scope wargValuesas not supplied to Method: " + methodName);}

        auto freeVarNames = body->collectFreeVariables();

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
                throw MerkError("The Following Vars: " + highlight(joinUnorderedSetStrings(tempFreeVars, ", "), Colors::yellow) + "; were defined outside of function defined using function");
            }
        } 
        
        SharedPtr<Scope> defScope = passedScope->buildFunctionDefScope(freeVarNames, methodName);
        if (!defScope){DEBUG_FLOW_EXIT();throw MerkError("Defining Scope for FunctionDef::evaluate is null");}
        if (!defScope){throw MerkError("defScope created in MethodDef::evaluate is null");}

        defScope->owner = generateScopeOwner("MethodDef", methodName); 
        UniquePtr<BaseAST> clonedBodyBase = body->clone();
        auto clonedBody = static_unique_ptr_cast<MethodBody>(std::move(clonedBodyBase));

        // auto nonStaticEls = body->getNonStaticElements();

        // Vector<Chain*> nonStaticElements;
        // if (nonStaticEls.data()) {
        //     nonStaticElements = nonStaticEls;
        // } else {
        //     nonStaticElements = {};
        // }

        // clonedBody->setScope(defScope);
        // clonedBody->setNonStaticElements(nonStaticElements);
        
        if (!clonedBody){throw MerkError("ClonedBody in MethodDef::evaluate is null");}
    
        if (!clonedBody->getScope()){throw MerkError("Scope not present in FunctionDef::evaluate(scope) of clonedBody");}

        DEBUG_LOG(LogLevel::TRACE, "FunctionDef Defining Scope: ", passedScope->getScopeLevel());
        
        SharedPtr<Method> method = makeShared<UserMethod>(methodName, parameters, std::move(clonedBody), ownScope, callType);
        
        if (!method){throw MerkError("Method created in MethodDef::evaluate is null");}
        
        method->setCapturedScope(defScope);
        if (!method->getCapturedScope()) {throw MerkError("No Captured Scope for method: " + methodName);}
        

        auto methodSig = method->toCallableSignature();
        DEBUG_LOG(LogLevel::TRACE, "SETTING METHOD SIG PARAMS FOR ", methodName);

        methodSig->setParameters(parameters.clone());


        if (method->getCallableType() != CallableType::METHOD && method->getSubType() == CallableType::METHOD) {
            DEBUG_LOG(LogLevel::ERROR, "The Types TO Provided To MethodDef::Evaluate -> CallType: ", callableTypeAsString(callType));
            DEBUG_LOG(LogLevel::ERROR, "Method 'get_area_offset' is not being constructed properly with Type: ", callableTypeAsString(method->getCallableType()), "And SubType: ", callableTypeAsString(method->getSubType()));
            throw MerkError("Evaluator:: evaluateMethodDef: See Above Error 1");
        }

        classScope->registerFunction(methodName, methodSig);

        DEBUG_LOG(LogLevel::TRACE, highlight("Registered Method: " + methodName, Colors::yellow), "into: ", classScope.get(), "Owner: ", classScope->owner);

        DEBUG_LOG(LogLevel::TRACE, "Method", methodName, "registered to ", classScope->owner);

        

        if (!method->getBody()){DEBUG_FLOW_EXIT();throw MerkError("Function body is null in FunctionDef::evaluate");}

        // Wrap in a FunctionNode (for returning it as a value)
        MethodNode methodNode(method);
        
        DEBUG_FLOW_EXIT();
        return methodNode;
    }

} // namespace Evaluator
