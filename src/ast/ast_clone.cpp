#include "core/types.h"
#include "core/errors.h"
#include "utilities/debugger.h"
#include "ast/ast_base.h"
#include "ast/ast.h"

#include "ast/ast_control.h"
#include "ast/ast_function.h"
#include "ast/ast_callable.h"
#include "ast/ast_class.h"


#include "core/callables/classes/class_base.h"
#include "core/callables/classes/method.h"

UniquePtr<BaseAST> ASTStatement::clone() const {
    throw MerkError("Cannot Clone Base Class ASTStatement");
} 

UniquePtr<BaseAST> LiteralValue::clone() const {
    return std::make_unique<LiteralValue>(value, getScope());
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
    validateScope(getScope(), "VariableReference::clone", name);
    return makeUnique<VariableReference>(name, getScope());
} 

UniquePtr<BaseAST> VariableAssignment::clone() const {
    UniquePtr<BaseAST> clonedValBase = valueExpression->clone();
    auto clonedVal = static_unique_ptr_cast<ASTStatement>(std::move(clonedValBase));
    return makeUnique<VariableAssignment>(name, std::move(clonedVal), getScope());
} 

UniquePtr<BaseAST> BinaryOperation::clone() const {
    validateScope(getScope(), "BinaryOperation::clone->getScope");

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


UniquePtr<BaseAST> CodeBlock::clone() const {
    UniquePtr<CodeBlock> newBlock = makeUnique<CodeBlock>(getScope());

    for (const auto &child : children) {
        DEBUG_LOG(LogLevel::PERMISSIVE, "Current CodeBlock Child Being cloned", child->toString());
        if (!child->getScope()) {
            child->printAST(std::cout);
            throw MerkError("Child Has No Valid Scope");
        }
        newBlock->addChild(child->clone());
    }
    return newBlock;
}

UniquePtr<BaseAST> Break::clone() const {
    return makeUnique<Break>(getScope());
}

UniquePtr<BaseAST> Return::clone() const {
    validateScope(getScope(), "Return::clone->getScope");
    // DEBUG_LOG(LogLevel::PERMISSIVE, "Return->getScope was validated");
    UniquePtr<BaseAST> clonedReturnBase = returnValue->clone();
    // DEBUG_LOG(LogLevel::PERMISSIVE, "Return->returnValue was cloned");
    auto clonedReturn = static_unique_ptr_cast<ASTStatement>(std::move(clonedReturnBase));
    clonedReturn->setScope(getScope());
    validateScope(clonedReturn->getScope(), "Return::clone -> clonedReturn->getScope");
    
    return makeUnique<Return>(getScope(), std::move(clonedReturn));
} 

UniquePtr<BaseAST> ConditionalBlock::clone() const {
    validateScope(getScope(), "ConditionalBlock::clone", condition->toString());
    UniquePtr<BaseAST> clonedCondBase = condition->clone();
    auto clonedCond = static_unique_ptr_cast<ConditionalBlock>(std::move(clonedCondBase));
    clonedCond->setScope(getScope());
    validateScope(clonedCond->getScope(), "ConditionalBlock::clone -> clonedCond->getScope");
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
    if (!condition || (condition && !condition->getScope())) {throw MerkError("Condition is null in if statement");}
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
    validateScope(getScope(), "FunctionBody::clone");
    UniquePtr<FunctionBody> newBlock = std::make_unique<FunctionBody>(getScope());

    for (const auto &child : children) {
        DEBUG_LOG(LogLevel::PERMISSIVE, "Current FunctionBody child being cloned: ", child->toString());
        if (!child->getScope()) {
            child->printAST(std::cout);
            throw MerkError("Child Has No Valid Scope");
        }
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
        if (clonedArg->getAstType() == AstType::CallableCall) {
            throw MerkError("FunctionCall tried to clone a base CallableCall");
        }
        clonedArgs.push_back(std::move(clonedArg));
    }
    return makeUnique<FunctionCall>(name, std::move(clonedArgs), getScope());
}

UniquePtr<BaseAST> MethodCall::clone() const {
    Vector<UniquePtr<ASTStatement>> clonedArgs;
    for (const auto &arg : arguments) {
        UniquePtr<BaseAST> clonedArgBase = arg->clone();
        auto clonedArg = static_unique_ptr_cast<ASTStatement>(std::move(clonedArgBase));
        if (clonedArg->getAstType() == AstType::CallableCall) {
            throw MerkError("MethodCall tried to clone a base CallableCall");
        }
        clonedArgs.push_back(std::move(clonedArg));
    }
    return makeUnique<MethodCall>(name, std::move(clonedArgs), getScope());
}

UniquePtr<BaseAST> ClassBody::clone() const {
    if (!getScope()){throw MerkError("No Scope Present in ClassBody::clone()");}

    UniquePtr<ClassBody> newBlock = makeUnique<ClassBody>(getScope());
    std::unordered_map<String, SharedPtr<Scope>> methodScopes;

    for (const auto &child : children) {
        DEBUG_LOG(LogLevel::TRACE, "ClassBody::clone()", "current child", child->toString());

        if (!child){
            DEBUG_LOG(LogLevel::WARNING, "ClassBody::clone()", "Null child encountered in ClassBody::clone()");
        }

        newBlock->addChild(child->clone());
        
        DEBUG_LOG(LogLevel::TRACE, "ClassBody::clone()", "Added Child to ClassBody", child->getAstTypeAsString());

    }
    return newBlock;
}



UniquePtr<BaseAST> MethodBody::clone() const {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    validateScope(getScope(), "MethodBody::clone");
    UniquePtr<MethodBody> newBlock = makeUnique<MethodBody>(getScope());

    for (const auto &child : children) {newBlock->addChild(child->clone());}
    newBlock->setNonStaticElements(nonStaticElements);
    DEBUG_FLOW_EXIT();
    return newBlock;
}



UniquePtr<BaseAST> MethodDef::clone() const {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    if (!body->getScope()){throw MerkError("No scope present in MethodDef::clone");}
    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    
    auto clonedBody = static_unique_ptr_cast<MethodBody>(std::move(clonedBodyBase));
    if (!getScope()) {throw MerkError("MethodDef::clone -> No scope");}
    auto methodDef = makeUnique<MethodDef>(name, parameters.clone(), std::move(clonedBody), callType, getScope()->clone());
    String access = getMethodAccessor();

    clonedBody->setNonStaticElements(nonStaticElements);
    methodDef->setNonStaticElements(nonStaticElements);

    methodDef->setMethodAccessor(access);

    DEBUG_FLOW_EXIT();
    return methodDef;
}




UniquePtr<BaseAST> Accessor::clone() const {
    auto cloned = makeUnique<Accessor>(getAccessor(), getScope());
    return cloned;
}





UniquePtr<BaseAST> CallableBody::clone() const {
    UniquePtr<CallableBody> newBlock = makeUnique<CallableBody>(getScope());

    for (const auto &child : children) {newBlock->addChild(child->clone());}

    if (nonStaticElements.size() > 0) {newBlock->setNonStaticElements(nonStaticElements);}

    else {newBlock->setNonStaticElements({});}

    
    return newBlock;

}


UniquePtr<BaseAST> CallableDef::clone() const {
    ParamList clonedParams;
    for (auto& param: parameters) {clonedParams.addParameter(ParamNode(param));}

    UniquePtr<CallableBody> clonedBodyBase = static_unique_ptr_cast<CallableBody>(body->clone());
    

    UniquePtr<CallableDef> calDef = std::make_unique<CallableDef>(name, clonedParams, std::move(clonedBodyBase), callType, getScope());

    return calDef;
}

UniquePtr<BaseAST> CallableCall::clone() const {
    Vector<UniquePtr<ASTStatement>> clonedArgs;
    for (auto& arg : arguments){
        UniquePtr<ASTStatement> argBase = static_unique_ptr_cast<ASTStatement>(arg->clone());
        clonedArgs.emplace_back(std::move(argBase));
    }
    UniquePtr<CallableCall> calCall = std::make_unique<CallableCall>(name, std::move(clonedArgs), getScope());
    return calCall;
}

UniquePtr<BaseAST> CallableRef::clone() const {
    UniquePtr<CallableRef> calRef = std::make_unique<CallableRef>(name, getScope());
    return calRef;
}