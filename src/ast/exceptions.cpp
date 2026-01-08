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

// ThrowStatement.h

Throw::Throw(UniquePtr<ASTStatement> value, SharedPtr<Scope> scope)
    : ASTStatement(scope), expr(std::move(value)) {}

Node Throw::evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> InstanceNode) const {
    
    if (!expr) { throw MerkError("Throw Exception"); }
    
    Node err = expr->evaluate(scope, InstanceNode);

    // Can perform some other handling for constructing the message


    throw ThrowException(err);
}


Node Return::evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) const  {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    
    if (!returnValue) {throw MerkError("Return statement must have a value.");}

    auto value = returnValue->evaluate(scope, instanceNode);

    // DEBUG_LOG(LogLevel::PERMISSIVE, "Value after return evaluation: ", value, "Type: ", value.getFlags().toString());

    DEBUG_FLOW_EXIT();
    throw ReturnException(value);  // Immediately exit function with value

}


UniquePtr<BaseAST> Throw::clone() const {
    UniquePtr<BaseAST> clonedReturnBase = expr->clone();

    auto clonedExpr = static_unique_ptr_cast<ASTStatement>(std::move(clonedReturnBase));

    return makeUnique<Throw>(std::move(clonedExpr), getScope());
}


Return::Return(SharedPtr<Scope> scope, UniquePtr<ASTStatement> value)
: ASTStatement(scope), returnValue(std::move(value)) {}




Break::Break(SharedPtr<Scope> scope) : ASTStatement(scope) {}


Continue::Continue(SharedPtr<Scope> scope) : ASTStatement(scope) {}