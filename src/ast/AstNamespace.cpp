#include "ast/AstNamespace.hpp"

AstNamespace::AstNamespace(String name) {
    MARK_UNUSED_MULTI(name);
}

AstNamespace::~AstNamespace() {}

Node AstNamespace::evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) const {
    MARK_UNUSED_MULTI(scope, instanceNode);
    return Node();
}

void AstNamespace::printAST(std::ostream& os, int indent) const {
    MARK_UNUSED_MULTI(os, indent);
    
}