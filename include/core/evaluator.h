#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "core/types.h"
#include "core/node.h"
#include <string>
#include <vector>

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

class Scope;

namespace Evaluator {
   
    Node evaluateLiteral(Node value, bool isString, bool isBool);
    Node evaluateVariableDeclaration(const ASTStatement* valueNode, VarNode name, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    Node evaluateVariableAssignment(String name, ASTStatement* value, SharedPtr<Scope>);
    VarNode& evaluateVariableReference(String name, SharedPtr<Scope> scope);

    Node evaluateBinaryOperation(const String& op, const Node& left, const Node& right, SharedPtr<Scope> scope);
    Node evaluateUnaryOperation(const String& op, const Node& operand);

    Node evaluateBasicLoop();

    Node evaluateWhileLoop(const ConditionalBlock& condition, const BaseAST* body, SharedPtr<Scope> scope);

    Node evaluateBlock(const Vector<UniquePtr<BaseAST>>& statements, SharedPtr<Scope> scope);

    Node evaluateElif (const ElifStatement& elifStatement, SharedPtr<Scope> scope);

    
    Node evaluateIf (const IfStatement& ifStatement, SharedPtr<Scope> scope);



    Node evaluateElse(const CodeBlock& body, SharedPtr<Scope> scope);


    Node evaluateFunction(Vector<UniquePtr<BaseAST>>& children, SharedPtr<Scope> scope);
    Node evaluateMethod(Vector<UniquePtr<BaseAST>>& children, SharedPtr<Scope> methodScope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    Node evaluateClassBody(SharedPtr<Scope> classCapturedScope, SharedPtr<Scope> classScope, SharedPtr<Scope> generatedScope, String accessor, Vector<UniquePtr<BaseAST>>& children);
    Node evaluateClassCall(SharedPtr<Scope> callScope, String className, Vector<Node> argValues);
    Node evaluateMethodDef(SharedPtr<Scope> passedScope, SharedPtr<Scope> ownScope, SharedPtr<Scope> classScope, String methodName, CallableBody* body, ParamList parameters, CallableType callType, CallableType methodType)
;
    // Node evaluateMethod(Vector<UniquePtr<BaseAST>>& children, SharedPtr<Scope> scope);

    [[noreturn]] Node evaluateBreak();
    [[noreturn]] Node evaluateBreak(SharedPtr<Scope> scope);

} // namespace Evaluator

#endif // EVALUATOR_H

