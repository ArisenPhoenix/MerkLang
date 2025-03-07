#ifndef AST_CONTROL_H
#define AST_CONTROL_H

#include "core/types.h"
#include "ast/ast_base.h"

#include <unordered_set>
#include "core/errors.h"

// This alias was used for clarity, when dealing with free variables specifically. 
// The codebase is getting quite large so being explicit has helped to avoid unnecessary problems
using FreeVars = std::unordered_set<String>;

class Scope;

class Control {
public:
    virtual void setCapturedScope(SharedPtr<Scope> newScope) = 0;

};

// Used as sort of a mixin class for a common interface among only those classes needing it.
class FreeVarCollection {
    public:
        mutable FreeVars freeVars;
        mutable FreeVars localAssign;
        virtual FreeVars collectFreeVariables() const = 0;
    };

class CodeBlock : public BaseAST, public FreeVarCollection {

protected:
    Vector<UniquePtr<BaseAST>> children;

public:
    explicit CodeBlock(SharedPtr<Scope> scope);

    CodeBlock(const CodeBlock&) = delete;
    CodeBlock& operator=(const CodeBlock&) = delete;
    ~CodeBlock() override;

    void addChild(UniquePtr<BaseAST> child);

    bool contains(const BaseAST* node) const;
    Vector<UniquePtr<BaseAST>> getChildrenOfType(AstType AstType, bool recurse = false);
    
    SharedPtr<Scope> getCurrentScope(){return getScope();} 

    const Vector<UniquePtr<BaseAST>>& getChildren() const;
    void printAST(std::ostream& os, int indent = 0) const override;

    AstType getAstType() const override { return AstType::CodeBlock; }
    String getAstTypeAsString() const override {return astTypeToString(getAstType());}
    String toString() const override;

    Node evaluate(SharedPtr<Scope> scope) const override;
    Node evaluate() const override;

    bool containsReturnStatement() const;

    friend class FunctionBody;

    UniquePtr<BaseAST> clone() const override;
    
    void setScope(SharedPtr<Scope> newScope) override;
    FreeVars collectFreeVariables() const override;

protected:
    Vector<UniquePtr<BaseAST>>& getMutableChildren() const { return const_cast<Vector<UniquePtr<BaseAST>>&>(children); }

};

class ConditionalBlock : public ASTStatement {
protected:
    UniquePtr<ASTStatement> condition; // The condition to evaluate

public:
    ConditionalBlock(UniquePtr<ASTStatement> condition, SharedPtr<Scope> scope);
    const ASTStatement* getCondition() const {return condition.get();};

    void printAST(std::ostream& os, int indent = 0) const override;
    Node evaluate(SharedPtr<Scope> scope) const override {return condition.get()->evaluate(scope);}
    Node evaluate() const override {return evaluate(getScope());}

    String getAstTypeAsString() const override {return astTypeToString(getAstType());}
    AstType getAstType() const override {return AstType::Conditional;}

    UniquePtr<BaseAST> clone() const override;
};




class ElseStatement : public ASTStatement, public FreeVarCollection {
    mutable UniquePtr<CodeBlock> body;
public:
    explicit ElseStatement(UniquePtr<CodeBlock> body, SharedPtr<Scope> scope);

    UniquePtr<CodeBlock>& getBody(){return body;}
    UniquePtr<CodeBlock>& getBody() const {return body;}

    Node evaluate(SharedPtr<Scope> scope) const override;
    Node evaluate() const override {return evaluate(getScope());}

    AstType getAstType() const override { return AstType::ElseStatement;}
    String getAstTypeAsString() const override {return astTypeToString(getAstType());}

    void printAST(std::ostream& os, int indent = 0) const override;

    UniquePtr<BaseAST> clone() const override;

    FreeVars collectFreeVariables() const override;

    void setScope(SharedPtr<Scope> newScope) override;
};

class ElifStatement : public ConditionalBlock, public FreeVarCollection { // Also contains the logic of If, parser's job to ensure if is always first.

protected:
    mutable UniquePtr<CodeBlock> body;
public:
    ElifStatement(UniquePtr<ASTStatement> condition, UniquePtr<CodeBlock> body, SharedPtr<Scope> scope);

    Node evaluate(SharedPtr<Scope> scope) const override;

    const UniquePtr<CodeBlock>& getBody() const { return body; }
    Node evaluate() const override {return evaluate(getScope());}
    AstType getAstType() const override { return AstType::ElifStatement; }
    void printAST(std::ostream& os, int indent = 0) const override;
    String getAstTypeAsString() const override {return astTypeToString(getAstType());}
    
