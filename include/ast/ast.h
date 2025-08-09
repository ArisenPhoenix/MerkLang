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

#include "core/node/node.h"
#include "core/types.h"

#include "ast/ast_base.h"
#include "utilities/debugging_functions.h"




class Scope;

class LiteralValue : public ASTStatement {
public:
    UniquePtr<Node> val = nullptr;
    explicit LiteralValue(
        LitNode value, 
        SharedPtr<Scope> scope
    );

    ~LiteralValue();

    String toString() const override;

    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;
    void printAST(std::ostream& os, int indent = 0) const override;
    AstType getAstType() const override {return AstType::Literal;}
    UniquePtr<BaseAST> clone() const override;
    LitNode getValue();


private:
    LitNode value;
};

class VariableDeclaration : public ASTStatement {

private:
    String name;
    DataTypeFlags variableMeta;
    std::optional<NodeValueType> typeTag;
    ResolvedType type;
    UniquePtr<ASTStatement> valueExpression;
    

public:

    VariableDeclaration(
        String name,
        DataTypeFlags variableMeta,
        SharedPtr<Scope> scope,
        // ResolvedType type,
        UniquePtr<ASTStatement> valueExpression = nullptr
    );
    
    friend class AttributeDeclaration;
    virtual String toString() const override;

    String getName() const {return name;}
    DataTypeFlags getVariableTypes() {return variableMeta;}
    std::optional<NodeValueType> getTypeTag() {return typeTag;}
    const UniquePtr<ASTStatement>& getExpression() {return valueExpression;}
    const UniquePtr<ASTStatement>& getExpression() const {return valueExpression;}
    const ASTStatement* getRawExpression() const {return valueExpression.get();}
    void setScope(SharedPtr<Scope> newScope) override;

    virtual Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;
    virtual void printAST(std::ostream& os, int indent = 0) const override;
    virtual AstType getAstType() const override {return AstType::VariableDeclaration;}
    virtual UniquePtr<BaseAST> clone() const override;
    virtual Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;
    FreeVars collectFreeVariables() const override;

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
    virtual Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;
    virtual void printAST(std::ostream& os, int indent = 0) const override;
    virtual UniquePtr<BaseAST> clone() const override;

    FreeVars collectFreeVariables() const override;
    

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
    ASTStatement* getRawExpression() const {return valueExpression.get();}

    const String& getName() const { return name; }
    void setScope(SharedPtr<Scope> newScope);

    virtual void printAST(std::ostream& os, int indent = 0) const override;
    virtual Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;
    virtual AstType getAstType() const override {return AstType::VariableAssignment;}
    virtual UniquePtr<BaseAST> clone() const override;
    virtual Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;
    FreeVars collectFreeVariables() const override;


};

class BinaryOperation : public ASTStatement {
    String op;
    UniquePtr<ASTStatement> left;
    UniquePtr<ASTStatement> right;

public:
    BinaryOperation(const String& op, UniquePtr<ASTStatement> left, UniquePtr<ASTStatement> right, SharedPtr<Scope> scope);

    String toString() const override;
   
    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;

    AstType getAstType() const override {return AstType::BinaryOperation;}
    const String& getOperator() const { return op;}

    const ASTStatement* getLeftSide() const { return left.get(); }
    const ASTStatement* getRightSide() const { return right.get(); }

    void printAST(std::ostream& os, int indent = 0) const override;

    UniquePtr<BaseAST> clone() const override;
    void setScope(SharedPtr<Scope> newScope) override;
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;
    FreeVars collectFreeVariables() const override;


};

class UnaryOperation : public ASTStatement {
public:
    UnaryOperation(const String& op, UniquePtr<ASTStatement> operand, SharedPtr<Scope> scope);
    String toString() const override;
    const String& getOperator() const { return op; }
    const ASTStatement* getOperand() const { return operand.get(); }
    AstType getAstType() const override {return AstType::UnaryOperation;}

    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;
    void printAST(std::ostream& os, int indent = 0) const override;
    UniquePtr<BaseAST> clone() const override;
    void setScope(SharedPtr<Scope> newScope) override;
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;


private:
    String op; // Operator (e.g., "!")
    UniquePtr<ASTStatement> operand;
};








 

        

#endif // AST_H

 






