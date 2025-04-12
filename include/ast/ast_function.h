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
};



class FunctionDef : public CallableDef {
public:
    friend class MethodDef;
    FunctionDef(String name, ParamList parameters, UniquePtr<FunctionBody> body, CallableType funcType, SharedPtr<Scope> scope);
    ~FunctionDef() = default;
    virtual Node evaluate(SharedPtr<Scope> scope) const override;
    virtual AstType getAstType() const override { return AstType::FunctionDefinition;}
    virtual UniquePtr<BaseAST> clone() const override;
    void printAST(std::ostream& os, int indent = 0) const override;

};
    
    
class FunctionCall : public CallableCall {
public:
    FunctionCall(String functionName, Vector<UniquePtr<ASTStatement>> args, SharedPtr<Scope> scope);

    virtual Node evaluate(SharedPtr<Scope> scope) const override;
    virtual AstType getAstType() const override { return AstType::FunctionCall;}
    virtual void printAST(std::ostream& os, int indent = 0) const override;
    virtual UniquePtr<BaseAST> clone() const override;
    // String toString() const override;

};

class FunctionRef : public CallableRef {
public:
    CallableType funcType;
    friend class MethodDef;
    FunctionRef(String name, SharedPtr<Scope> scope);
    virtual Node evaluate(SharedPtr<Scope> scope) const override;
    // virtual Node evaluate() const override { return evaluate(getScope());}
    virtual AstType getAstType() const override { return AstType::FunctionReference;}
    // virtual CallableSignature toCallableSignature() const;

    // virtual UniquePtr<BaseAST> clone() const override;
    virtual void printAST(std::ostream& os, int indent = 0) const override;
    // UniquePtr<FunctionBody> getFunctionBody() {return static_unique_ptr_cast<FunctionBody>(std::move(getBody()));}
    virtual UniquePtr<BaseAST> clone() const override;

};
    

#endif //AST_FUNCTION_H
