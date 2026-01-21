#pragma once

#include "core/TypesFWD.hpp"
#include "utilities/debugger.h"

#include "ast/AstBase.hpp"
#include "ast/Ast.hpp"

#include "ast/AstClass.hpp"
#include "ast/AstControl.hpp"
#include "ast/AstFunction.hpp"

#include "core/errors.h" 

class Chain;
class FunctionDef;

class MethodBody : public CallableBody {
public:
    MethodBody(SharedPtr<Scope> scope);
    MethodBody(UniquePtr<CodeBlock>&& body);
    MethodBody(UniquePtr<CallableBody>* body);
    ~MethodBody() override;

    Vector<UniquePtr<BaseAST>>& getChildren(){return children;};
    UniquePtr<BaseAST> clone() const override;
    Node evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

    AstType getAstType() const override { return AstType::ClassMethodBlock;}
    EvalResult evaluateFlow(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

};



class MethodDef : public CallableDef {
protected:
    String accessor;
    CallableType InvocableType = CallableType::DEF;
    SharedPtr<Scope> classScope;
    Vector<Chain*> nonStaticElements;
public:
    MethodDef(String name, ParamList parameters, UniquePtr<MethodBody> body, CallableType InvocableType, SharedPtr<Scope> scope);
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
    // SharedPtr<Scope>& getClassScope() const;
    bool isConstructor() const;
    void setNonStaticElements(Vector<Chain*> nonStaticEls);
    Vector<Chain*> getNonStaticElements();

};

class MethodCall : public CallableCall {
public:
    MethodCall(String methodName, UniquePtr<ArgumentType> args, SharedPtr<Scope> scope);
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
