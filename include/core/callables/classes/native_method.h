// #ifndef NATIVE_METHOD_H
// #define NATIVE_METHOD_H

// #include "core/types.h"
// #include "core/callables/callable.h" 
// #include "core/callables/classes/method.h"
// #include "core/callables/param_node.h"


// class NativeMethod : public Method {
// private:
//     std::function<Node(NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self)> methodFn;
//     // SharedPtr<Scope> classScope = nullptr;

// public:
//     NativeMethod(
//         String name,
//         ParamList params,
//         SharedPtr<Scope> classScope,
//         std::function<Node(NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self)> methodFn 
//     );

//     ~NativeMethod() override;
                
//     Node execute(Vector<Node> args,
//                  SharedPtr<Scope> callScope,
//                  SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

//     String toString() const override;

//     SharedPtr<CallableSignature> toCallableSignature() override;
//     // SharedPtr<CallableSignature> toCallableSignature(SharedPtr<Method> method);

//     void setCapturedScope(SharedPtr<Scope> scope) override;
//     SharedPtr<Scope> getCapturedScope() const override;
//     void setScope(SharedPtr<Scope> newScope) const override;
// };

// #endif // NATIVE_METHOD_H
