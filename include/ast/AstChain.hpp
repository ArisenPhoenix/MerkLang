#pragma once

#include "core/TypesFWD.hpp"
#include "ast/AstBase.hpp"
#include "utilities/helper_functions.h"


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