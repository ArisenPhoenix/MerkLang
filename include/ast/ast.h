#ifndef AST_H
#define AST_H

#include <memory>
#include <string> 
#include <vector>
#include <unordered_map>
#include <variant>
#include <typeinfo>
#include <cxxabi.h>
#include <optional>
#include <typeindex>

#include "core/types.h"
#include "ast/ast_base.h"
#include "utilities/debugging_functions.h"


class Scope;

class LiteralValue : public ASTStatement {
public:
    explicit LiteralValue(
        LitNode value, 
        SharedPtr<Scope> scope,
        bool isString,
        bool isBool
    );

    String toString() const override;

    Node evaluate(SharedPtr<Scope> scope) const override;
    void printAST(std::ostream& os, int indent = 0) const override;
    AstType getAstType() const override {return AstType::Literal;}
    UniquePtr<BaseAST> clone() const override;
    // virtual Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;


private:
    LitNode value;
    bool _isString;
    bool _isBool;
};

class VariableDeclaration : public ASTStatement {

private:
    String name;
    VarNode variable;
    std::optional<std::type_index> typeTag;
    UniquePtr<ASTStatement> valueExpression;

public:
    VariableDeclaration(
        String name,
        VarNode variable,
        SharedPtr<Scope> scope,
        std::optional<std::type_index> typeTag = std::nullopt,
        UniquePtr<ASTStatement> valueExpression = nullptr
    );
    VariableDeclaration(UniquePtr<VariableDeclaration> varDec);
    friend class AttributeDeclaration;
    virtual String toString() const override;

    String getName() const {return name;}
    VarNode getVariable() {return variable;}
    std::optional<std::type_index> getTypeTag() {return typeTag;}
    const UniquePtr<ASTStatement>& getExpression() {return valueExpression;}
    const UniquePtr<ASTStatement>& getExpression() const {return valueExpression;}
    void setScope(SharedPtr<Scope> newScope) override;

    virtual Node evaluate(SharedPtr<Scope> scope) const override;
    virtual void printAST(std::ostream& os, int indent = 0) const override;
    virtual AstType getAstType() const override {return AstType::VariableDeclaration;}
    virtual UniquePtr<BaseAST> clone() const override;
    virtual Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;

};

class VariableReference : public ASTStatement {
    String name;

public:
    explicit VariableReference(const String& name, SharedPtr<Scope> scope);
    explicit VariableReference(UniquePtr<VariableReference> varRef);

    const String& getName() const { return name; }
    void setScope(SharedPtr<Scope> newScope) override;
    String toString() const override;

    virtual AstType getAstType() const override {return AstType::VariableReference;}
    virtual Node evaluate(SharedPtr<Scope> scope) const override;
    virtual void printAST(std::ostream& os, int indent = 0) const override;
    virtual UniquePtr<BaseAST> clone() const override;
    

};



class VariableAssignment : public ASTStatement {
private:
    String name;
    UniquePtr<ASTStatement> valueExpression;

public:
    VariableAssignment(String name, UniquePtr<ASTStatement> value, SharedPtr<Scope> scope);
    VariableAssignment(UniquePtr<VariableAssignment> varAssign);

    String toString() const override;
    
    friend class ParameterAssignment;
    const UniquePtr<ASTStatement>& getExpression() const {return valueExpression;}

    const String& getName() const { return name; }
    void setScope(SharedPtr<Scope> newScope);

    virtual void printAST(std::ostream& os, int indent = 0) const override;
    virtual Node evaluate(SharedPtr<Scope> scope) const override;
    virtual AstType getAstType() const override {return AstType::VariableAssignment;}
    virtual UniquePtr<BaseAST> clone() const override;
    virtual Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;


};

class BinaryOperation : public ASTStatement {
    String op;
    UniquePtr<ASTStatement> left;
    UniquePtr<ASTStatement> right;

public:
    BinaryOperation(const String& op, UniquePtr<ASTStatement> left, UniquePtr<ASTStatement> right, SharedPtr<Scope> scope);

    String toString() const override;
   
    Node evaluate(SharedPtr<Scope> scope) const override;

    AstType getAstType() const override {return AstType::BinaryOperation;}
    const String& getOperator() const { return op;}

    const ASTStatement* getLeftSide() const { return left.get(); }
    const ASTStatement* getRightSide() const { return right.get(); }

    void printAST(std::ostream& os, int indent = 0) const override;

    UniquePtr<BaseAST> clone() const override;
    void setScope(SharedPtr<Scope> newScope) override;
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;


};

class UnaryOperation : public ASTStatement {
public:
    UnaryOperation(const String& op, UniquePtr<ASTStatement> operand, SharedPtr<Scope> scope);
    String toString() const override;
    const String& getOperator() const { return op; }
    const ASTStatement* getOperand() const { return operand.get(); }
    AstType getAstType() const override {return AstType::BinaryOperation;}

    Node evaluate(SharedPtr<Scope> scope) const override;
    void printAST(std::ostream& os, int indent = 0) const override;
    UniquePtr<BaseAST> clone() const override;
    void setScope(SharedPtr<Scope> newScope) override;
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;


private:
    String op; // Operator (e.g., "!")
    UniquePtr<ASTStatement> operand;
};


class Break : public ASTStatement {
public:
    Break(SharedPtr<Scope> scope);

    String toString() const override {
        return "Break";
    }
    
    AstType getAstType() const override {return AstType::Break;}

    [[noreturn]] Node evaluate(SharedPtr<Scope> scope) const override;

    void printAST(std::ostream& os, int indent = 0) const override;

    UniquePtr<BaseAST> clone() const override;
};





class Return : public ASTStatement {
private:
    UniquePtr<ASTStatement> returnValue;

public:
    explicit Return(SharedPtr<Scope> scope, UniquePtr<ASTStatement> value);
    Node evaluate(SharedPtr<Scope> scope) const override;
    String toString() const;
    void printAST(std::ostream& os, int indent = 0) const override;
    AstType getAstType() const override { return AstType::Return; }
    UniquePtr<BaseAST> clone() const override;
    void setScope(SharedPtr<Scope> newScope) override;
    UniquePtr<ASTStatement>& getValue() {return returnValue;}
    // UniquePtr<ASTStatement> getReturnValue() const {return returnValue;}
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;

};

 

class Continue : public ASTStatement {
public:
    explicit Continue(SharedPtr<Scope> scope);

    Node evaluate(SharedPtr<Scope> scope) const override;

    String toString() const override;
    void printAST(std::ostream& os, int indent = 0) const override;

    AstType getAstType() const override { return AstType::Continue; }
    UniquePtr<BaseAST> clone() const override;
    // Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;

};
        

#endif // AST_H

 






