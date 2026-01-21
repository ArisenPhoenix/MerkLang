// FlowEvaluator.cpp  (or replace Evaluator.cpp if you want)

#include <unordered_map>
#include <unordered_set>
#include <sstream>

#include "core/node/Node.hpp"
#include "core/node/ArgumentNode.hpp"
#include "core/node/NodeStructures.hpp"

#include "core/types.h"
#include "core/errors.h"
#include "core/Scope.hpp"

// New flow types
#include "core/evaluators/EvalResult.hpp"
#include "core/evaluators/FlowEvaluator.hpp"   // your new header (or core/Evaluator.h if you keep name)

#include "utilities/helper_functions.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"

#include "ast/AstBase.hpp"
#include "ast/Ast.hpp"
#include "ast/AstControl.hpp"
#include "ast/AstChain.hpp"
#include "ast/AstMethod.hpp"
#include "ast/AstClass.hpp"
#include "ast/ast_validate.h"

#include "ast/helpers.h"
#include "core/callables/functions/Function.hpp"
#include "core/callables/classes/Method.hpp"


// namespace {

// // Keep this if you still use it elsewhere
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
//         {"<",  Operator::LessThan},
//         {">",  Operator::GreaterThan},
//         {"<=", Operator::LessThanOrEquals},
//         {">=", Operator::GreaterThanOrEquals},
//         {"+",  Operator::Plus},
//         {"-",  Operator::Minus},
//         {"*",  Operator::Multiply},
//         {"/",  Operator::Divide},
//         {"%",  Operator::Modulo}
//     };

//     auto it = operatorMap.find(op);
//     return (it != operatorMap.end()) ? it->second : Operator::Invalid;
// }

// bool isDebug = Debugger::getInstance().getLogLevel() == LogLevel::DEBUG;

// void evaluatingFor(const String& value, const String& methodName, int scopeLevel = -2) {
//     MARK_UNUSED_MULTI(value, methodName);
//     if (!isDebug) return;
//     String level = (scopeLevel > -2) ? std::to_string(scopeLevel) : "[NOT USED]";
//     DEBUG_LOG(LogLevel::INFO, "DEBUG Evaluator: Evaluating ", methodName, " for ", value, ", scopeLevel: ", level);
// }

// void evaluatingFor(const Node& value, const String& methodName, int scopeLevel = -2) {
//     MARK_UNUSED_MULTI(value, methodName, scopeLevel);
//     if (!isDebug) return;
//     String level = (scopeLevel > -2) ? std::to_string(scopeLevel) : "[NOT USED]";
//     DEBUG_LOG(LogLevel::INFO, "DEBUG Evaluator: Evaluating ", methodName, " for ", value, ", scopeLevel: ", level);
// }

// } // namespace

namespace FlowEvaluator {

EvalResult evaluateLiteral(Node value) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!value.isValid()) throw MerkError("Literal Value is Not Valid");
    if (value.isInstance()) throw MerkError("evaluateLiteral result isInstance");
    DEBUG_FLOW_EXIT();
    return EvalResult::Normal(std::move(value));
}

EvalResult evaluateVariableDeclaration(String& name,
                                      const ASTStatement* valueNode,
                                      DataTypeFlags varMeta,
                                      SharedPtr<Scope> scope,
                                      SharedPtr<ClassInstanceNode> instanceNode)
{
    MARK_UNUSED_MULTI(instanceNode);
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!scope) throw MerkError("evaluateVariableDeclaration: scope is null");
    if (!valueNode) throw MerkError("evaluateVariableDeclaration: valueNode is null");

    SharedPtr<Scope> instanceScope = instanceNode ? instanceNode->getInstanceScope() : scope;
    bool usingInstanceScope = (instanceScope != scope);

    String varName = name;
    if (varName.empty()) throw MerkError("VarName is empty in evaluateVariableDeclaration");
    auto& tr = scope->localTypes;
    varMeta.declaredSig = tr.bindResolvedType(varMeta.fullType, *scope);
    Node resolved = valueNode->evaluate(scope, instanceNode);
    auto finalVar = makeUnique<VarNode>(resolved, varMeta);
    auto returnVal =  finalVar->getValueNode();
    // Same rule you had: if assigning to a member name, go to instance scope
    if (usingInstanceScope && instanceScope && instanceScope->hasMember(varName)) {
        instanceScope->declareVariable(varName, std::move(finalVar));
    } else {
        scope->declareVariable(varName, std::move(finalVar));
    }

    DEBUG_FLOW_EXIT();
    return EvalResult::Normal(returnVal);
}

