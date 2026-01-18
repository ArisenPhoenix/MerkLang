#pragma once

#include <functional>
#include <memory> 
#include <optional>

// #include "core/types.h"
#include "core/TypesFWD.hpp"
#include "core/callables/Callable.hpp"
#include "core/callables/Invocable.hpp"

class Scope;
class NativeFunction;
class CallableBody;


class Function : public Invocable {

public:
    Function(String name, ParamList params, CallableType functionType, bool requiresReturn = true, bool isStatic = false);
    virtual ~Function() = default;
    virtual Node execute(const ArgumentList args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const = 0;
    virtual SharedPtr<CallableSignature> toCallableSignature() = 0;

    virtual FunctionBody* getThisBody() const = 0;

    virtual UniquePtr<CallableBody> getBody() override = 0;
    virtual CallableBody* getBody() const override = 0;
    virtual CallableBody* getInvocableBody() override = 0;
    virtual void setCapturedScope(SharedPtr<Scope> scope) override = 0;
    virtual void setScope(SharedPtr<Scope> newScope) const override = 0;
    virtual String toString() const override = 0; 


};



    
class UserFunction : public Function {

public:
    mutable UniquePtr<FunctionBody> body;   // The function’s code block


public:
    UserFunction(String name, UniquePtr<FunctionBody> body, ParamList parameters, CallableType funcType);

    Node execute(ArgumentList args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

    SharedPtr<CallableSignature> toCallableSignature() override;
    
    FunctionBody* getThisBody() const override;
    virtual UniquePtr<CallableBody> getBody() override;
    virtual CallableBody* getBody() const override;
    virtual CallableBody* getInvocableBody() override;
    void setScope(SharedPtr<Scope> newScope) const override;
    void setCapturedScope(SharedPtr<Scope> scope) override;
    String toString() const override;

};


class FunctionNode : public CallableNode {
public:
    FunctionNode(SharedPtr<Function> function);

    FunctionNode(SharedPtr<NativeFunction> native);

    FunctionNode(SharedPtr<Callable> function);

    FunctionNode(String originalName, Vector<SharedPtr<CallableSignature>>);

    // Resolve a specific overload from the stored overload-set.
    std::optional<SharedPtr<CallableSignature>> getFunction(String name, const ArgumentList& args, SharedPtr<Scope> scope);

    SharedPtr<Callable> getCallable() const override;

    String toString() const override;
};

