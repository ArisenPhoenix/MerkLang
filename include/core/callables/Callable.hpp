#pragma once

// #include "core/types.h"
#include "core/TypesFWD.hpp"
#include "ast/AstCallable.hpp"
#include "core/errors.h"

// Callable is the abstract base for all callables (functions and methods).
class Callable: public NodeBase {
public:
    String name;
    mutable ParamList parameters;
    CallableType callType;
    CallableType subType;


    bool getIsStatic();
    void setIsStatic(bool);
    bool getRequiresReturn();
    void setRequiresReturn(bool);

    Callable() {}
    Callable(Callable& callable); 
    Callable(Method& method);
    Callable(Function& function);
    Callable(String name, ParamList params, CallableType callType);
    Callable(String name, ParamList params, CallableType callType, bool requiresReturn, bool isStatic);
    virtual ~Callable() = default;

    String getName() const;
    String& getQualifiedName();

    CallableType getCallableType() const;
    CallableType getSubType() const;

    void setSubType(CallableType subClassification);
    void setCallableType(CallableType primaryClassification);
    void placeArgsInCallScope(ArgResultType evaluatedArgs, SharedPtr<Scope> callScope) const;

    virtual Node execute(ArgResultType args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const = 0;
    
    // Set and get the captured scope.
    virtual void setCapturedScope(SharedPtr<Scope> scope) = 0;
    virtual SharedPtr<Scope> getCapturedScope() const = 0;
    virtual void setScope(SharedPtr<Scope> newScope) const = 0;
    virtual String toString() const = 0; 
    
    // Produce a FunctionSignature for registration.
    virtual SharedPtr<class CallableSignature> toCallableSignature() = 0;

    virtual void setValue(const VariantType&) override;
    virtual VariantType getValue() const override;
    virtual SharedPtr<NodeBase> clone();
    virtual void clear();
    NodeValueType getType() const override;

    // virtual NodeValueType getType() override;
    virtual SharedPtr<NodeBase> clone() const;
    

protected:
    bool requiresReturn = false;
    bool isStatic = false;

};


class CallableNode : public Node {
protected:
    SharedPtr<Scope> internalScope; // Optional — used for chained resolution

public:
    explicit CallableNode(SharedPtr<Callable> callable, String callableType = "Callable");
    explicit CallableNode(SharedPtr<CallableNode> callableNode); // for returning non shared callable node
    explicit CallableNode(Vector<SharedPtr<CallableSignature>> callSignatures, String callableType = "Callable");

    virtual SharedPtr<Callable> getCallable() const;

    void setInternalScope(SharedPtr<Scope> scope);
    SharedPtr<Scope> getInternalScope() const;

    virtual String toString() const;
    std::size_t hash() const override;

    virtual bool isInstance() const override;
    virtual bool isInstance() override;

    SharedPtr<ClassInstance> toInstance() override;
    SharedPtr<ClassInstance> toInstance() const override;
    virtual SharedPtr<CallableNode> clone() const;
    virtual VariantType getValue() const;
    
};


SharedPtr<Callable> asCallable(SharedPtr<NodeBase> callable);