EvalResult evaluateVariableAssignment(String name,
                                     ASTStatement* value,
                                     SharedPtr<Scope> scope,
                                     SharedPtr<ClassInstanceNode> instanceNode)
{
    MARK_UNUSED_MULTI(instanceNode);
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!scope) throw MerkError("evaluateVariableAssignment: scope is null");
    if (!value) throw MerkError("evaluateVariableAssignment: value node is null");

    Node finalVal = value->evaluate(scope, instanceNode);
    if (finalVal.toString() == "var") throw MerkError("finalVal is var in evaluateVariableAssignment");

    DEBUG_LOG(LogLevel::TRACE, "========================");
    DEBUG_LOG(LogLevel::TRACE, "Assigning: ", finalVal.toString() + " META: " + finalVal.getFlags().toString());
    DEBUG_LOG(LogLevel::TRACE, "========================");

    scope->updateVariable(name, finalVal);

    DEBUG_FLOW_EXIT();
    return EvalResult::Normal(std::move(finalVal));
}

VarNode& evaluateVariableReference(String name,
                                  SharedPtr<Scope> scope,
                                  SharedPtr<ClassInstanceNode> instanceNode)
{
    MARK_UNUSED_MULTI(instanceNode);
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!scope) throw MerkError("evaluateVariableReference: scope is null");

    // Keep your current semantics: look up from provided scope
    // (If you later want instanceScope precedence, do it in the AST node for member access)
    auto& variable = scope->getVariable(name);

    DEBUG_FLOW_EXIT();
    return variable;
}

EvalResult evaluateBinaryOperation(const String& op,
                                  const Node& leftValue,
                                  const Node& rightValue,
                                  SharedPtr<Scope> scope,
                                  SharedPtr<ClassInstanceNode> instanceNode)
{
    MARK_UNUSED_MULTI(instanceNode);
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!scope) throw MerkError("evaluateBinaryOperation: scope is null");

    // evaluatingFor(leftValue, "evaluateBinaryOperation", scope->getScopeLevel());
    // evaluatingFor(rightValue, "evaluateBinaryOperation", scope->getScopeLevel());

    Node val;

    if (op == "+") val = leftValue + rightValue;
    else if (op == "-") val = leftValue - rightValue;
    else if (op == "*") val = leftValue * rightValue;
    else if (op == "/") val = leftValue / rightValue;
    else if (op == "%") val = leftValue % rightValue;

    else if (op == "and" || op == "&&") val = Node(leftValue.isTruthy() && rightValue.isTruthy());
    else if (op == "or"  || op == "||") val = Node(leftValue.isTruthy() || rightValue.isTruthy());

    else if (op == "+=") val = (Node(leftValue) += rightValue);
    else if (op == "-=") val = (Node(leftValue) -= rightValue);
    else if (op == "*=") val = (Node(leftValue) *= rightValue);
    else if (op == "/=") val = (Node(leftValue) /= rightValue);
    else if (op == "++") val = (Node(leftValue) += Node(1));

    else if (op == "==") val = Node(leftValue == rightValue);
    else if (op == "!=") val = Node(leftValue != rightValue);
    else if (op == "<")  val = Node(leftValue <  rightValue);
    else if (op == "<=") val = Node(leftValue <= rightValue);
    else if (op == ">")  val = Node(leftValue >  rightValue);
    else if (op == ">=") val = Node(leftValue >= rightValue);

    else throw MerkError("Unsupported operator: " + op);

    DEBUG_FLOW_EXIT();
    return EvalResult::Normal(std::move(val));
}

