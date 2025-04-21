#ifndef CLASS_BASE_H
#define CLASS_BASE_H

#include "core/types.h"
#include "ast/ast_callable.h"
#include "core/functions/callable.h"
#include "core/node.h"
#include "core/errors.h"
// #include "core/functions/callable.h"
// #include "core/classes/class_base.h"
class Scope;
class ClassSignature;
class FunctionSignature;
class MethodSignature;
class ClassBody;


// Represents the runtime definition of a class.
class ClassBase : public Callable {
protected:
    // The class scope that holds method definitions and member variable declarations.
    SharedPtr<Scope> scope;
    SharedPtr<Scope> classScope;
    SharedPtr<Scope> capturedScope;

    UniquePtr<ClassBody> body;
    ParamList parameters;
    String accessor;

public:
    // Constructor: Given a class name and a parent (defining) scope,
    // a new class scope is created.
    // ClassBase(String name, SharedPtr<Scope> parentScope);
    ClassBase(String name, String accessor, UniquePtr<ClassBody> body, SharedPtr<Scope> parentScope);
    ~ClassBase();
    // Add a method to the class by storing its function pointer in the class scope.
    void addMethod(const String& name, SharedPtr<Method> methodSignature);
    // Retrieve a method's signature from the class scope.
    SharedPtr<Callable> getMethod(const String& name);

    // Update (or set) a member variable's value.
    void addMember(const String& name, UniquePtr<VarNode> value);

    // Retrieve a member variable's value.
    Node getMember(const String& name);

    Node execute(Vector<Node> args, SharedPtr<Scope> scope) const override;

    void setCapturedScope(SharedPtr<Scope> scope) override;
    void setClassScope(SharedPtr<Scope> scope);

    SharedPtr<Scope> getCapturedScope() const override;
    SharedPtr<Scope> getClassScope() const;
    SharedPtr<Scope> getScope() {return scope;}
    void setScope(SharedPtr<Scope> newScope) const;
    String& getQualifiedAccessor() {return accessor;}
    String toString() const override;

    void setBody(UniquePtr<ClassBody> updatedBody);
    UniquePtr<ClassBody>& getBody();
    ClassBody* getBody() const;
    void setParameters(ParamList params) {parameters = params;}
    ParamList getParameters() {return parameters;}
    String getAccessor() {return accessor;}

    // Produce a ClassSignature for this class definition.
    SharedPtr<CallableSignature> toCallableSignature() override; 
    
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

    // SharedPtr<Scope> capturedScope;
    std::weak_ptr<Scope> capturedScope;
    // mutable SharedPtr<Scope> startingScope;
    SharedPtr<Scope> instanceScope; // reference to definition (optional)
    String accessor;

public:
    // ClassInstance(SharedPtr<ClassBase> base);
    ClassInstance(String& name, SharedPtr<Scope> capturedScope, SharedPtr<Scope> instanceScope, ParamList params, String& accessor);

    // ClassInstance(String& name, SharedPtr<Scope> capturedScope, SharedPtr<Scope> instanceScope, String& accessor);
    ~ClassInstance() override;
    
    // Node execute(const Vector<Node>& args, SharedPtr<Scope> callerScope) const;
    Node execute(const Vector<Node> args, SharedPtr<Scope> scope) const;

    SharedPtr<Scope> getCapturedScope() const override;
    void setCapturedScope(SharedPtr<Scope> scope) override;
    SharedPtr<Scope> getInstanceScope() const { return instanceScope; }

    SharedPtr<CallableSignature> toCallableSignature() override;

    String toString() const override;
    String getAccessor() {return accessor;}

    void construct(const Vector<Node>& args);
    // SharedPtr<Scope> getScope() {return startingScope;}
    
    Node getField(const String& name, TokenType type) const;

    SharedPtr<Scope> getInstanceScope();
    void setInstanceScope(SharedPtr<Scope> scope);
    void setScope(SharedPtr<Scope> newScope) const override;


};

class ClassInstanceNode : public CallableNode {
public:
    ClassInstanceNode(SharedPtr<ClassInstance> callable);

    SharedPtr<Callable> getCallable() const override;

};


class ClassSignature : public CallableSignature {
private:
    String accessor;
    UniquePtr<ClassBody> classBody;

public:
    explicit ClassSignature(SharedPtr<ClassBase> classBase);
    ~ClassSignature();
    String getAccessor() const;

    SharedPtr<ClassBase> getClassDef() const;

    SharedPtr<ClassInstance> instantiate(const Vector<Node>& args) const;

    // Optional: override call() to auto-instantiate when the class is "called"
    Node call(const Vector<Node>& args, SharedPtr<Scope> scope, SharedPtr<Scope> classScope) const;

    // UniquePtr<ClassBody>& getBody() {return classBody;}
};

#endif // CLASS_BASE_H
