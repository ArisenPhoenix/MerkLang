#ifndef NATIVE_CLASS_H
#define NATIVE_CLASS_H

#include "core/types.h"
#include "core/functions/argument_node.h"
#include "core/functions/callable.h"
#include "core/classes/class_base.h"
#include "core/classes/native_method.h"
#include "core/functions/callable.h"

#include "core/scope.h"

class NativeClass : public ClassBase {
private:
    // std::function<void(SharedPtr<ClassInstanceNode>, Vector<Node>)> constructorFn;
    std::function<void(Vector<Node>, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode>)> constructorFn;
    // std::unordered_map<String, SharedPtr<NativeMethod>> methodMap;

public:
    NativeClass(String name, String accessor, SharedPtr<Scope> classScope);

    void addMethod(const String& name, SharedPtr<NativeMethod> method);

    void setConstructor(std::function<void(Vector<Node>, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode>)> fn);

    Node execute(Vector<Node> args,
                 SharedPtr<Scope> scope,
                 [[maybe_unused]] SharedPtr<ClassInstanceNode> instance = nullptr) const override;

    String toString() const override;

    SharedPtr<CallableSignature> toCallableSignature() override;

    void setCapturedScope(SharedPtr<Scope> scope) override;
    SharedPtr<Scope> getCapturedScope() const override;
    void setScope(SharedPtr<Scope> newScope) const override;
    // SharedPtr<CallableSignature> toCallableSignature() const;
};

#endif //NATIVE_CLASS_H
