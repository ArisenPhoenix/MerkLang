#pragma once

// #include "core/types.h"
#include "core/TypesFWD.hpp"
#include "core/EvalResult.hpp"
#include <string>
#include <vector>
#include <optional>
#include "core/Evaluator.hpp"
// This acts as a separate module for evaluation logic - as its name implies
// This is for future considerations when implementing compilation. 
// Also, it keeps other files smaller and more easily sifted...

class ConditionalBlock;  // Forward declaration
class BaseAST;
class ASTStatement;
class ElifStatement;
class ElseStatement;
class CodeBlock;
class IfStatement;
class FunctionBlock;
class ClassInstanceNode;
class CallableBody;
class MethodBody;

namespace FlowEvaluator {
   
    EvalResult evaluateLiteral(Node value);
    EvalResult evaluateVariableDeclaration(String& varName, const ASTStatement* valueNode, DataTypeFlags varMeta, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    EvalResult evaluateVariableAssignment(String name, ASTStatement* value, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    VarNode& evaluateVariableReference(String name, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);

    EvalResult evaluateBinaryOperation(const String& op, const Node& left, const Node& right, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    EvalResult evaluateUnaryOperation(const String& op, const Node& operand, SharedPtr<ClassInstanceNode> instanceNode = nullptr);

    EvalResult evaluateBasicLoop();

    EvalResult evaluateWhileLoop(const ConditionalBlock& condition, const BaseAST* body, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);

    EvalResult evaluateBlock(const Vector<UniquePtr<BaseAST>>& statements, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);

    EvalResult evaluateIf (const IfStatement& ifStatement, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    EvalResult evaluateElif (const ElifStatement& elifStatement, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    EvalResult evaluateElse(const CodeBlock& body, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);


    EvalResult evaluateFunction(Vector<UniquePtr<BaseAST>>& children, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    EvalResult evaluateMethodBody(Vector<UniquePtr<BaseAST>>& children, SharedPtr<Scope> methodScope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    EvalResult evaluateClassBody(SharedPtr<Scope> classCapturedScope, SharedPtr<Scope> classScope, SharedPtr<Scope> generatedScope, String accessor, Vector<UniquePtr<BaseAST>>& children, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    EvalResult evaluateClassCall(SharedPtr<Scope> callScope, String className, ArgResultType argValues, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    EvalResult evaluateMethodDef(SharedPtr<Scope> passedScope, SharedPtr<Scope> ownScope, SharedPtr<Scope> classScope, String methodName, MethodBody* body, ParamList parameters, CallableType callType, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    

    EvalResult evaluateBreak();
    EvalResult evaluateBreak(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);

} // namespace Evaluator
