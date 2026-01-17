#pragma once

// #include "core/types.h"
#include "core/TypesFWD.hpp"
#include "core/callables/Callable.hpp"
#include "core/callables/classes/ClassBase.hpp"
#include "core/callables/classes/NativeMethod.hpp"
#include "core/callables/Callable.hpp"

#include "core/Scope.hpp"

class NativeClass : public ClassBase {
private:
    std::function<void(ArgumentList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode>)> constructorFn;
 
public:
    NativeClass(String name, String accessor, SharedPtr<Scope> classScope);
    ~NativeClass() override;
    void addMethod(const String& name, SharedPtr<NativeMethod> method);

    void setConstructor(std::function<void(ArgumentList, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode>)> fn);

    Node execute(ArgumentList args,
                 SharedPtr<Scope> scope,
                 [[maybe_unused]] SharedPtr<ClassInstanceNode> instance = nullptr) const override;

    String toString() const override;

    SharedPtr<CallableSignature> toCallableSignature() override;

    void setCapturedScope(SharedPtr<Scope> scope) override;
    SharedPtr<Scope> getCapturedScope() const override;
    void setScope(SharedPtr<Scope> newScope) const override;
};
