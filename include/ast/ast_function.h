#ifndef AST_FUNCTION_H
#define AST_FUNCTION_H

#include <unordered_set>
#include "utilities/debugger.h"
#include "ast/ast.h"
#include "core/functions/function_node.h"

class Scope;


class ParameterAssignment : public VariableAssignment {
    public:
        ParameterAssignment(String name, UniquePtr<ASTStatement> valueNode, SharedPtr<Scope> scope)
            : VariableAssignment(std::move(name), std::move(valueNode), scope) {}
    
        String toString() const override;
        void printAST(std::ostream& os, int indent = 0) const override;
        AstType getAstType() const override { return AstType::ParameterAssignment; }
        String getAstTypeAsString() const override {return astTypeToString(getAstType());}
    };
    

// Special CodeBlack for properly managing FunctionCall Scope and Parameters
// Would allow for modifications just in case, without requiring a whole rewrite of the CodeBlock
class FunctionBody : public CodeBlock {
public:
    FunctionBody(SharedPtr<Scope> scope);
    // Steal Constructor
    FunctionBody(UniquePtr<CodeBlock>&& block)
        : CodeBlock(block->getScope()) {
        this->children = std::move(block->children);
        block.reset();
    }

    ~FunctionBody(){
        DEBUG_LOG(LogLevel::TRACE, highlight("Destroying FunctionBody", Colors::orange));
    } 

    Node evaluate(SharedPtr<Scope> scope) const override;
    Node evaluate() const override {return evaluate(getScope());}
    void printAST(std::ostream& os, int indent = 0) const;
    Vector<UniquePtr<BaseAST>>& getChildren(){return children;};
    UniquePtr<BaseAST> clone() const override;

    AstType getAstType() const override { return AstType::FunctionBlock;}
    String getAstTypeAsString() const override {return astTypeToString(getAstType());}
};



// FunctionDef: defines a function, registers it, and may return nothing.
class FunctionDef : public ASTStatement {
private:
    String name;
    ParamList parameters;
    UniquePtr<FunctionBody> body;

public:
    FunctionType funcType;

    FunctionDef(String name, ParamList parameters, UniquePtr<FunctionBody> body, FunctionType funcType, SharedPtr<Scope> scope);
    Node evaluate(SharedPtr<Scope> scope) const override;
    Node evaluate() const override { return evaluate(getScope()); }
    AstType getAstType() const override { return AstType::FunctionDefinition;}
    String getAstTypeAsString() const override {return astTypeToString(getAstType());}
    UniquePtr<BaseAST> clone() const override;
    void printAST(std::ostream& os, int indent = 0) const override;
    UniquePtr<FunctionBody>& getBody() { return body; }
};

// FunctionRef: returns a reference to a function as a value.
// Not yet fully implemented as it will require a Node class which stores a reference to the vector of functions to accomodate overloads
// So, it hasn't been completed YET.
class FunctionRef : public ASTStatement {
private:
    String functionName;
public:

    FunctionRef(String name, SharedPtr<Scope> scope);

    Node evaluate(SharedPtr<Scope> scope) const override;
    Node evaluate() const override { return evaluate(getScope()); }
    AstType getAstType() const override { return AstType::FunctionReference;}
    String getAstTypeAsString() const override {return astTypeToString(getAstType());}

    void printAST(std::ostream& os, int indent = 0) const override;
    UniquePtr<BaseAST> clone() const override;
};



// FunctionCall: evaluates arguments, retrieves a function, and executes it.
class FunctionCall : public ASTStatement {
private:
    String functionName;
    Vector<UniquePtr<ASTStatement>> arguments;
public:

    FunctionCall(String functionName, Vector<UniquePtr<ASTStatement>> arguments, SharedPtr<Scope> scope);

    Node evaluate(SharedPtr<Scope> scope) const override;
    Node evaluate() const override { return evaluate(getScope()); }
    AstType getAstType() const override { return AstType::FunctionCall;}
    String getAstTypeAsString() const override {return astTypeToString(getAstType());}
    void printAST(std::ostream& os, int indent = 0) const override;
    UniquePtr<BaseAST> clone() const override;
};

#endif // AST_FUNCTION_H


