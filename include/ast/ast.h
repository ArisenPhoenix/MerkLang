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


class FunctionNode;
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

    String getAstTypeAsString() const {return astTypeToString(getAstType());}
    Node evaluate(SharedPtr<Scope> scope) const override;
    Node evaluate() const override {return evaluate(getScope());}
    void printAST(std::ostream& os, int indent = 0) const override;
    AstType getAstType() const override {return AstType::Literal;}
    UniquePtr<BaseAST> clone() const override {
        return std::make_unique<LiteralValue>(value, getScope(), _isString, _isBool);
    }

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
        UniquePtr<ASTStatement> valueNode = nullptr
    );

    String toString() const override;

    String getAstTypeAsString() const {return astTypeToString(getAstType());}
    Node evaluate(SharedPtr<Scope> scope) const override;
    Node evaluate() const override {return evaluate(getScope());}
    void printAST(std::ostream& os, int indent = 0) const override;
    AstType getAstType() const override {return AstType::VariableDeclaration;}
    String getName() const {return name;}
    UniquePtr<BaseAST> clone() const override;  
    void setScope(SharedPtr<Scope> newScope) override;
};

class VariableReference : public ASTStatement {
    String name;

public:
    explicit VariableReference(const String& name, SharedPtr<Scope> scope);

    String toString() const override;

    AstType getAstType() const override {
        return AstType::VariableReference;
    }

    const String& getName() const { return name; }

    Node evaluate(SharedPtr<Scope> scope) const;
    Node evaluate() const override {return evaluate(getScope());}
    String getAstTypeAsString() const {return astTypeToString(getAstType());}
    void printAST(std::ostream& os, int indent = 0) const override;
    UniquePtr<BaseAST> clone() const override;
    void setScope(SharedPtr<Scope> newScope) override;

};



class VariableAssignment : public ASTStatement {
private:
    String name;
    UniquePtr<ASTStatement> valueExpression;

public:
    VariableAssignment(String name, UniquePtr<ASTStatement> value, SharedPtr<Scope> scope);

    String toString() const override;
    void printAST(std::ostream& os, int indent = 0) const override;

    Node evaluate(SharedPtr<Scope> scope) const override;
    Node evaluate() const override {return evaluate(getScope());}

    friend class ParameterAssignment;
    const UniquePtr<ASTStatement>& getValue() const {return valueExpression;}
    AstType getAstType() const override {return AstType::VariableAssignment;}
    String getAstTypeAsString() const override {return astTypeToString(getAstType());}

    UniquePtr<BaseAST> clone() const override;
    const String& getName() const { return name; }
    void setScope(SharedPtr<Scope> newScope);

};

class BinaryOperation : public ASTStatement {
    String op;
    UniquePtr<ASTStatement> left;
    UniquePtr<ASTStatement> right;

public:
    BinaryOperation(const String& op, UniquePtr<ASTStatement> left, UniquePtr<ASTStatement> right, SharedPtr<Scope> scope);

    String toString() const override;
   
    Node evaluate(SharedPtr<Scope> scope) const override;
    Node evaluate() const override {return evaluate(getScope());}

    AstType getAstType() const override {return AstType::BinaryOperation;}
    String getAstTypeAsString() const {return astTypeToString(getAstType());}

    const String& getOperator() const { return op;}

    const ASTStatement* getLeftSide() const { return left.get(); }
    const ASTStatement* getRightSide() const { return right.get(); }

    void printAST(std::ostream& os, int indent = 0) const override;

    UniquePtr<BaseAST> clone() const override;
    void setScope(SharedPtr<Scope> newScope) override;

};

class UnaryOperation : public ASTStatement {
public:
    UnaryOperation(const String& op, UniquePtr<ASTStatement> operand, SharedPtr<Scope> scope);
    String toString() const override;
    const String& getOperator() const { return op; }
    const ASTStatement* getOperand() const { return operand.get(); }
    AstType getAstType() const override {
        return AstType::BinaryOperation;
    }

    Node evaluate(SharedPtr<Scope> scope) const override;
    Node evaluate() const override {return evaluate(getScope());}

    void printAST(std::ostream& os, int indent = 0) const override;
    UniquePtr<BaseAST> clone() const override;
    void setScope(SharedPtr<Scope> newScope) override;


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
    String getAstTypeAsString() const override {return astTypeToString(getAstType());}

    [[noreturn]] Node evaluate(SharedPtr<Scope> scope) const override;
    [[noreturn]] Node evaluate() const override;

    void printAST(std::ostream& os, int indent = 0) const override;

    UniquePtr<BaseAST> clone() const override;
};


class ReturnException : public std::exception {
    private:
        Node returnValue;
    
    public:
        explicit ReturnException(Node value) : returnValue(std::move(value)) {}
    
        const char* what() const noexcept override {
            return "Return statement encountered.";
        }
    
        Node getValue() const { return returnValue; }
};


class Return : public ASTStatement {
private:
    UniquePtr<ASTStatement> returnValue;

public:
    explicit Return(SharedPtr<Scope> scope, UniquePtr<ASTStatement> value);

    Node evaluate(SharedPtr<Scope> scope) const override;
    Node evaluate() const override {return evaluate(getScope());}

    String toString() const;
    void printAST(std::ostream& os, int indent = 0) const override;

    AstType getAstType() const override { return AstType::Return; }
    String getAstTypeAsString() const { return astTypeToString(getAstType()); }

    UniquePtr<BaseAST> clone() const override;
    void setScope(SharedPtr<Scope> newScope) override;

};


#endif // AST_H








