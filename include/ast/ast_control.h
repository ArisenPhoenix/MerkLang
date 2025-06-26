#ifndef AST_CONTROL_H
#define AST_CONTROL_H
#include <unordered_set>

#include "core/types.h"
#include "ast/ast_base.h"
#include "core/errors.h"

// This alias was used for clarity, when dealing with free variables specifically. 
// The codebase is getting quite large so being explicit has helped to avoid unnecessary problems

class Scope;

class Control {
public:
    virtual void setCapturedScope(SharedPtr<Scope> newScope) = 0;

};

// Used as sort of a mixin class for a common interface among only those classes needing it.
class AstCollector {
protected:
    mutable Vector<BaseAST*> collectedNodes;

public:
    const Vector<BaseAST*>& collectChildrenOfType(const Vector<UniquePtr<BaseAST>>& children, AstType type) const;

    const Vector<BaseAST*>& collectChildrenOfType(const Vector<UniquePtr<BaseAST>>& children, const Vector<AstType>& types) const;
    ~AstCollector();
};

// #endif // AST_COLLECTOR_H

    

class CodeBlock : public BaseAST, AstCollector {

protected:
    SharedPtr<Scope> scope;
    Vector<UniquePtr<BaseAST>> children;
    bool containsReturn = false;
public:
    explicit CodeBlock(SharedPtr<Scope> scope);

    CodeBlock(const CodeBlock&) = delete;
    CodeBlock(Vector<UniquePtr<BaseAST>> children, SharedPtr<Scope>);
    CodeBlock& operator=(const CodeBlock&) = delete;
    void clear();
    ~CodeBlock() override;

    void addChild(UniquePtr<BaseAST> child);

    bool contains(const BaseAST* node) const;
    
    virtual SharedPtr<Scope> getCurrentScope() const {return getScope();} 

    const Vector<UniquePtr<BaseAST>>& getChildren() const;
    void printAST(std::ostream& os, int indent = 0) const override;

    AstType getAstType() const override { return AstType::CodeBlock; }
    String toString() const override;

    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;
    Node evaluate() const override {return evaluate(getScope());}
    SharedPtr<Scope> getScope() const override;
    bool containsReturnStatement() const;
    void setContainsReturnStatement(bool val) {containsReturn = val;}
    friend class CallableBody; 
    friend class FunctionBody;
    friend class ClassBody;
    friend class MethodBody;
    friend class ClassDef;
    friend class MethodDef;
    friend class CallableDef;

    UniquePtr<BaseAST> clone() const override;
    
    void setScope(SharedPtr<Scope> newScope) override;
    virtual FreeVars collectFreeVariables() const override;
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;

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
    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;
    Node evaluate() const override;
    AstType getAstType() const override {return AstType::Conditional;}

    UniquePtr<BaseAST> clone() const override;
    static UniquePtr<ConditionalBlock> create(UniquePtr<ASTStatement> condition, SharedPtr<Scope> scope) {
        return makeUnique<ConditionalBlock>(std::move(condition), scope);
    }
    void setScope(SharedPtr<Scope> newScope) override;
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;
};




class ElseStatement : public ASTStatement {
    mutable UniquePtr<CodeBlock> body;
public:
    explicit ElseStatement(UniquePtr<CodeBlock> body, SharedPtr<Scope> scope);

    UniquePtr<CodeBlock>& getBody(){return body;}
    UniquePtr<CodeBlock>& getBody() const {return body;}

    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;
    AstType getAstType() const override { return AstType::ElseStatement;}
    void printAST(std::ostream& os, int indent = 0) const override;

    UniquePtr<BaseAST> clone() const override;

    FreeVars collectFreeVariables() const override;

    void setScope(SharedPtr<Scope> newScope) override;
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;
};

class ElifStatement : public ASTStatement { // Also contains the logic of If, parser's job to ensure if is always first.

protected:
    mutable UniquePtr<CodeBlock> body;
public:
    UniquePtr<ConditionalBlock> condition;

    ElifStatement(UniquePtr<ASTStatement> condition, UniquePtr<CodeBlock> body, SharedPtr<Scope> scope);
    const ASTStatement* getCondition() const { return condition->getCondition(); }


    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;

