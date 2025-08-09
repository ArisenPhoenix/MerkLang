#ifndef EVALUATOR_H
#define EVALUATOR_H

// #include "core/node/node.h"
#include "core/types.h"
#include <string>
#include <vector>
#include <optional>

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

class Scope;

namespace Evaluator {
   
    Node evaluateLiteral(Node value);
    Node evaluateVariableDeclaration(String& varName, const ASTStatement* valueNode, DataTypeFlags varMeta, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    Node evaluateVariableAssignment(String name, ASTStatement* value, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    VarNode& evaluateVariableReference(String name, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);

    Node evaluateBinaryOperation(const String& op, const Node& left, const Node& right, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    Node evaluateUnaryOperation(const String& op, const Node& operand, SharedPtr<ClassInstanceNode> instanceNode = nullptr);

    Node evaluateBasicLoop();

    Node evaluateWhileLoop(const ConditionalBlock& condition, const BaseAST* body, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);

    Node evaluateBlock(const Vector<UniquePtr<BaseAST>>& statements, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);

    Node evaluateIf (const IfStatement& ifStatement, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    Node evaluateElif (const ElifStatement& elifStatement, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    Node evaluateElse(const CodeBlock& body, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);


    Node evaluateFunction(Vector<UniquePtr<BaseAST>>& children, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    Node evaluateMethodBody(Vector<UniquePtr<BaseAST>>& children, SharedPtr<Scope> methodScope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    Node evaluateClassBody(SharedPtr<Scope> classCapturedScope, SharedPtr<Scope> classScope, SharedPtr<Scope> generatedScope, String accessor, Vector<UniquePtr<BaseAST>>& children, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    Node evaluateClassCall(SharedPtr<Scope> callScope, String className, ArgResultType argValues, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    Node evaluateMethodDef(SharedPtr<Scope> passedScope, SharedPtr<Scope> ownScope, SharedPtr<Scope> classScope, String methodName, MethodBody* body, ParamList parameters, CallableType callType, SharedPtr<ClassInstanceNode> instanceNode = nullptr);

    [[noreturn]] Node evaluateBreak();
    [[noreturn]] Node evaluateBreak(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);

} // namespace Evaluator

#endif // EVALUATOR_H

