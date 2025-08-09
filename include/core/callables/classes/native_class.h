#ifndef NATIVE_CLASS_H
#define NATIVE_CLASS_H


#include "core/types.h"
#include "core/callables/callable.h"
#include "core/callables/classes/class_base.h"
#include "core/callables/classes/native_method.h"
#include "core/callables/callable.h"

#include "core/scope.h"

class NativeClass : public ClassBase {
private:
    std::function<void(ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode>)> constructorFn;
 
public:
    NativeClass(String name, String accessor, SharedPtr<Scope> classScope);
    ~NativeClass() override;
    void addMethod(const String& name, SharedPtr<NativeMethod> method);

    void setConstructor(std::function<void(ArgResultType, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode>)> fn);

    Node execute(ArgResultType args,
                 SharedPtr<Scope> scope,
                 [[maybe_unused]] SharedPtr<ClassInstanceNode> instance = nullptr) const override;

    String toString() const override;

    SharedPtr<CallableSignature> toCallableSignature() override;

    void setCapturedScope(SharedPtr<Scope> scope) override;
    SharedPtr<Scope> getCapturedScope() const override;
    void setScope(SharedPtr<Scope> newScope) const override;
};

#endif //NATIVE_CLASS_H