    const UniquePtr<CodeBlock>& getBody() const { return body; }
    AstType getAstType() const override { return AstType::ElifStatement; }
    void printAST(std::ostream& os, int indent = 0) const override;    
    UniquePtr<BaseAST> clone() const override;
    FreeVars collectFreeVariables() const override;

    virtual void setScope(SharedPtr<Scope> newScope) override;
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;
};


// Originally IfStatement inherited from ElifStatement because if it quacks and walks like a duck...
// During some debugging it was made to be a standalone, the issue was not related to its derivative
// However, I haven't decided to make it inherit from Elif again, for actually no good reason, though it is a bit counterintuitive.
class IfStatement : public ASTStatement {
protected:
    mutable UniquePtr<CodeBlock> body;
    UniquePtr<ConditionalBlock> condition;
    Vector<UniquePtr<ElifStatement>> elifNodes; // Vector of `elif` nodes
    UniquePtr<ElseStatement> elseNode;          // Optional `else` block

public:


    IfStatement(UniquePtr<ASTStatement>condition, UniquePtr<CodeBlock> body, SharedPtr<Scope> scope);
    const ASTStatement* getCondition() const { return condition->getCondition(); }

    void addElifNode(UniquePtr<ElifStatement> elifNode) {
        if (!elifNode) {
            throw RunTimeError("Cannot add a null elif node to IfStatement.");
        }
            elifNodes.push_back(std::move(elifNode));
        }

    void setElseNode(UniquePtr<ElseStatement> elseNode) {this->elseNode = std::move(elseNode);}

    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;

    const Vector<UniquePtr<ElifStatement>>& getElifs() const {return elifNodes;}
    const UniquePtr<ElseStatement>& getElse() const {return elseNode;}

    AstType getAstType() const override { return AstType::IfStatement; }
    void printAST(std::ostream& os, int indent = 0) const override;

    UniquePtr<BaseAST> clone() const override;
    const UniquePtr<CodeBlock>& getBody() const { return body; }
    
    FreeVars collectFreeVariables() const override;
    
    void setScope(SharedPtr<Scope> newScope) override;
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;
};

class NoOpNode: public ASTStatement {
public:
    NoOpNode(SharedPtr<Scope> scope) : ASTStatement(scope) {}

    Node evaluate([[maybe_unused]] SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;
    String toString() const override {return "NoOpNode";}
    
    AstType getAstType() const override {return AstType::NoOp;};
    void printAST(std::ostream& os, int indent = 0) const override;
    UniquePtr<BaseAST> clone() const override;
}; 

// New base class for loops
class LoopBlock : public ASTStatement {
protected:
    UniquePtr<ConditionalBlock> condition; // Condition to check before each iteration
    UniquePtr<CodeBlock> body;             // The loop's body

public:
    explicit LoopBlock(UniquePtr<ConditionalBlock> condition, UniquePtr<CodeBlock> body, SharedPtr<Scope> scope);
    ~LoopBlock();
    
    const ConditionalBlock* getCondition() const { return condition.get(); }
    const CodeBlock* getBody() const {return body.get();}

    void printAST(std::ostream& os, int indent = 0) const override {(void)os; (void)indent; throw MerkError("Cannot Print a Base LoopBlock");};
 
    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;

    virtual AstType getAstType() const override {return AstType::LoopBlock;};
    virtual String toString() const override { return getAstTypeAsString();}

    void setScope(SharedPtr<Scope> newScope) override;
    virtual Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override = 0;
    virtual FreeVars collectFreeVariables() const override {return {};}

    

};

class WhileLoop : public LoopBlock {
public:
    WhileLoop(UniquePtr<ConditionalBlock> condition, UniquePtr<CodeBlock> body, SharedPtr<Scope> scope);
    ~WhileLoop();
    String toString() const override;

    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;

    void printAST(std::ostream& os, int indent = 0) const override;

    AstType getAstType() const override {return AstType::WhileLoop;};

    UniquePtr<BaseAST> clone() const override;

    FreeVars collectFreeVariables() const override;
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;
    void setScope(SharedPtr<Scope> newScope) override;

};

 


#endif //AST_CONTROL_H 