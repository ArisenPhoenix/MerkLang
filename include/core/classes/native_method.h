#ifndef NATIVE_METHOD_H
#define NATIVE_METHOD_H

#include "core/types.h"
#include "core/functions/callable.h"
#include "core/classes/method.h"
#include "core/functions/param_node.h"


// class NativeFunction : public Function {
// private:
//     std::function<Node(Vector<Node>, SharedPtr<Scope>)> nativeImpl;
//     SharedPtr<Scope> capturedScope; // For consistency with UserFunction

// public:
//     NativeFunction(
//         String name,
//         ParamList params,
//         std::function<Node(Vector<Node>, SharedPtr<Scope>)> impl
//     );

//     Node execute(Vector<Node> args, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

//     SharedPtr<CallableSignature> toCallableSignature() override;

//     void setCapturedScope(SharedPtr<Scope> scope) override;
//     SharedPtr<Scope> getCapturedScope() const override;

//     String toString() const override;
//     void setScope(SharedPtr<Scope> newScope) const override;

//     FunctionBody* getBody() const override { return nullptr; } // Native function has no body
// };


class NativeMethod : public Method {
private:
    std::function<Node(NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self)> methodFn;
    // SharedPtr<Scope> classScope = nullptr;

public:
    NativeMethod(
        String name,
        ParamList params,
        SharedPtr<Scope> classScope,
        std::function<Node(NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self)> methodFn 
    );

    ~NativeMethod() override;
                
    Node execute(Vector<Node> args,
                 SharedPtr<Scope> callScope,
                 SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

    String toString() const override;

    SharedPtr<CallableSignature> toCallableSignature() override;
    // SharedPtr<CallableSignature> toCallableSignature(SharedPtr<Method> method);

    void setCapturedScope(SharedPtr<Scope> scope) override;
    SharedPtr<Scope> getCapturedScope() const override;
    void setScope(SharedPtr<Scope> newScope) const override;
};

#endif // NATIVE_METHOD_H
