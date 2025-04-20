// core/functions/function_node.h
#ifndef FUNCTION_NODE_H
#define FUNCTION_NODE_H

#include <functional>
#include <memory> 

#include "core/types.h"
#include "core/node.h"
#include "core/functions/param_node.h"    // For ParamList
#include "ast/ast_control.h"              // For CodeBlock
#include "core/functions/callable.h"      // New base callable class
#include "ast/ast_callable.h"



class Scope;

class FunctionBody : public CallableBody {
    public:
        friend class MethodBody;
        FunctionBody(SharedPtr<Scope> scope);
    
        // Steal Constructor
        FunctionBody(UniquePtr<CodeBlock>&& block);
    
        ~FunctionBody(){DEBUG_LOG(LogLevel::TRACE, highlight("Destroying FunctionBody", Colors::orange));} 
    
        virtual Node evaluate(SharedPtr<Scope> scope) const override;
        virtual UniquePtr<BaseAST> clone() const override;
        virtual AstType getAstType() const override { return AstType::FunctionBlock;}    
        void printAST(std::ostream& os, int indent = 0) const override;
        // UniquePtr<BaseAST> clone() const override;
    
        
        Vector<UniquePtr<BaseAST>>& getChildren(){return children;};
    };
// class FunctionBody;




// Function now inherits from Callable.
class Function : public Callable {
protected:
    CallableType subType = CallableType::DEF;
public:
    // Constructor: simply forward to Callable.
    Function(String name, ParamList params, CallableType functionType);
    virtual ~Function() = default;

    // Execute must be implemented by derived classes.
    virtual Node execute(const Vector<Node> args, SharedPtr<Scope> scope) const = 0;

    // Return a FunctionSignature representing this callable.
    virtual SharedPtr<CallableSignature> toCallableSignature() = 0;

    // Optionally, return a pointer to the function body (if applicable).
    virtual FunctionBody* getBody() const { return nullptr; }
    // virtual UniquePtr<CallableBody> getBody() const = 0;

    virtual String toString() const override = 0;

    virtual void setCapturedScope(SharedPtr<Scope> scope) = 0;
    virtual SharedPtr<Scope> getCapturedScope() const = 0;
    virtual void setScope(SharedPtr<Scope> newScope) const override = 0;


};
    
class UserFunction : public Function {
private:
    SharedPtr<Scope> capturedScope;  // Store the captured scope to prevent automatic destruction (a shared_ptr)

public:
    UniquePtr<FunctionBody> body;   // The function’s code block


public:
    UserFunction(String name, UniquePtr<FunctionBody> body, ParamList parameters, CallableType funcType);

    Node execute(Vector<Node> args, SharedPtr<Scope> scope) const override;

    void setCapturedScope(SharedPtr<Scope> scope);

    SharedPtr<Scope> getCapturedScope() const;

    SharedPtr<CallableSignature> toCallableSignature() override;
    
    FunctionBody* getBody() const override;
    // UniquePtr<CallableBody> getBody() const override {return body;}

    String toString() const override {return "<Function: " + getName() + ">";}
    void setScope(SharedPtr<Scope> newScope) const override;

};


class FunctionNode : public CallableNode {
public:
    FunctionNode(SharedPtr<Function> function) : CallableNode(function , "Function") {data.type = NodeValueType::Function;}

    FunctionNode(SharedPtr<Callable> function) : CallableNode(function, "Function") {
        data.type = NodeValueType::Function;
    }
    SharedPtr<Callable> getCallable() const override {return std::get<SharedPtr<Function>>(data.value);}
};


#endif // FUNCTION_NODE_H