EvalResult evaluateUnaryOperation(const String& op,
                                 const Node& operand,
                                 SharedPtr<ClassInstanceNode> instanceNode)
{
    MARK_UNUSED_MULTI(instanceNode);
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    Node val;
    if (op == "!" || op == "not") {
        val = Node(!operand.toBool());
        DEBUG_FLOW_EXIT();
        return EvalResult::Normal(std::move(val));
    }

    if (op == "?") {
        val = Node(!operand.isNull());
        DEBUG_FLOW_EXIT();
        return EvalResult::Normal(std::move(val));
    }

    if (op == "-") {
        val = operand.negate();
        DEBUG_FLOW_EXIT();
        return EvalResult::Normal(std::move(val));
    }

    DEBUG_FLOW_EXIT();
    throw MerkError("Unsupported unary operator: " + op);
}

EvalResult evaluateBasicLoop() {
    DEBUG_FLOW(FlowLevel::LOW);
    DEBUG_FLOW_EXIT();
    throw MerkError("LoopNode is a base class and cannot be evaluated directly.");
}

EvalResult evaluateBlock(const Vector<UniquePtr<BaseAST>>& statements,
                         SharedPtr<Scope> scope,
                         SharedPtr<ClassInstanceNode> instanceNode)
{
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!scope) throw MerkError("evaluateBlock: scope is null");

    Node lastValue;

    for (const auto& child : statements) {
        if (!child) throw MerkError("Null child node in evaluateBlock");

        EvalResult r = child->evaluateFlow(scope, instanceNode);

        // bubble Return/Break/Continue/Throw
        if (r.isControl()) {
            DEBUG_FLOW_EXIT();
            return r;
        }

        lastValue = std::move(r.value);
    }

    DEBUG_FLOW_EXIT();
    return EvalResult::Normal(std::move(lastValue));
}

EvalResult evaluateIf(const IfStatement& ifStatement,
                      SharedPtr<Scope> scope,
                      SharedPtr<ClassInstanceNode> instanceNode)
{
    MARK_UNUSED_MULTI(instanceNode);
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!scope) throw MerkError("evaluateIf: scope is null");

    // NOTE: condition is expression eval (Node). Body is flow eval.
    if (ifStatement.getCondition()->evaluate(scope, instanceNode).isTruthy()) {
        EvalResult r = ifStatement.getBody()->evaluateFlow(scope, instanceNode);
        DEBUG_FLOW_EXIT();
        return r;
    }

    for (const auto& elif : ifStatement.getElifs()) {
        // Your ElifStatement::evaluate() returns truthy Node currently.
        Node cond = elif->evaluate(scope, instanceNode);
        if (cond.isTruthy()) {
            EvalResult r = elif->getBody()->evaluateFlow(scope, instanceNode);
            DEBUG_FLOW_EXIT();
            return r;
        }
    }

    if (ifStatement.getElse()) {
        EvalResult r = ifStatement.getElse()->evaluateFlow(scope, instanceNode);
        DEBUG_FLOW_EXIT();
        return r;
    }

    DEBUG_FLOW_EXIT();
    return EvalResult::Normal(Node());
}

EvalResult evaluateElif(const ElifStatement& elifStatement,
                        SharedPtr<Scope> scope,
                        SharedPtr<ClassInstanceNode> instanceNode)
{
    MARK_UNUSED_MULTI(instanceNode);
    DEBUG_FLOW(FlowLevel::LOW);
    if (!scope) throw MerkError("evaluateElif: scope is null");

    if (!elifStatement.getCondition()) {
        DEBUG_FLOW_EXIT();
        throw MerkError("ElifStatement missing condition");
    }

    Node v = Node(elifStatement.getCondition()->evaluate(scope, instanceNode).isTruthy());
    DEBUG_FLOW_EXIT();
    return EvalResult::Normal(std::move(v));
}

EvalResult evaluateElse(const CodeBlock& body, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) {
    MARK_UNUSED_MULTI(instanceNode);
    DEBUG_FLOW(FlowLevel::LOW);
    if (!scope) throw MerkError("evaluateElse: scope is null");

    // You previously created child scope here. Keep if you want else-is-block scoping.
    auto child = scope->createChildScope();
    EvalResult r = body.evaluateFlow(child, instanceNode);

    DEBUG_FLOW_EXIT();
    return r;
}


