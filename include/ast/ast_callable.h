#ifndef AST_CALLABLE_H
#define AST_CALLABLE_H

#include <unordered_set>
#include "utilities/debugger.h" 
#include "core/functions/param_node.h"
#include "ast/ast_base.h"
#include "ast/ast.h"
#include "ast/ast_control.h"

class Scope;
class MethodDef;
class Callable;
class MethodBody;
class FunctionBody;
// class CodeBlock;


class CallableSignature {
private:
    SharedPtr<Callable> callable;
    mutable Vector<NodeValueType> parameterTypes; 
    CallableType callType;
    CallableType subType; // DEF, FUNCTION, etc.

public:
    explicit CallableSignature(SharedPtr<Callable> callable, CallableType callType);
    virtual ~CallableSignature() = default;  // <-- Add this line

    Node call(const Vector<Node>& args, SharedPtr<Scope> scope) const;

    bool matches(const Vector<NodeValueType>& otherTypes) const;
    
    // Return the stored callable.
    SharedPtr<Callable> getCallable() const;

    // CallableBody* getBody() const {return callable->getBody();}
    

    // template<typename T>
    // SharedPtr<T> getCallableClonedAs() const;

    void setParameterTypes(Vector<NodeValueType> paramTypes) {parameterTypes = paramTypes;} 
    const Vector<NodeValueType>& getParameterTypes() const;
    CallableType getCallableType() { return callType; }
    void setCallableType(CallableType callType);
    bool getIsUserFunction();
    CallableType getSubType() {return subType;}
    void setSubType(CallableType subClassification) {subType = subClassification;}

};


// Special CodeBlack for properly managing FunctionCall Scope and Parameters
// Would allow for modifications just in case, without requiring a whole rewrite of the CodeBlock
class CallableBody : public CodeBlock {
public:
    // friend class MethodBody;
    CallableBody(SharedPtr<Scope> scope);
    CallableBody(UniquePtr<CodeBlock>&& block)
        : CodeBlock(block->getScope()) {
        this->children = std::move(block->children);
        block.reset();
    }

    CallableBody(UniquePtr<CallableBody>&& block);
    CallableBody(UniquePtr<CallableBody>* body);
    ~CallableBody(){
        DEBUG_LOG(LogLevel::TRACE, highlight("Destroying FunctionBody", Colors::orange));
    } 


    virtual AstType getAstType() const override { return AstType::CallableBody;}
    virtual Vector<UniquePtr<BaseAST>>& getChildren(){return children;};

    virtual void printAST(std::ostream& os, int indent = 0) const;
    virtual Node evaluate(SharedPtr<Scope> scope) const override;
    
    virtual UniquePtr<BaseAST> clone() const override;

    // For Calling Within A Method's Context
    MethodBody* toMethodBody();
    
    // For Calling Within A Function's Context
    FunctionBody* toFunctionBody();

    // CatchAll
    CallableBody* toCallableBody();
    
};



// FunctionDef: defines a function, registers it, and may return nothing.
class CallableDef : public ASTStatement {
public:
    String name;
    mutable ParamList parameters;
    UniquePtr<CallableBody> body;

    CallableType callType;
    friend class MethodDef;

    CallableDef(String name, ParamList parameters, UniquePtr<CallableBody> body, CallableType funcType, SharedPtr<Scope> scope);
    ~CallableDef() = default;
    virtual AstType getAstType() const override { return AstType::CallableDefinition;}
    virtual Node evaluate(SharedPtr<Scope> scope) const override;
    virtual UniquePtr<BaseAST> clone() const override;
    void printAST(std::ostream& os, int indent = 0) const override;
    virtual String toString() const override {return astTypeToString(getAstType());}

    UniquePtr<CallableBody>& getBody() { return body; }
    CallableBody* getBody() const {return body.get();}
    String getName() {return name;}
    ParamList getParameters() {return parameters;}
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;
    
};

class CallableCall : public ASTStatement {

public:
    String name;
    Vector<UniquePtr<ASTStatement>> arguments;
    CallableCall(String name, Vector<UniquePtr<ASTStatement>> arguments, SharedPtr<Scope> scope);

    virtual AstType getAstType() const override { return AstType::CallableCall;}
    

    virtual Node evaluate(SharedPtr<Scope> scope) const override;
    virtual void printAST(std::ostream& os, int indent = 0) const override;
    virtual UniquePtr<BaseAST> clone() const override;

    String toString() const override {return "<" + astTypeToString(getAstType()) + "name" + getName() + ">";}
    String getName() const {return name;}
    // virtual UniquePtr<CallableBody> getBody() const = 0;
    Vector<Node> handleArgs(SharedPtr<Scope> scope) const;
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;
};


class CallableRef : public ASTStatement {

public:
    String name;

    CallableRef(String name, SharedPtr<Scope> scope);
    AstType getAstType() const override { return AstType::CallableReference;}
    String toString() const override {return astTypeToString(getAstType());}

    Node evaluate(SharedPtr<Scope> scope) const override;

    String getName() {return name;}
    virtual UniquePtr<BaseAST> clone() const override;
    // Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;
};
    


    
    




#endif // AST_CALLABLE_H 