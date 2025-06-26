// core/functions/function_node.h
#ifndef FUNCTION_NODE_H
#define FUNCTION_NODE_H

#include <functional>
#include <memory> 

#include "core/types.h"
#include "core/node.h"

#include "core/callables/param_node.h"    // For ParamList
#include "core/callables/callable.h"      // New base callable class
#include "core/callables/invocalble.h"

class Scope;
class NativeFunction;
class CallableBody;


class Function : public Invocable {

public:
    Function(String name, ParamList params, CallableType functionType, bool requiresReturn = true, bool isStatic = false);
    virtual ~Function() = default;
    virtual Node execute(const Vector<Node> args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const = 0;
    virtual SharedPtr<CallableSignature> toCallableSignature() = 0;

    virtual FunctionBody* getThisBody() = 0;
    virtual FunctionBody* getThisBody() const = 0;

    virtual UniquePtr<CallableBody> getBody() override = 0;
    virtual CallableBody* getBody() const override = 0;
    virtual CallableBody* getInvocableBody() override = 0;
    // virtual UniquePtr<CallableBody>& getBody() override = 0;

    virtual void setScope(SharedPtr<Scope> newScope) const override = 0;
    virtual String toString() const override = 0; 


};



    
class UserFunction : public Function {

public:
    mutable UniquePtr<FunctionBody> body;   // The function’s code block


public:
    UserFunction(String name, UniquePtr<FunctionBody> body, ParamList parameters, CallableType funcType);

    Node execute(Vector<Node> args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

    SharedPtr<CallableSignature> toCallableSignature() override;
    
    FunctionBody* getThisBody() override; 
    FunctionBody* getThisBody() const override;
    virtual UniquePtr<CallableBody> getBody() override;
    virtual CallableBody* getBody() const override;
    virtual CallableBody* getInvocableBody() override;
    // UniquePtr<CallableBody>& getBody() override;

    // FunctionBody* getInvocableBody() const = 0;
    // FunctionBody* getInvocableBody() = 0;
    void setScope(SharedPtr<Scope> newScope) const override;
    String toString() const override;

};


class FunctionNode : public CallableNode {
public:
    FunctionNode(SharedPtr<Function> function);

    FunctionNode(SharedPtr<NativeFunction> native);

    FunctionNode(SharedPtr<Callable> function);


    SharedPtr<Callable> getCallable() const override;
};


// // Function inherits from Callable.
// class Function : public Callable {

// public:
//     Function(String name, ParamList params, CallableType functionType);
//     virtual ~Function() = default;

//     // Execute must be implemented by derived classes.
//     virtual Node execute(const Vector<Node> args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const = 0;

//     // Return a FunctionSignature representing this callable.
//     virtual SharedPtr<CallableSignature> toCallableSignature() = 0;

//     virtual FunctionBody* getBody() const { return nullptr; }
//     virtual FunctionBody* getBody() {return nullptr;}
    

//     virtual String toString() const override = 0;

//     virtual void setCapturedScope(SharedPtr<Scope> scope) = 0;
//     virtual SharedPtr<Scope> getCapturedScope() const = 0;
//     virtual void setScope(SharedPtr<Scope> newScope) const override = 0;


// };
    
// class UserFunction : public Function {
// public:
//     UniquePtr<FunctionBody> body;   // The function’s code block


// public:
//     UserFunction(String name, UniquePtr<FunctionBody> body, ParamList parameters, CallableType funcType);

//     Node execute(Vector<Node> args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

//     SharedPtr<CallableSignature> toCallableSignature() override;
    
//     FunctionBody* getBody() override;
//     FunctionBody* getBody() const override;

//     String toString() const override;
//     void setScope(SharedPtr<Scope> newScope) const override;

// };

#endif // FUNCTION_NODE_H
