#include "core/types.h"
#include "core/errors.h"
#include "utilities/debugger.h"
#include "ast/ast_base.h"
#include "ast/ast.h"

#include "ast/ast_control.h"
#include "ast/ast_function.h"
#include "ast/ast_callable.h"

UniquePtr<BaseAST> ASTStatement::clone() const {
    throw MerkError("Cannot Clone Base Class ASTStatement");
} 

UniquePtr<BaseAST> LiteralValue::clone() const {
    return std::make_unique<LiteralValue>(value, getScope(), _isString, _isBool);
}

UniquePtr<BaseAST> VariableDeclaration::clone() const {
    UniquePtr<ASTStatement> valueInfo = nullptr;
    if (valueExpression){
        auto valueInfoBase = valueExpression->clone();
        valueInfo = static_unique_ptr_cast<ASTStatement>(std::move(valueInfoBase));
    }
    
    return makeUnique<VariableDeclaration>(name, variable, getScope(), typeTag, std::move(valueInfo));
}

UniquePtr<BaseAST> VariableReference::clone() const {

    return makeUnique<VariableReference>(name, getScope());
} 

UniquePtr<BaseAST> VariableAssignment::clone() const {
    UniquePtr<BaseAST> clonedValBase = valueExpression->clone();
    auto clonedVal = static_unique_ptr_cast<ASTStatement>(std::move(clonedValBase));
    return makeUnique<VariableAssignment>(name, std::move(clonedVal), getScope());
} 

UniquePtr<BaseAST> BinaryOperation::clone() const {
    UniquePtr<BaseAST> clonedLeftBase = left->clone();
    auto clonedLeft = static_unique_ptr_cast<ASTStatement>(std::move(clonedLeftBase));

    UniquePtr<BaseAST> clonedRightBase = right->clone();
    auto clonedRight = static_unique_ptr_cast<ASTStatement>(std::move(clonedRightBase));


    return makeUnique<BinaryOperation>(op, std::move(clonedLeft), std::move(clonedRight), getScope());
} 

UniquePtr<BaseAST> UnaryOperation::clone() const {
    UniquePtr<BaseAST> clonedOperandBase = operand->clone();
    auto clonedOperand = static_unique_ptr_cast<ASTStatement>(std::move(clonedOperandBase));
    return makeUnique<UnaryOperation>(op, std::move(clonedOperand), getScope());
} 

UniquePtr<BaseAST> NoOpNode::clone() const {return makeUnique<NoOpNode>(getScope());}


// AST CONTROL
UniquePtr<BaseAST> CodeBlock::clone() const {
    // Create a new CodeBlock with the same scope (or a copy of it, as appropriate)
    UniquePtr<CodeBlock> newBlock = makeUnique<CodeBlock>(getScope());

    for (const auto &child : children) {
        newBlock->addChild(child->clone());
    }
    return newBlock;
}

UniquePtr<BaseAST> Break::clone() const {
    return makeUnique<Break>(getScope());
}

UniquePtr<BaseAST> Return::clone() const {
    UniquePtr<BaseAST> clonedReturnBase = returnValue->clone();
    auto clonedReturn = static_unique_ptr_cast<ASTStatement>(std::move(clonedReturnBase));
    return makeUnique<Return>(getScope(), std::move(clonedReturn));
} 

UniquePtr<BaseAST> ConditionalBlock::clone() const {
    UniquePtr<BaseAST> clonedCondBase = condition->clone();
    auto clonedCond = static_unique_ptr_cast<ConditionalBlock>(std::move(clonedCondBase));
    return clonedCond;
}

UniquePtr<BaseAST> ElseStatement::clone() const {
    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    auto clonedBody = dynamic_unique_ptr_cast<CodeBlock>(std::move(clonedBodyBase));
    return makeUnique<ElseStatement>(std::move(clonedBody), getScope());
}

UniquePtr<BaseAST> ElifStatement::clone() const {
    UniquePtr<BaseAST> clonedCondBase = condition->clone();
    auto clonedCond = static_unique_ptr_cast<ConditionalBlock>(std::move(clonedCondBase));

    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    auto clonedBody = dynamic_unique_ptr_cast<CodeBlock>(std::move(clonedBodyBase));
    return makeUnique<ElifStatement>(std::move(clonedCond), std::move(clonedBody), getScope());
}

UniquePtr<BaseAST> IfStatement::clone() const {
    // Clone the condition and downcast to the expected type.
    UniquePtr<BaseAST> clonedCondBase = condition->clone();
    auto clonedCondition = static_unique_ptr_cast<ConditionalBlock>(std::move(clonedCondBase));

    // Clone the body and downcast to CodeBlock.
    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    auto clonedBody = dynamic_unique_ptr_cast<CodeBlock>(std::move(clonedBodyBase));

    // Construct a new IfStatement with the downcasted condition and body.
    auto clonedIf = makeUnique<IfStatement>(std::move(clonedCondition), std::move(clonedBody), getScope());

    // Clone each ElifStatement and add them.
    for (const auto &elifNode : elifNodes) {
        UniquePtr<BaseAST> clonedElifBase = elifNode->clone();
        auto clonedElif = static_unique_ptr_cast<ElifStatement>(std::move(clonedElifBase));
        clonedIf->addElifNode(std::move(clonedElif));
    }

    // If an else node exists, clone it and set it.
    if (elseNode) {
        UniquePtr<BaseAST> clonedElseBase = elseNode->clone();
        auto clonedElse = static_unique_ptr_cast<ElseStatement>(std::move(clonedElseBase));
        clonedIf->setElseNode(std::move(clonedElse));
    }

    // Return the newly constructed IfStatement clone.
    return clonedIf;
}

UniquePtr<BaseAST> WhileLoop::clone() const {
    UniquePtr<BaseAST> clonedCondBase = condition->clone();
    auto clonedCond = static_unique_ptr_cast<ConditionalBlock>(std::move(clonedCondBase));

    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    auto clonedBody = dynamic_unique_ptr_cast<CodeBlock>(std::move(clonedBodyBase));

    return makeUnique<WhileLoop>(std::move(clonedCond), std::move(clonedBody), getScope());
}


// AST FUNCTIONS
UniquePtr<BaseAST> FunctionBody::clone() const {
    // Create a new CodeBlock with the same scope (or a copy of it, as appropriate)
    UniquePtr<FunctionBody> newBlock = std::make_unique<FunctionBody>(getScope());

    // Deep-copy each child.
    for (const auto &child : children) {
        newBlock->addChild(child->clone());
    }
    return newBlock;
}

UniquePtr<BaseAST> FunctionRef::clone() const {
    return makeUnique<FunctionRef>(name, getScope());
}

UniquePtr<BaseAST> FunctionCall::clone() const {
    Vector<UniquePtr<ASTStatement>> clonedArgs;
    for (const auto &arg : arguments) {
        UniquePtr<BaseAST> clonedArgBase = arg->clone();
        auto clonedArg = static_unique_ptr_cast<ASTStatement>(std::move(clonedArgBase));
        clonedArgs.push_back(std::move(clonedArg));
    }
    return makeUnique<FunctionCall>(name, std::move(clonedArgs), getScope());
}