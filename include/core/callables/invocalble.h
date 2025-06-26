#ifndef INVOCABLE_H
#define INVOCABLE_H

// #include "ast/ast_function.h"
// #include "ast/ast_method.h"
#include "core/callables/callable.h"


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
    virtual Node execute(Vector<Node> args, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> self) const override = 0;
    virtual SharedPtr<CallableSignature> toCallableSignature() = 0;

    virtual void setScope(SharedPtr<Scope> newScope) const override = 0;
    virtual String toString() const override = 0; 

    virtual UniquePtr<CallableBody> getBody() = 0;
    virtual CallableBody* getBody() const {return nullptr;}
    virtual CallableBody* getInvocableBody() {return nullptr;}
    
    // virtual UniquePtr<CallableBody>& getBody() = 0;
    
    virtual void setCapturedScope(SharedPtr<Scope> scope);
    virtual SharedPtr<Scope> getCapturedScope() const;

};






// class FunctionNode : public CallableNode {
// public:
//     FunctionNode(SharedPtr<Function> function);

//     FunctionNode(SharedPtr<Callable> function);
//     FunctionNode(SharedPtr<NativeFunction> native);

//     SharedPtr<Callable> getCallable() const override;
// };



#endif // INVOCABLE_H





