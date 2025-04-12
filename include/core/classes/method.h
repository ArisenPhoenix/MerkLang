
#ifndef METHOD_H
#define METHOD_H

#include "core/scope.h"
#include "ast/ast_function.h"
#include "ast/ast_callable.h"
#include "core/functions/function_node.h"  // Now includes Callable (and Function)


class Method : public Callable {
protected:
    CallableType subType = CallableType::DEF;
    String accessor;

public:
    UniquePtr<MethodBody> body;
    // Use the same constructor signature as functions

    Method(String name, ParamList params, UniquePtr<MethodBody> body, SharedPtr<Scope> scope, bool requiresReturn = false);
    Method(Function&& function);  // For preparation of dynamic updates of methods in a class etc.
    // Method(Function&& function);
    Method(Method& method);

    // Method(String name, ParamList params, UniquePtr<MethodBody> body, SharedPtr<Scope> scope, String accessor, bool requiresReturn = false);
    // Execution will be similar but checks for the accessor will be needed
    Node execute(Vector<Node> args, SharedPtr<Scope> callScope) const override;

    void setCapturedScope(SharedPtr<Scope> scope) override;
    SharedPtr<Scope> getCapturedScope() const override;

    SharedPtr<CallableSignature> toCallableSignature() override;
    String toString() const override;

    String getAccessor();
    void setAccessor(String access);

protected:
    SharedPtr<Scope> capturedScope;
};


class MethodNode : public CallableNode {
public:
    MethodNode(SharedPtr<Method> method) : CallableNode(method, "Method") {
        data.type = NodeValueType::Method;
    }

    MethodNode(SharedPtr<Callable> method) : CallableNode(method, "Method") {
        data.type = NodeValueType::Method;
    }




    SharedPtr<Callable> getCallable() const override {
        return std::get<SharedPtr<Method>>(data.value);
    }
};
#endif // METHOD_H
