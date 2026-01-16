#pragma once

#include "core/TypesFWD.hpp"
#include "utilities/debugger.h"       // For DEBUG_LOG
#include "ast/AstBase.hpp"             // For ASTStatement, printIndent, etc.
#include "ast/Ast.hpp"
#include "ast/AstControl.hpp"
#include "ast/AstFunction.hpp"

#include "core/errors.h"              // For error types

class AstNamespace : public BaseAST {
public:
    // Constructor: capture the class name and the defining scope.
    AstNamespace(String name);
    ~AstNamespace();

    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

    // Print the AST for debugging.
    void printAST(std::ostream& os, int indent = 0) const override;
    AstType getAstType() const override {return AstType::Namespace;}
};