#pragma once

#include <functional>
// #include "core/types.h"
#include "core/TypesFWD.hpp"
#include "core/callables/functions/Function.hpp"


class NativeFunction : public Function {
private:
    std::function<Node(ArgResultType, SharedPtr<Scope>, SharedPtr<ClassInstanceNode>)> nativeImpl;
    SharedPtr<Scope> capturedScope; // For consistency with UserFunction

public:
    NativeFunction(
        String name,
        ParamList params,
        std::function<Node(ArgResultType, SharedPtr<Scope>, SharedPtr<ClassInstanceNode>)> impl

    );

    Node execute(ArgResultType args, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

    SharedPtr<CallableSignature> toCallableSignature() override;

    void setCapturedScope(SharedPtr<Scope> scope) override;
    SharedPtr<Scope> getCapturedScope() const override;

    String toString() const override;
    void setScope(SharedPtr<Scope> newScope) const override;

    FunctionBody* getThisBody() const override {return nullptr;};
    virtual UniquePtr<CallableBody> getBody() {return nullptr;};
    virtual CallableBody* getBody() const {return nullptr;};
    virtual CallableBody* getInvocableBody() {return nullptr;};
};
