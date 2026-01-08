#ifndef EXCPETIONS_H
#define EXCPETIONS_H


#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <typeinfo>
#if defined(__GNUC__) && !defined(_WIN32)
    #include <cxxabi.h>
#endif
#include <optional>
#include <typeindex>

#include "ast/ast_base.h"

class BreakException : public std::exception {
public:
    const char* what() const noexcept override {
        return "Break statement executed.";
    }
};

class ContinueException : public std::exception {
public:
    const char* what() const noexcept override {
        return "Continue statement encountered.";
    }
};


class ThrowException : public std::exception {
private:
    Node exception;
public:
    explicit ThrowException(Node value) : exception(std::move(value)) {}

    const char* what() const noexcept override;

    Node getValue() const {return exception;}
};


class ReturnException : public std::exception {
private:
    Node returnValue;

public:
    explicit ReturnException(Node value) : returnValue(std::move(value)) {}

    const char* what() const noexcept override;

    Node getValue() const { return returnValue; }
};


class Continue : public ASTStatement {
public:
    explicit Continue(SharedPtr<Scope> scope);

    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;

    String toString() const override;
    void printAST(std::ostream& os, int indent = 0) const override;

    AstType getAstType() const override { return AstType::Continue; }
    UniquePtr<BaseAST> clone() const override;

};


class Break : public ASTStatement {
public:
    Break(SharedPtr<Scope> scope);

    String toString() const override {return "Break";}
    
    AstType getAstType() const override {return AstType::Break;}

    [[noreturn]] Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;

    void printAST(std::ostream& os, int indent = 0) const override;

    UniquePtr<BaseAST> clone() const override;
};



class Return : public ASTStatement {
private:
    UniquePtr<ASTStatement> returnValue;

public:
    explicit Return(SharedPtr<Scope> scope, UniquePtr<ASTStatement> value);
    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;
    String toString() const override;
    void printAST(std::ostream& os, int indent = 0) const override;
    AstType getAstType() const override { return AstType::Return; }
    UniquePtr<BaseAST> clone() const override;
    void setScope(SharedPtr<Scope> newScope) override;
    UniquePtr<ASTStatement>& getValue() {return returnValue;}
    ASTStatement* getValue() const {return returnValue.get();}
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;
    FreeVars collectFreeVariables() const override;

};


// ThrowStatement.h
class Throw : public ASTStatement {
private:
    UniquePtr<ASTStatement> expr;

public:
    Throw(UniquePtr<ASTStatement> value, SharedPtr<Scope> scope);
    [[noreturn]] Node evaluate(SharedPtr<Scope> scope, [[maybe_unused]]  SharedPtr<ClassInstanceNode> instance = nullptr) const override;
    String toString() const override;
    AstType getAstType() const override { return AstType::ThrowStatement; }
    UniquePtr<BaseAST> clone() const override;

    UniquePtr<ASTStatement>& getValue() {return expr;}
    ASTStatement* getValue() const {return expr.get();}
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;
    FreeVars collectFreeVariables() const override;
    void printAST(std::ostream& os, int indent = 0) const override;

};


// Should probably include the return exception here too


#endif //EXCPETIONS_H