    UniquePtr<BaseAST> clone() const override;
    FreeVars collectFreeVariables() const override;

    virtual void setScope(SharedPtr<Scope> newScope) override;    
};


// Originally IfStatement inherited from ElifStatement because if it quacks and walks like a duck...
// During some debugging it was made to be a standalone, the issue was not related to its derivative
// However, I haven't decided to make it inherit from Elif again, for actually no good reason, though it is a bit counterintuitive.
class IfStatement : public ConditionalBlock, public FreeVarCollection {
protected:
    mutable UniquePtr<CodeBlock> body;
    Vector<UniquePtr<ElifStatement>> elifNodes; // Vector of `elif` nodes
    UniquePtr<ElseStatement> elseNode;          // Optional `else` block
    
public:
    IfStatement(UniquePtr<ASTStatement>condition, UniquePtr<CodeBlock> body, SharedPtr<Scope> scope);

    void addElifNode(UniquePtr<ElifStatement> elifNode) {
        if (!elifNode) {
            throw RunTimeError("Cannot add a null elif node to IfStatement.");
        }
            elifNodes.push_back(std::move(elifNode));
        }

    void setElseNode(UniquePtr<ElseStatement> elseNode) {this->elseNode = std::move(elseNode);}

    Node evaluate(SharedPtr<Scope> scope) const override;
    Node evaluate() const override {return evaluate(getScope());}

    const Vector<UniquePtr<ElifStatement>>& getElifs() const {return elifNodes;}
    const UniquePtr<ElseStatement>& getElse() const {return elseNode;}

    AstType getAstType() const override { return AstType::IfStatement; }
    String getAstTypeAsString() const override {return astTypeToString(getAstType());}

    void printAST(std::ostream& os, int indent = 0) const override;

    UniquePtr<BaseAST> clone() const override;
    const UniquePtr<CodeBlock>& getBody() const { return body; }
    
    FreeVars collectFreeVariables() const override;
    
    void setScope(SharedPtr<Scope> newScope) override;
};

class NoOpNode: public ASTStatement {
public:
    NoOpNode(SharedPtr<Scope> scope) : ASTStatement(scope) {}

    Node evaluate([[maybe_unused]] SharedPtr<Scope> scope) const override;
    Node evaluate() const override {return evaluate(getScope());}
    String toString() const override {return "NoOpNode";}
    
    AstType getAstType() const override {return AstType::NoOp;};
    String getAstTypeAsString() const override {return astTypeToString(getAstType());}
    void printAST(std::ostream& os, int indent = 0) const override;
    UniquePtr<BaseAST> clone() const override;
}; 

// New base class for loops
class LoopBlock : public ASTStatement, public FreeVarCollection {
protected:
    UniquePtr<ConditionalBlock> condition; // Condition to check before each iteration
    UniquePtr<CodeBlock> body;             // The loop's body

public:
    explicit LoopBlock(UniquePtr<ConditionalBlock> condition, UniquePtr<CodeBlock> body, SharedPtr<Scope> scope);
    ~LoopBlock();
    
    const ConditionalBlock* getCondition() const { return condition.get(); }
    const CodeBlock* getBody() const {return body.get();}

    void printAST(std::ostream& os, int indent = 0) const override {(void)os; (void)indent; throw MerkError("Cannot Print a Base LoopBlock");};
 
    // Executes the loop
    Node evaluate(SharedPtr<Scope> scope) const override;
    Node evaluate() const override {return evaluate(getScope());}

    virtual AstType getAstType() const override {return AstType::LoopBlock;};
    virtual String getAstTypeAsString() const override {return astTypeToString(getAstType());}
    virtual String toString() const override { return getAstTypeAsString();}
    virtual FreeVars collectFreeVariables() const override = 0;

    void setScope(SharedPtr<Scope> newScope) override;

    

};

class WhileLoop : public LoopBlock {
public:
    WhileLoop(UniquePtr<ConditionalBlock> condition, UniquePtr<CodeBlock> body, SharedPtr<Scope> scope);
    ~WhileLoop();
    String toString() const override;

    Node evaluate(SharedPtr<Scope> scope) const override;
    Node evaluate() const override {return evaluate(getScope());}

    void printAST(std::ostream& os, int indent = 0) const override;

    AstType getAstType() const override {return AstType::WhileLoop;};
    String getAstTypeAsString() const override {return astTypeToString(getAstType());}

    UniquePtr<BaseAST> clone() const override;

    FreeVars collectFreeVariables() const override;

};

 


#endif //AST_CONTROL_H