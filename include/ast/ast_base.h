
#ifndef AST_BASE_H
#define AST_BASE_H

#include <string> 
#include "core/types.h"
#include "core/node.h"
#include "core/errors.h"
#include <ostream>
#include <variant>

class Scope;

// These templates are specific to the AST structures, so it is reasonable to only include them here, rather than types.h

template <typename T, typename U>
UniquePtr<T> static_unique_ptr_cast(UniquePtr<U>&& ptr) {
    return UniquePtr<T>(static_cast<T*>(ptr.release()));
}


template <typename T, typename U>
UniquePtr<T> dynamic_unique_ptr_cast(UniquePtr<U>&& ptr) {
    if (T* raw = dynamic_cast<T*>(ptr.get())) {
        ptr.release();
        return UniquePtr<T>(raw);
    }
    throw std::runtime_error("dynamic_unique_ptr_cast failed");
}


template <typename T, typename U>
UniquePtr<T> static_shared_ptr_cast(SharedPtr<U>&& ptr) {
    return SharedPtr<T>(static_cast<T*>(ptr.release()));
}


class BaseAST {
protected:
    WeakPtr<Scope> scope;
    String branch = "Base";

public:
    BaseAST(SharedPtr<Scope> scope = nullptr);
    virtual ~BaseAST() = default;

    // virtual String toString() const {return "BaseAST";}
    virtual String toString() const = 0;

    // Virtual method for printing the AST node (for debugging purposes)
    virtual void printAST(std::ostream& os, int indent = 0) const {
        (void)indent;
        os << highlight("BaseAST", Colors::red) << "\n";
    }

    virtual bool contains(const BaseAST* node) const {
        (void)node;
        return false; // Default behavior
    }
    
    virtual AstType getAstType() const {return AstType::Base;}
    String getAstTypeAsString() const {return astTypeToString(getAstType());}
    
    virtual void setScope(SharedPtr<Scope> newScope);

    SharedPtr<Scope> getScope() const {
        auto locked = scope.lock();
        return locked;
    }
    virtual Vector<const BaseAST*> getAllAst(bool includeSelf = true) const;
    virtual Node evaluate(SharedPtr<Scope> scope) const = 0;
    virtual Node evaluate() const = 0;

    virtual UniquePtr<BaseAST> clone() const = 0;
    String getBranch() const {return branch;}

    // Vector<const BaseAST*> BaseAST::getAllAst(std::function<bool(const BaseAST*)> filter, bool includeSelf) const {
    //     Vector<const BaseAST*> result;
    
    //     if (includeSelf && filter(this)) {
    //         result.push_back(this);
    //     }
    
    //     for (const auto& child : getChildren()) {
    //         auto subResults = child->getAllAst(filter, true);  // recurse
    //         result.insert(result.end(), subResults.begin(), subResults.end());
    //     }
    
    //     return result;
    // }
};





class ASTStatement : public BaseAST {
public:
    explicit ASTStatement(SharedPtr<Scope> scope) : BaseAST(scope) {branch = "AST";}
    ~ASTStatement() override = default;

    virtual void printAST(std::ostream& os, int indent = 0) const {
        // This one should throw an error, but it isn't clear yet if it still has any uses
        // Originally this was the BaseClass, but when needing to implement the CodeBlock, it became apparent
        // that a further base was needed. So this could very well be an unnecessary relic as well.
        (void)indent;
        os << highlight("ASTStatement", Colors::red) << "\n";
    }
    virtual AstType getAstType() const override {return AstType::AST;}
    virtual String toString() const override {return getAstTypeAsString();}

    virtual Node evaluate(SharedPtr<Scope> scope) const override= 0;

    virtual Node evaluate() const override {return evaluate(getScope());}

    virtual UniquePtr<BaseAST> clone() const override;

    virtual Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;

};

#endif // AST_BASE_H