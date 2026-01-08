#pragma once

#include "core/TypesFWD.hpp"
#include "ast/AstBase.hpp"
#include "utilities/helper_functions.h"


enum class ChainOpKind {
    Reference,   // Just evaluates to a value (like a getter)
    Assignment,  // Modifies an existing field
    Declaration  // Defines a new field (possibly with const/static)
};

String opKindAsString(ChainOpKind opKind);

// Obsolete but will keep it for now.

enum class ResolutionMode {
    Normal,
    ClassInstance,
    BaseClass,
    Global,
    Module,
};


struct ChainElement {
    String name;
    UniquePtr<BaseAST> object;
    String delimiter;
    TokenType type;
    ChainElement();

    ChainElement(ChainElement&& other) noexcept;
    ~ChainElement();
    void clear();

    ChainElement& operator=(ChainElement&& other) noexcept;
    ChainElement(const ChainElement&) = delete;
    ChainElement& operator=(const ChainElement&) = delete;
    void printAST(std::ostream& os, int indent = 0) const;
};


class Chain : public ASTStatement {
private:
    Vector<ChainElement> elements;
    WeakPtr<Scope> classScope; //  set when evaluating in class
    int resolutionStartIndex = 0;  // default is 0, resolve from the beginning
    ResolutionMode mode = ResolutionMode::Normal;
    mutable SharedPtr<Scope> lastScope;


public:
    explicit Chain(SharedPtr<Scope> scope);
    ~Chain();
    void clear();
    void addElement(ChainElement&& elem);
    void replaceLastElementWith(ChainElement&& elem);
    void setResolutionStartIndex(int index);

    const Vector<ChainElement>& getElements() const;

    AstType getAstType() const override {return AstType::Chain;}
    void printAST(std::ostream& os, int indent = 0) const override;

    int getResolutionStartIndex() const;
    ResolutionMode getResolutionMode() const;
    SharedPtr<Scope> getSecondaryScope() const;

    virtual String toString() const override;
    
    virtual Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;
    Node evaluate() const {return evaluate(getScope());}

    void setSecondaryScope(SharedPtr<Scope> secondary);
    void setResolutionMode(ResolutionMode newMode);
    
    virtual UniquePtr<BaseAST> clone() const override;
    Vector<const BaseAST*> getAllAst(bool includeSelf) const override;
    void setScope(SharedPtr<Scope> scope) override;
    Vector<ChainElement>& getMutableElements();
    ChainElement& getLast();
    ChainElement& getElement(int index);

    void setFirstElement(UniquePtr<ASTStatement> probablyScope, String name);

    SharedPtr<Scope> getLastScope();
    SharedPtr<Scope> getLastScope() const;
    void setLastScope(SharedPtr<Scope> mostRecentScope) const;
    FreeVars collectFreeVariables() const override;
    
};


class ChainOperation : public ASTStatement {
private:
    UniquePtr<Chain> lhs;
    UniquePtr<ASTStatement> rhs; // Nullable for Reference
    ChainOpKind opKind;
    bool isConst = false;
    bool isMutable = true;
    bool isStatic = false;
    WeakPtr<Scope> classScope;
    int resolutionStartIndex = 0;
    ResolutionMode resolutionMode = ResolutionMode::Normal;
    
public:
    ChainOperation(UniquePtr<Chain> lhs,
                   UniquePtr<ASTStatement> rhs,
                   ChainOpKind opKind,
                   SharedPtr<Scope> scope,
                   bool isConst = false,
                   bool isMutable = true,
                   bool isStatic = false
                );
 
    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;
    AstType getAstType() const override { return AstType::ChainOperation; }
    String toString() const override;
    void printAST(std::ostream& os, int indent = 0) const override;
    UniquePtr<BaseAST> clone() const override;
    Vector<const BaseAST*> getAllAst(bool includeSelf = false) const override;

    UniquePtr<Chain>& getLeft();
    UniquePtr<ASTStatement>& getRight();
    Chain* getLeftSide() const;
    ASTStatement* getRightSide() const; 

    ChainOpKind getOpKind() const { return opKind; }
    bool getIsConst() const { return isConst; }
    bool getIsMutable() const { return isMutable; }

    SharedPtr<Scope> getSecondaryScope();
    ResolutionMode getResolutionMode();
    void setScope(SharedPtr<Scope> scope) override;

    void setSecondaryScope(SharedPtr<Scope> scope); 

    void setResolutionMode(ResolutionMode newMode, String accessor);
    void setResolutionStartIndex(int index, String accessor);
    void setResolutionMethod(int index, ResolutionMode newMode, SharedPtr<Scope> newScope, String accessor);
    FreeVars collectFreeVariables() const override;

};