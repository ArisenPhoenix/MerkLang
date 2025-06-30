#ifndef AST_METHOD_H
#define AST_METHOD_H

#include "core/types.h"               // For String, Vector, UniquePtr, SharedPtr, NodeValueType, etc.
#include "utilities/debugger.h"       // For DEBUG_LOG

#include "core/node.h"
#include "ast/ast_base.h"             // For ASTStatement, printIndent, etc.
#include "ast/ast.h"
#include "ast/ast_control.h"
#include "ast/ast_function.h"
#include "core/callables/param_node.h"

#include "core/errors.h"              // For error types

class Chain;
class FunctionDef;

class MethodBody : public CallableBody {
private:
    Vector<Chain*> nonStaticElements;

public:
    MethodBody(SharedPtr<Scope> scope);
    MethodBody(UniquePtr<CodeBlock>&& body);
    MethodBody(UniquePtr<CallableBody>* body);
    ~MethodBody() override;

    Vector<UniquePtr<BaseAST>>& getChildren(){return children;};
    UniquePtr<BaseAST> clone() const override;

    // virtual Node evaluate(SharedPtr<Scope> scope,  SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;
    // virtual UniquePtr<BaseAST> clone() const override;
    Node evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

    // void setNonStaticElements(Vector<Chain*> nonStaticEls);
    AstType getAstType() const override { return AstType::ClassMethodBlock;}
    // bool getIsStatic();
    // Vector<Chain*> getNonStaticElements();
    // UniquePtr<BaseAST> clone() const override;
};



class MethodDef : public CallableDef {
protected:
    String accessor;
    CallableType methodType = CallableType::DEF;
    SharedPtr<Scope> classScope;
    Vector<Chain*> nonStaticElements;
public:
    // Constructor: Force the FunctionType to METHOD.
    MethodDef(String name, ParamList parameters, UniquePtr<MethodBody> body, CallableType methodType, SharedPtr<Scope> scope);
    MethodDef(UniquePtr<FunctionDef> funcDef);
    ~MethodDef();

    UniquePtr<BaseAST> clone() const override;

    String toString() const override;
    AstType getAstType() const override { return AstType::ClassMethodDef;}
    void setMethodAccessor(String& accessorName);
    String getMethodAccessor() const;
    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;
    ParamList& getParameters();
    const ParamList& getParameters() const;   // For inspection

    void setClassScope(SharedPtr<Scope> scope);
    SharedPtr<Scope> getClassScope() const;
    bool isConstructor() const;
    void setNonStaticElements(Vector<Chain*> nonStaticEls);
    Vector<Chain*> getNonStaticElements();

};

class MethodCall : public CallableCall {
public:
    MethodCall(String methodName, Vector<UniquePtr<ASTStatement>> args, SharedPtr<Scope> scope);
    ~MethodCall();
    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;
    AstType getAstType() const override { return AstType::ClassMethodCall;}
    UniquePtr<BaseAST> clone() const override;
};

    
class MethodRef : public CallableRef {
protected:
    String accessor;
public:
    MethodRef(String name, SharedPtr<Scope> scope);

    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

    AstType getAstType() const override {return AstType::ClassMethodRef;}
};









#endif //AST_METHOD_H