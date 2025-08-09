#ifndef NATIVE_METHOD_H
#define NATIVE_METHOD_H

#include "core/types.h"
#include "core/callables/callable.h" 
#include "core/callables/classes/method.h"


class NativeMethod : public Method {
private:
    std::function<Node(ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self)> methodFn;

public:
    NativeMethod(
        String name,
        ParamList params,
        SharedPtr<Scope> classScope,
        std::function<Node(ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self)> methodFn 
    );

    ~NativeMethod() override;
                
    Node execute(ArgResultType args,
                 SharedPtr<Scope> callScope,
                 SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

    String toString() const override;

    SharedPtr<CallableSignature> toCallableSignature() override;
    void setCapturedScope(SharedPtr<Scope> scope) override;
    SharedPtr<Scope> getCapturedScope() const override;
    void setScope(SharedPtr<Scope> newScope) const override;
    virtual MethodBody* getThisBody();
    virtual UniquePtr<CallableBody> getBody() override;
    virtual CallableBody* getBody() const override;
    virtual CallableBody* getInvocableBody() override;
};

#endif // NATIVE_METHOD_H
