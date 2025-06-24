#ifndef CALLABLE_H
#define CALLABLE_H

#include "core/types.h"
#include "ast/ast_callable.h"
#include "core/functions/param_node.h"
#include "core/errors.h" 

class Scope;
class Node;
class ParamList;
class Method;

// Callable is the abstract base for all callables (functions and methods).
class Callable : public std::enable_shared_from_this<Callable> {
public:
    String name;
    mutable ParamList parameters;
    CallableType callType;
    CallableType subType;
    bool requiresReturn = true;

    Callable() {}
    Callable(Callable& callable); 
    Callable(Method& method);
    Callable(Function& function);
    Callable(String name, ParamList params, CallableType callType);
    Callable(String name, ParamList params, CallableType callType, bool requiresReturn);
    virtual ~Callable() = default;
    
    // Execute the callable with the provided arguments and scope.
    virtual Node execute(Vector<Node> args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const = 0;
    
    // Set and get the captured scope.
    virtual void setCapturedScope(SharedPtr<Scope> scope) = 0;
    virtual SharedPtr<Scope> getCapturedScope() const = 0;
    
    // Produce a FunctionSignature for registration.
    virtual SharedPtr<class CallableSignature> toCallableSignature() = 0;
    virtual String toString() const = 0; 

    virtual void setScope(SharedPtr<Scope> newScope) const = 0;
    
    String getName() const;
    String& getQualifiedName();

    CallableType getCallableType() const;
    CallableType getSubType() const;
    void setSubType(CallableType subClassification);
    void setCallableType(CallableType primaryClassification);
    void placeArgsInCallScope(Vector<Node> evaluatedArgs, SharedPtr<Scope> callScope) const;

};


class CallableNode : public Node {
protected:
    SharedPtr<Scope> internalScope; // Optional — used for chained resolution

public:
    explicit CallableNode(SharedPtr<Callable> callable, String callableType = "Callable");
    explicit CallableNode(SharedPtr<CallableNode> callableNode); // for returning non shared callable node


    virtual SharedPtr<Callable> getCallable() const;

    void setInternalScope(SharedPtr<Scope> scope);
    SharedPtr<Scope> getInternalScope() const;

    virtual String toString() const;
};




#endif // CALLABLE_H 