EvalResult evaluateWhileLoop(const ConditionalBlock& condition, const BaseAST* body, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) {
    DEBUG_FLOW(FlowLevel::LOW);
    if (!scope) throw MerkError("evaluateWhileLoop: scope is null");
    if (!body)  throw MerkError("WhileLoop has no body");

    while (true) {
        Node cond = condition.evaluate(scope, instanceNode);
        if (!cond.toBool()) break;

        EvalResult r = body->evaluateFlow(scope, instanceNode);

        if (r.isContinue()) continue;
        if (r.isBreak())    break;

        if (r.isControl()) { // Return or Throw bubble
            DEBUG_FLOW_EXIT();
            return r;
        }
    }

    DEBUG_FLOW_EXIT();
    return EvalResult::Normal(Node());
}

// These are now pure factories (NOT noreturn anymore).
// You can keep noreturn in header if you want, but it becomes false once flow is used.
EvalResult evaluateBreak() {
    return EvalResult::Break();
}

EvalResult evaluateBreak(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) {
    MARK_UNUSED_MULTI(scope, instanceNode);
    return EvalResult::Break();
}

// Optional: keep these if you still have legacy callers.
// Otherwise delete from FlowEvaluator.hpp.
EvalResult evaluateFunction(Vector<UniquePtr<BaseAST>>& children,
                            SharedPtr<Scope> scope,
                            SharedPtr<ClassInstanceNode> instanceNode)
{
    DEBUG_FLOW(FlowLevel::NONE);
    if (!scope) throw MerkError("evaluateFunction: scope is null");

    // Functions should evaluate in call scope, but use instance scope if provided (your old rule)
    auto instanceScope = instanceNode ? instanceNode->getInstanceScope() : scope;

    // IMPORTANT: Use flow evaluation, not evaluate()
    EvalResult r = evaluateBlock(children, instanceScope, instanceNode);
    DEBUG_FLOW_EXIT();
    return r;
}

EvalResult evaluateMethodBody(Vector<UniquePtr<BaseAST>>& children,
                              SharedPtr<Scope> methodScope,
                              SharedPtr<ClassInstanceNode> instanceNode)
{
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!methodScope) throw MerkError("evaluateMethodBody: methodScope is null");
    if (!instanceNode) throw MerkError("evaluateMethodBody has no instanceNode");

    // Same as function: flow block
    EvalResult r = evaluateBlock(children, methodScope, instanceNode);
    DEBUG_FLOW_EXIT();
    return r;
}

// ClassBody / ClassCall / MethodDef can remain Node-based for now if they never need Return/Break/Continue.
// But since your header wants flow, we wrap Node() into Normal(Node()).

EvalResult evaluateClassBody(SharedPtr<Scope> classCapturedScope,
                            SharedPtr<Scope> classScope,
                            SharedPtr<Scope> generatedScope,
                            String accessor,
                            Vector<UniquePtr<BaseAST>>& children,
                            SharedPtr<ClassInstanceNode> instanceNode)
{
    MARK_UNUSED_MULTI(classCapturedScope, instanceNode);
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    if (!classCapturedScope) throw MerkError("Class Captured Scope Was Not Set On Body");
    if (!classScope) throw MerkError("Class Scope Was Not Set On Body");
    if (!generatedScope) throw MerkError("generatedScope is null in evaluateClassBody");
    if (accessor.empty()) throw MerkError("Accessor Was Not Set On Body");

    generatedScope->owner = generateScopeOwner("Class", "Body");

    for (const auto& child : children) {
        if (!child) throw MerkError("Null child in class body");

        switch (child->getAstType()) {
            case AstType::VariableDeclaration:
                child->evaluate(classScope->getParent(), instanceNode);
                break;

            case AstType::ClassMethodDef: {
                auto* methodDef = static_cast<MethodDef*>(child.get());
                auto methodScope = classScope->createChildScope();
                if (!methodScope) throw MerkError("generated methodScope is null in ClassDef::evaluate");

                methodDef->setClassScope(classScope);

                applyAccessorScopeFix(methodDef, classScope, accessor);
                stripImplicitAccessor(methodDef, accessor);

                methodDef->evaluate(classScope, instanceNode);
                break;
            }

            default:
                // TODO: handle nested class definitions
                DEBUG_LOG(LogLevel::ERROR, "Unhandled AST type in ClassBody:", child->getAstTypeAsString());
                throw MerkError("Unexpected AST statement in ClassBody: " + child->getAstTypeAsString());
        }
    }

    DEBUG_FLOW_EXIT();
    return EvalResult::Normal(Node());
}

