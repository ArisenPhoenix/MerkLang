#include "core/types.h"
#include "core/errors.h"
#include "ast/AstBase.hpp"
#include "ast/Ast.hpp"
#include "ast/Exceptions.hpp"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"
#include "utilities/helper_functions.h"



const char* ThrowException::what() const noexcept {
    return "Throw statement encountered.";
}

const char* ReturnException::what() const noexcept {
    return "Return statement encountered";
}


Throw::Throw(UniquePtr<ASTStatement> value, SharedPtr<Scope> scope)
    : ASTStatement(scope), expr(std::move(value)) {}

Node Throw::evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> InstanceNode) const {
    
    if (!expr) { throw MerkError("Throw Exception"); }
    
    Node err = expr->evaluate(scope, InstanceNode);
    throw ThrowException(err);
}

EvalResult Throw::evaluateFlow(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance) const {
    if (!expr) throw MerkError("Throw statement must have a value.");
    Node err = expr->evaluate(scope, instance);
    return EvalResult::Throw(std::move(err));
}


Node Return::evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) const  {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!returnValue) { throw MerkError("Return statement must have a value."); }
    auto value = returnValue->evaluate(scope, instanceNode);

    DEBUG_FLOW_EXIT();
    throw ReturnException(value);

}


UniquePtr<BaseAST> Throw::clone() const {
    UniquePtr<BaseAST> clonedThrowBase = expr->clone();

    auto clonedExpr = static_unique_ptr_cast<ASTStatement>(std::move(clonedThrowBase));

    return makeUnique<Throw>(std::move(clonedExpr), getScope());
}




Return::Return(SharedPtr<Scope> scope, UniquePtr<ASTStatement> value)
: ASTStatement(scope), returnValue(std::move(value)) {}
EvalResult Return::evaluateFlow(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!returnValue) { throw MerkError("Return statement must have a value."); }
    Node value = returnValue->evaluate(scope, instance);
    DEBUG_FLOW_EXIT();
    return EvalResult::Return(std::move(value));
}




Break::Break(SharedPtr<Scope> scope) : ASTStatement(scope) {}
EvalResult Break::evaluateFlow(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance) const {
    MARK_UNUSED_MULTI(scope, instance);
    return EvalResult::Break();
}

Continue::Continue(SharedPtr<Scope> scope) : ASTStatement(scope) {}

EvalResult Continue::evaluateFlow(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance) const {
    MARK_UNUSED_MULTI(scope, instance);
    return EvalResult::Continue();
}
