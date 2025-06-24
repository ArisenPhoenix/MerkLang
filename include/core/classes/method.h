
#ifndef METHOD_H
#define METHOD_H

#include "core/scope.h"
#include "ast/ast_function.h"
#include "ast/ast_callable.h"
#include "core/functions/function_node.h"  // Now includes Callable (and Function)


class Chain;


class Method : public Callable {
    String accessor;
    bool isStatic = false;
    SharedPtr<Scope> classScope;
public:
    Method(String name, ParamList params, CallableType methodType, bool requiresReturn = false);
    bool requiresReturn = false;
    virtual ~Method();

    virtual Node execute(Vector<Node> args, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> self) const = 0;

    virtual SharedPtr<CallableSignature> toCallableSignature() = 0;
    virtual MethodBody* getBody() const { return nullptr; }

    virtual void setCapturedScope(SharedPtr<Scope> scope) = 0;
    virtual SharedPtr<Scope> getCapturedScope() const = 0;

    virtual void setScope(SharedPtr<Scope> newScope) const override = 0;
    virtual String toString() const = 0;

    void setAccessor(String access);
    String getAccessor();

    SharedPtr<Scope> getClassScope() const;
    void setClassScope(SharedPtr<Scope> newClassScope);

    bool getIsStatic();

protected:
    SharedPtr<Scope> capturedScope;
};


class UserMethod : public Method {
public:
    UniquePtr<MethodBody> body;
    UserMethod(String name, ParamList params, UniquePtr<MethodBody> body, SharedPtr<Scope> scope, CallableType callType, bool requiresReturn = false);
    UserMethod(Function&& function);  // For preparation of dynamic updates of methods in a class etc.
    UserMethod(UserMethod& method);
    virtual Node execute(Vector<Node> args, SharedPtr<Scope> callScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

    void setCapturedScope(SharedPtr<Scope> scope) override;
    SharedPtr<Scope> getCapturedScope() const override;

    virtual SharedPtr<CallableSignature> toCallableSignature() override;
    virtual SharedPtr<CallableSignature> toCallableSignature(SharedPtr<UserMethod> method);
    
    String toString() const override;

    void setScope(SharedPtr<Scope> newScope) const override;

    MethodBody* getBody();
    MethodBody* getBody() const;
    Vector<Chain*> getNonStaticElements();

};


class MethodNode : public CallableNode {
public:
    MethodNode(SharedPtr<Method> method);

    MethodNode(SharedPtr<Callable> method);

    SharedPtr<Callable> getCallable() const override;
};
#endif // METHOD_H
