#pragma once

#include "core/TypesFWD.hpp"
#include "utilities/debugger.h"
#include "ast/AstBase.hpp"
#include "ast/Ast.hpp"
#include "ast/AstControl.hpp"
#include "ast/AstFunction.hpp"

#include "core/errors.h" 

class AstNamespace : public BaseAST {
public:
    AstNamespace(String name);
    ~AstNamespace();

    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

    void printAST(std::ostream& os, int indent = 0) const override;
    AstType getAstType() const override {return AstType::Namespace;}
};