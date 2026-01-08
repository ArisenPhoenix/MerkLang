#ifndef INVOCABLE_H
#define INVOCABLE_H

#include "core/callables/Callable.hpp"
#include "core/types.h"

class Scope;
class Method;
class Function;
class Chain;
class CallableBody;

class Invocable : public Callable {
protected:
    SharedPtr<Scope> capturedScope;  // Store the captured scope to prevent automatic destruction (a shared_ptr)

public:
    Invocable(String name, ParamList params, CallableType definedType, bool requiresReturn = false, bool isStatic = true);
    virtual Node execute(ArgumentList args, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> self) const override = 0;
    virtual SharedPtr<CallableSignature> toCallableSignature() = 0;

    virtual void setScope(SharedPtr<Scope> newScope) const override = 0;
    virtual String toString() const override = 0; 

    virtual UniquePtr<CallableBody> getBody() = 0;
    virtual CallableBody* getBody() const {return nullptr;}
    virtual CallableBody* getInvocableBody() {return nullptr;}
        
    virtual void setCapturedScope(SharedPtr<Scope> scope);
    virtual SharedPtr<Scope> getCapturedScope() const;

};

#endif // INVOCABLE_H





