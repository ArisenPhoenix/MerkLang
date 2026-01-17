#pragma once

// #include "core/types.h"
#include "core/TypesFWD.hpp"
#include "core/Scope.hpp"
#include "ast/AstFunction.hpp" 
#include "ast/AstCallable.hpp"
#include "core/callables/Invocable.hpp"
#include "ast/AstMethod.hpp"

class Chain;
class NativeMethod;
class CallableBody;


class Method : public Invocable {
    String accessor;
    SharedPtr<Scope> classScope;
    
public:
    Method(String name, ParamList params, CallableType methodType, bool requiresReturn = false, bool isStatic = false);
    void setAccessor(String access);
    String getAccessor();

    void setClassScope(SharedPtr<Scope> newClassScope);
    SharedPtr<Scope> getClassScope() const;

    virtual ~Method();
    virtual Node execute(ArgumentList args, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> self) const = 0;
    virtual SharedPtr<CallableSignature> toCallableSignature() = 0;

    
    virtual MethodBody* getThisBody() = 0;
    virtual UniquePtr<CallableBody> getBody() override = 0;
    virtual CallableBody* getBody() const override = 0;
    virtual CallableBody* getInvocableBody() override = 0;

    virtual void setScope(SharedPtr<Scope> newScope) const override = 0;
    virtual String toString() const override = 0; 
    
};

class UserMethod : public Method {
protected:
    mutable UniquePtr<MethodBody> body;

public:
    
    UserMethod(String name, ParamList params, UniquePtr<MethodBody> body, SharedPtr<Scope> scope, CallableType callType);
    UserMethod(Function&& function);  // For preparation of dynamic updates of methods in a class etc.
    UserMethod(UserMethod& method);
    
    

    CallableBody* getInvocableBody() override;

    MethodBody* getThisBody() override;
    
    UniquePtr<CallableBody> getBody() override;
    CallableBody* getBody() const override;



    Vector<Chain*> getNonStaticElements();

    Node execute(ArgumentList args, SharedPtr<Scope> callScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;
    SharedPtr<CallableSignature> toCallableSignature() override;
    SharedPtr<CallableSignature> toCallableSignature(SharedPtr<UserMethod> method);

    String toString() const override;
    void setScope(SharedPtr<Scope> newScope) const override;
};


class MethodNode : public CallableNode {
public:
    ~MethodNode();
    MethodNode(SharedPtr<Method> method);

    MethodNode(SharedPtr<Callable> method);

    MethodNode(SharedPtr<NativeMethod> method);

    SharedPtr<Callable> getCallable() const override;
};