EvalResult evaluateClassCall(SharedPtr<Scope> callScope,
                            String className,
                            ArgumentList argValues,
                            SharedPtr<ClassInstanceNode> instanceNode)
{
    MARK_UNUSED_MULTI(instanceNode);
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!callScope) throw MerkError("evaluateClassCall: callScope is null");

    auto classOpt = callScope->getClass(className);
    if (!classOpt.has_value()) throw MerkError("Class not found: " + className);

    auto classSig = classOpt.value();
    if (!classSig) throw MerkError("Class signature is null for class: " + className);

    auto classTemplate = classSig->getClassDef();
    SharedPtr<Scope> instanceScope = callScope->buildInstanceScope(classTemplate, classTemplate->getName());
    if (!instanceScope) throw MerkError("InstanceScope creation failed in ClassCall::evaluate()");

    auto capturedScope = instanceScope->getParent();
    if (!capturedScope) throw MerkError("Captured scope missing when instantiating class: " + classTemplate->getName());

    SharedPtr<ClassInstance> instance = makeShared<ClassInstance>(classTemplate, capturedScope, instanceScope);
    instance->construct(argValues, instance);

    DEBUG_FLOW_EXIT();
    Node inst = ClassInstanceNode(instance);
    inst.getFlags().isInstance = true;
    inst.getFlags().isCallable = true;                  
    inst.getFlags().type = NodeValueType::ClassInstance;
    inst.getFlags().fullType.setBaseType(className);
    inst.getFlags().inferredSig = callScope->localTypes.classType(className);

    return EvalResult::Normal(std::move(inst));
}

EvalResult evaluateMethodDef(SharedPtr<Scope> passedScope,
                            SharedPtr<Scope> ownScope,
                            SharedPtr<Scope> classScope,
                            String methodName,
                            MethodBody* body,
                            ParamList parameters,
                            CallableType callType,
                            SharedPtr<ClassInstanceNode> instanceNode)
{
    MARK_UNUSED_MULTI(instanceNode);
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    if (!passedScope) throw MerkError("Provided Scope to MethodDef::evaluate is null");
    if (!ownScope) throw MerkError("MethodDef::evaluate: ownScope is null");
    if (!classScope) throw MerkError("MethodDef::evaluate: classScope is null");
    if (!body) throw MerkError("MethodDef::evaluate: body is null");

    auto freeVarNames = body->collectFreeVariables();

    if (callType == CallableType::FUNCTION) {
        FreeVars temp = freeVarNames;
        for (auto& param : parameters) {
            auto it = temp.find(param.getName());
            if (it != temp.end()) temp.erase(it);
        }
        if (!temp.empty()) {
            throw MerkError("Vars defined outside function: " +
                            highlight(joinUnorderedSetStrings(temp, ", "), Colors::yellow));
        }
    }

    SharedPtr<Scope> defScope = passedScope->buildFunctionDefScope(freeVarNames, methodName);
    if (!defScope) { throw MerkError("Def scope is null for MethodDef: " + methodName); }

    defScope->owner = generateScopeOwner("MethodDef", methodName);

    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    auto clonedBody = static_unique_ptr_cast<MethodBody>(std::move(clonedBodyBase));
    if (!clonedBody) { throw MerkError("Cloned body is null for MethodDef: " + methodName); }

    SharedPtr<Method> method = makeShared<UserMethod>(methodName, parameters, std::move(clonedBody), ownScope, callType);
    if (!method) { throw MerkError("Method created is null: " + methodName); }

    method->setCapturedScope(defScope);
    if (!method->getCapturedScope()) { throw MerkError("No captured scope for method: " + methodName); }

    auto methodSig = method->toCallableSignature();
    methodSig->setParameters(parameters.clone());

    classScope->registerFunction(methodName, methodSig);

    DEBUG_FLOW_EXIT();
    return EvalResult::Normal(MethodNode(method));
}

} // namespace Evaluator
