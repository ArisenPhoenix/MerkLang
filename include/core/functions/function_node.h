// core/functions/function_node.h
#ifndef FUNCTION_NODE_H
#define FUNCTION_NODE_H

#include <functional>
#include <memory>


#include "core/types.h"
#include "core/node.h"
#include "core/functions/param_node.h"    // For ParamList
#include "core/functions/function_node.h"
#include "ast/ast_control.h"              // For CodeBlock


// Forward declarations
class Scope;  
class FunctionSignature;
class FunctionBody;

// Abstract base class for all functions
class Function {
public:
    String name;
    mutable ParamList parameters;
    FunctionType funcType;
    Function(String name, ParamList params, FunctionType functionType);
    virtual ~Function() = default;

    virtual Node execute(const Vector<Node> args, SharedPtr<Scope> scope) const = 0;

    virtual SharedPtr<FunctionSignature> toFunctionSignature() const = 0;
    virtual FunctionBody* getBody() const {return nullptr;}

    virtual void setCapturedScope(SharedPtr<Scope> scope) = 0;
    virtual SharedPtr<Scope> getCapturedScope() const {return nullptr;}
    virtual FunctionType getFunctionType() const {return funcType;};
    String getName() const { return name; }


};

// A thin wrapper that “signs” a function for registration.
// In a more complete design, will include return type info.
class FunctionSignature {
private:
    SharedPtr<Function> function;
    mutable Vector<NodeValueType> parameterTypes; 
    FunctionType funcType;
     

public:
    explicit FunctionSignature(SharedPtr<Function> function, FunctionType funcType);
    ~FunctionSignature();
      
    Node call(const Vector<Node>& args, SharedPtr<Scope> scope) const;
    
    bool matches(const Vector<NodeValueType>& otherTypes) const;
    
    SharedPtr<Function> getFunction() const;
    const Vector<NodeValueType>& getParameterTypes() const;
    FunctionType getFunctionType() {return funcType;}
    void setFunctionType(FunctionType functionType);
    bool getIsUserFunction();
};



class NativeFunction : public Function {
private:
    std::function<Node(Vector<Node>&, SharedPtr<Scope>)> nativeImpl;

public:
    NativeFunction(String name, std::function<Node(Vector<Node>&, SharedPtr<Scope>)> impl, ParamList parameters);

    Node execute(Vector<Node> args, SharedPtr<Scope> scope) const override;
    virtual SharedPtr<FunctionSignature> toFunctionSignature() const = 0;
    virtual String toString() const = 0;
};




class UserFunction : public Function {
private:
    SharedPtr<Scope> capturedScope;  // Store the captured scope to prevent deletion

public:
    UniquePtr<FunctionBody> body;   // The function’s code block


public:
    UserFunction(String name, UniquePtr<FunctionBody> body, ParamList parameters, FunctionType funcType);

    Node execute(Vector<Node> args, SharedPtr<Scope> scope) const override;

    void setCapturedScope(SharedPtr<Scope> scope);

    SharedPtr<Scope> getCapturedScope() const;

    SharedPtr<FunctionSignature> toFunctionSignature() const override;
    
    FunctionBody* getBody() const override {return body.get();}

    String toString() const {return "<Function: " + getName() + ">";}
};



class FunctionNode : public Node {
public:
   
    explicit FunctionNode(SharedPtr<Function> func) {
        data.type = NodeValueType::Function;
        data.value = func; // Stored as SharedPtr<Function>
        nodeType = "FunctionNode";
    }

    SharedPtr<Function> getFunction() const {
        return std::get<SharedPtr<Function>>(data.value);
    }

    String toString() const {
        return "<Function: " + getFunction()->getName() + ">";
    }
};


#endif // FUNCTION_NODE_H
