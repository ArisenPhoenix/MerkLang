
#ifndef METHOD_H
#define METHOD_H

#include "core/scope.h"
#include "ast/ast_function.h"
#include "ast/ast_callable.h"
#include "core/functions/function_node.h"  // Now includes Callable (and Function)


class Chain;

class Method : public Callable {
protected:
    CallableType subType = CallableType::DEF;
    String accessor;
    mutable SharedPtr<Scope> classScope;
    bool isStatic;

public:
    UniquePtr<MethodBody> body;
    // Use the same constructor signature as functions

    Method(String name, ParamList params, UniquePtr<MethodBody> body, SharedPtr<Scope> scope, CallableType callType, bool requiresReturn = false);
    Method(Function&& function);  // For preparation of dynamic updates of methods in a class etc.
    Method(Method& method);
    Node execute(Vector<Node> args, SharedPtr<Scope> callScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

    void setCapturedScope(SharedPtr<Scope> scope) override;
    SharedPtr<Scope> getCapturedScope() const override;

    SharedPtr<CallableSignature> toCallableSignature() override;
    SharedPtr<CallableSignature> toCallableSignature(SharedPtr<Method> method);
    
    String toString() const override;

    String getAccessor();
    void setAccessor(String access);

    void setScope(SharedPtr<Scope> newScope) const override;

    SharedPtr<Scope> getClassScope() const;
    void setClassScope(SharedPtr<Scope> newClassScope);

    MethodBody* getBody();
    MethodBody* getBody() const;
    Vector<Chain*> getNonStaticElements();
    bool getIsStatic();

protected:
    SharedPtr<Scope> capturedScope;
};


class MethodNode : public CallableNode {
public:
    MethodNode(SharedPtr<Method> method);

    MethodNode(SharedPtr<Callable> method);

    SharedPtr<Callable> getCallable() const override;
};
#endif // METHOD_H
