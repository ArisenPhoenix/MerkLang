#ifndef AST_FUNCTION_H
#define AST_FUNCTION_H

#include <unordered_set>
#include "utilities/debugger.h"
#include "ast/ast.h"
#include "core/callables/functions/function.h"


class Scope;

class FunctionBody : public CallableBody {
public:
    friend class MethodBody;
    FunctionBody(SharedPtr<Scope> scope);

    // Steal Constructor
    FunctionBody(UniquePtr<CodeBlock>&& block);

    ~FunctionBody();

    virtual Node evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;
    virtual UniquePtr<BaseAST> clone() const override;
    virtual AstType getAstType() const override { return AstType::FunctionBlock;}    
    void printAST(std::ostream& os, int indent = 0) const override;
    Vector<UniquePtr<BaseAST>>& getChildren(){return children;};
};

class FunctionDef : public CallableDef {
public:
    friend class MethodDef;
    FunctionDef(String name, ParamList parameters, UniquePtr<FunctionBody> body, CallableType funcType, SharedPtr<Scope> scope);
    ~FunctionDef() = default;
    virtual Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;
    virtual AstType getAstType() const override { return AstType::FunctionDefinition;}
    virtual UniquePtr<BaseAST> clone() const override;
    void printAST(std::ostream& os, int indent = 0) const override;

};
    
    
class FunctionCall : public CallableCall {
public:
    FunctionCall(String functionName, UniquePtr<ArgumentType> args, SharedPtr<Scope> scope);

    virtual Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;
    virtual AstType getAstType() const override { return AstType::FunctionCall;}
    virtual void printAST(std::ostream& os, int indent = 0) const override;
    virtual UniquePtr<BaseAST> clone() const override;

};

class FunctionRef : public CallableRef {
public:
    CallableType funcType;
    friend class MethodDef;
    FunctionRef(String name, SharedPtr<Scope> scope);
    virtual Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;
    virtual AstType getAstType() const override { return AstType::FunctionReference;}
    virtual void printAST(std::ostream& os, int indent = 0) const override;
    virtual UniquePtr<BaseAST> clone() const override;

};
    

#endif //AST_FUNCTION_H
