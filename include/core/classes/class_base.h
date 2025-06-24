#ifndef CLASS_BASE_H
#define CLASS_BASE_H

#include "core/types.h"
#include "ast/ast_callable.h"
#include "core/functions/callable.h"
#include "core/errors.h"

class Node;
class Scope;
class ClassSignature;
class FunctionSignature;
class MethodSignature;
class ClassBody;


// Represents the runtime definition of a class.
class ClassBase : public Callable {
protected:
    // The class scope that holds method definitions and member variable declarations.
    String accessor;
    SharedPtr<Scope> classScope;
    WeakPtr<Scope> capturedScope;
    SharedPtr<Scope> initialCapturedScope;


public: 
    ClassBase(String name, String accessor, SharedPtr<Scope> classScope);

    ~ClassBase();
    // Add a method to the class by storing its function pointer in the class scope.
    void addMethod(const String& name, SharedPtr<Method> methodSignature);
    // Retrieve a method's signature from the class scope.
    SharedPtr<Callable> getMethod(const String& name);

    // Update (or set) a member variable's value.
    void addMember(const String& name, UniquePtr<VarNode> value);

    // Retrieve a member variable's value.
    Node getMember(const String& name);

    virtual Node execute(Vector<Node> args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

    void setCapturedScope(SharedPtr<Scope> scope) override;
    void setClassScope(SharedPtr<Scope> scope);

    SharedPtr<Scope> getCapturedScope() const override;
    SharedPtr<Scope> getClassScope() const;

    String getAccessor();
    String& getQualifiedAccessor();
    void setScope(SharedPtr<Scope> newScope) const;

    virtual String toString() const override;

    void setParameters(ParamList params);
    ParamList getParameters() {return parameters;}


    // Produce a ClassSignature for this class definition.
    virtual SharedPtr<CallableSignature> toCallableSignature() override; 
    
};





    
    
class ClassNode : public CallableNode {
public:
    ClassNode(SharedPtr<ClassBase> callable) : CallableNode(callable, "Class") {
        data.type = NodeValueType::Class;
    }

    SharedPtr<Callable> getCallable() const override {
        return std::static_pointer_cast<Callable>(std::get<SharedPtr<ClassBase>>(data.value));
    }
};



class ClassInstance : public Callable { 
private:
    SharedPtr<Scope> capturedScope;
    SharedPtr<Scope> instanceScope; // reference to definition (optional)
    String accessor;

public:
    bool isConstructed = false;

    ClassInstance(const String& name, SharedPtr<Scope> capturedScope, SharedPtr<Scope> instanceScope, ParamList params, const String& accessor);
    // ClassInstance(const String name, SharedPtr<Scope> capturedScope, SharedPtr<Scope> instanceScope, ParamList params, const String accessor); // for NativeClasses

    ~ClassInstance() override;
    
    Node execute(const Vector<Node> args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const;

    SharedPtr<Scope> getCapturedScope() const override;
    void setCapturedScope(SharedPtr<Scope> scope) override;
    SharedPtr<Scope> getInstanceScope() const { return instanceScope; }

    SharedPtr<CallableSignature> toCallableSignature() override;

    String toString() const override;
    String getAccessor() {return accessor;} 

    void construct(const Vector<Node>& args, SharedPtr<ClassInstance> classInstance);
    Node call(String name, Vector<Node> args);

    virtual Node getField(const String& name) const;                     // assumes a variable
    virtual Node getField(const String& name, TokenType type) const;     // specific to what kind of member i.e var/method

    virtual void declareField(const String& name, const Node& val);             // probably only used in dynamic construction of a class
    virtual void declareField(const String& name, const VarNode& val);             // probably only used in dynamic construction of a class

    virtual void updateField(const String& name, Node val) const;                 // most commonly used
    
    SharedPtr<Scope> getInstanceScope();
    void setInstanceScope(SharedPtr<Scope> scope); 
    void setScope(SharedPtr<Scope> newScope) const override;


};

class ClassInstanceNode : public CallableNode {
public:
    ClassInstanceNode(SharedPtr<ClassInstance> callable);
    ClassInstanceNode(SharedPtr<CallableNode> callable);

    ClassInstanceNode getInstanceNode() const;
    SharedPtr<ClassInstance> getInstance() const; 
    SharedPtr<Scope> getScope();
    SharedPtr<Scope> getInstanceScope();
    SharedPtr<Callable> getCallable() const override;

    String toString() const override;
};


class ClassSignature : public CallableSignature {
private:
    String accessor;

public:
    explicit ClassSignature(SharedPtr<ClassBase> classBase);
    ~ClassSignature();
    String getAccessor() const;

    SharedPtr<ClassBase> getClassDef() const;

    SharedPtr<ClassInstance> instantiate(const Vector<Node>& args) const;

    Node call(const Vector<Node>& args, SharedPtr<Scope> scope, SharedPtr<Scope> classScope) const;
};

#endif // CLASS_BASE_H
