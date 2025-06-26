// #ifndef NATIVE_FUNCTION_H
// #define NATIVE_FUNCTION_H

// #include <functional>
// #include "core/types.h"
// #include "core/callables/functions/function.h"


// class NativeFunction : public Function {
// private:
//     // std::function<Node(Vector<Node>, SharedPtr<Scope>)> nativeImpl;
//     std::function<Node(Vector<Node>, SharedPtr<Scope>, SharedPtr<ClassInstanceNode>)> nativeImpl;

//     SharedPtr<Scope> capturedScope; // For consistency with UserFunction

// public:
//     NativeFunction(
//         String name,
//         ParamList params,
//         std::function<Node(Vector<Node>, SharedPtr<Scope>, SharedPtr<ClassInstanceNode>)> impl

//     );

//     Node execute(Vector<Node> args, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

//     SharedPtr<CallableSignature> toCallableSignature() override;

//     void setCapturedScope(SharedPtr<Scope> scope) override;
//     SharedPtr<Scope> getCapturedScope() const override;

//     String toString() const override;
//     void setScope(SharedPtr<Scope> newScope) const override;

//     FunctionBody* getBody() const override { return nullptr; } // Native function has no body
// };

// #endif // NATIVE_FUNCTION_H
