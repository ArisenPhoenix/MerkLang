#include <sstream>
#include <stdexcept>
#include <iostream>
#include <typeindex>
#include <optional>
#include <cassert>
#include <string>
#include <unordered_set>

#include "core/types.h"
#include "core/errors.h"
#include "core/node.h"

#include "ast/exceptions.h"

#include "ast/ast_base.h"
#include "ast/ast.h"
#include "ast/ast_control.h"

#include "utilities/debugger.h"
#include "utilities/debugging_functions.h"
#include "utilities/helper_functions.h"

#include "core/evaluator.h"
#include "core/scope.h"


FreeVarCollection::~FreeVarCollection() {
    freeVars.clear();
    localAssign.clear();
}

AstCollector::~AstCollector() {
    collectedNodes.clear();
}

const Vector<BaseAST*>& AstCollector::collectChildrenOfType(const Vector<UniquePtr<BaseAST>>& children, AstType type) const {
    collectedNodes.clear();
    for (const auto& child : children) {
        if (child && child->getAstType() == type) {
            collectedNodes.push_back(child.get());
        }
    }
    return collectedNodes;
}
const Vector<BaseAST*>& AstCollector::collectChildrenOfType(const Vector<UniquePtr<BaseAST>>& children, const Vector<AstType>& types) const {
    collectedNodes.clear();
    for (const auto& child : children) {
        if (!child) continue;
        if (std::find(types.begin(), types.end(), child->getAstType()) != types.end()) {
            collectedNodes.push_back(child.get());
        }
    }
    return collectedNodes;
}

// Control Constructors
CodeBlock::CodeBlock(SharedPtr<Scope> scope): scope(scope) {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    if (!getScope()) {
        DEBUG_LOG(LogLevel::INFO, highlight("CodeBlock initialized with nullptr scope.", Colors::red), scope);

        throw MerkError("CodeBlock must have a valid scope for constructor.");
    }
    DEBUG_LOG(LogLevel::INFO, "CodeBlock initialized within scopeLevel: ", getScope()->getScopeLevel());
    branch = "CodeBlock";

    DEBUG_FLOW_EXIT();
}


CodeBlock::CodeBlock(Vector<UniquePtr<BaseAST>> otherChildren, SharedPtr<Scope> scope) : BaseAST(), scope(scope) {
    children = std::move(otherChildren);
}

void CodeBlock::clear() {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    DEBUG_LOG(LogLevel::INFO, 
        getScope() && getScope()->getScopeLevel() == 0 
            ? "Destroying Root CodeBlock." 
            : "Destroying CodeBlock", 
        "Scope address: ", 
             getScope() ? getScope().get() : nullptr, 
        ", Scope level: ", 
             getScope() ? getScope()->getScopeLevel() : -1);

    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        it->get()->getScope().reset();
        it->reset();
    }
    children.clear();
    if (getScope().use_count() <= 1) {
        getScope().reset();
    }
    DEBUG_FLOW_EXIT();
}

CodeBlock::~CodeBlock() {
    clear();
}

void CodeBlock::addChild(UniquePtr<BaseAST> child) {
    String childType = child->getAstTypeAsString();
    DEBUG_LOG(LogLevel::TRACE, "Adding", childType, "to CodeBlock. Child address: ", child.get(), 
             ", CodeBlock address: ", this, ", Scope address: ", getScope());
             
    if (!child) {
        throw MerkError("Attempted to add a " + childType + "child to CodeBlock.");
    }
    if (this->contains(child.get())) {
        throw MerkError("Cycle detected: CodeBlock cannot add itself or its ancestor as a child.");
    }

    if (!child->getScope()) {  // In case it was missed elsewhere?
        child->setScope(getScope());
    }

    children.push_back(std::move(child));
    DEBUG_LOG(LogLevel::INFO, childType, "added. Children count: ", children.size());
}

const Vector<UniquePtr<BaseAST>>& CodeBlock::getChildren() const {
    return children;  // Fall back to the direct children of the current block
}

bool CodeBlock::contains(const BaseAST* ASTStatement) const {
    DEBUG_FLOW();

    for (const auto& child : children) {
        if (child.get() == ASTStatement) return true;
        if (child->contains(ASTStatement)) return true; // Polymorphic call
    }

    DEBUG_FLOW_EXIT();
    return false;
}

ElseStatement::ElseStatement(UniquePtr<CodeBlock> body, SharedPtr<Scope> scope)
    : ASTStatement(scope), body(std::move(body)) {
        DEBUG_FLOW(FlowLevel::VERY_LOW);

        branch = "HoldsBody";
        DEBUG_FLOW_EXIT();
    }


// Control Flow Constructors
ConditionalBlock::ConditionalBlock(UniquePtr<ASTStatement> condition, SharedPtr<Scope> scope)
    : ASTStatement(scope), condition(std::move(condition)) {
        DEBUG_FLOW(FlowLevel::HIGH);
        // validateScope(getScope(), "ConditionalBlock::ConditionalBlock", condition->toString());
        if (!getCondition()){throw MerkError("ConditionalBlock missing condition in ConditionalBlock constructor.");}
        branch = "Conditional";
        DEBUG_FLOW_EXIT();
    }

ElifStatement::ElifStatement(UniquePtr<ASTStatement> condition, UniquePtr<CodeBlock> body, SharedPtr<Scope> scope)
    : ASTStatement(scope), body(std::move(body)), condition(ConditionalBlock::create(std::move(condition), scope)) {
        DEBUG_FLOW(FlowLevel::VERY_LOW);

        if (!getCondition()){
            DEBUG_LOG(LogLevel::ERROR, highlight("Constructing ElIfStatement Failed as it has no condition", Colors::red));
            throw MerkError("ElIfStatement missing condition in ElifStatement constructor.");
        }
        branch = "HoldsBody";

        DEBUG_FLOW_EXIT();
    }

IfStatement::IfStatement(UniquePtr<ASTStatement> condition, UniquePtr<CodeBlock> body, SharedPtr<Scope> scope)
    // : ConditionalBlock(std::move(condition), scope), body(std::move(body)) {
    : ASTStatement(scope), body(std::move(body)), condition(ConditionalBlock::create(std::move(condition), scope)) {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    DEBUG_LOG(LogLevel::INFO, highlight("Constructing IfStatement", Colors::red));

    branch = "HoldsBody";

    DEBUG_FLOW_EXIT();
}

LoopBlock::LoopBlock(UniquePtr<ConditionalBlock> condition, UniquePtr<CodeBlock> body, SharedPtr<Scope> scope)
    : ASTStatement(scope), condition(std::move(condition)), body(std::move(body)) {
        DEBUG_FLOW(FlowLevel::VERY_LOW);
        
        branch = "HoldsBody";

        DEBUG_FLOW_EXIT();
    }

LoopBlock::~LoopBlock() {
    // DEBUG_FLOW();
    // DEBUG_FLOW_EXIT();
}


// Loop Constructors
WhileLoop::WhileLoop(UniquePtr<ConditionalBlock> condition, UniquePtr<CodeBlock> body, SharedPtr<Scope> scope)
    : LoopBlock(std::move(condition), std::move(body), scope) {
        DEBUG_FLOW(FlowLevel::VERY_LOW);

        // validateScope(scope, "WhileLoop");
        if (!getCondition()) {
            throw MerkError("WhileLoop: condition is null during initialization.");
        }
        if (!getBody()) {
            throw MerkError("WhileLoop: body is null during initialization.");
        }
        DEBUG_LOG(LogLevel::INFO, "Condition ownership in WhileLoop: valid");
        DEBUG_LOG(LogLevel::INFO, "Body ownership in WhileLoop: valid");
        DEBUG_LOG(LogLevel::INFO, "Condition ownership in WhileLoop: ", this->condition ? "valid" : "null");
        DEBUG_LOG(LogLevel::INFO, "Body ownership in WhileLoop: ", this->body ? "valid" : "null");
        
        DEBUG_FLOW_EXIT();
}

WhileLoop::~WhileLoop(){
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    DEBUG_LOG(LogLevel::FLOW, "Destroyed WhileLoop with scope level: ");

    DEBUG_FLOW_EXIT();

}

Break::Break(SharedPtr<Scope> scope) : ASTStatement(scope) {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    // validateScope(scope, "Break::Break", "");

    DEBUG_FLOW_EXIT();
}

Node ConditionalBlock::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    
    return condition.get()->evaluate(scope, instanceNode);
}
Node ConditionalBlock::evaluate() const {return condition.get()->evaluate(getScope());}

// Loop Evaluations
Node LoopBlock::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::MED);

    (void)scope;
    DEBUG_FLOW_EXIT();
    throw MerkError("LoopBlock is a base class and cannot be evaluated directly.");


}

Node WhileLoop::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::LOW);

    DEBUG_LOG(LogLevel::INFO, "Evaluating WhileLoop with scope level: ", scope->getScopeLevel());
    Node val = Evaluator::evaluateWhileLoop(*condition, body.get(), scope, instanceNode);

    DEBUG_FLOW_EXIT();
    return val;
}

void CodeBlock::setScope(SharedPtr<Scope> newScope) const {
    MARK_UNUSED_MULTI(newScope);
    // this->scope = newScope;
}

// Control Flow Evaluations
Node CodeBlock::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::LOW);
    setScope(scope);
    // validateScope(scope, "CodeBlock::evaluate(scope)");
    Node val =  Evaluator::evaluateBlock(children, scope, instanceNode);

    DEBUG_FLOW_EXIT();
    return val;
}

Node ElseStatement::evaluate(SharedPtr<Scope> evalScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::LOW);

    // validateScope(evalScope, "ElseStatement::evaluate");

    Node val = Evaluator::evaluateElse(*body, evalScope, instanceNode);

    DEBUG_FLOW_EXIT();
    return val;
};

Node ElifStatement::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::LOW);

    if (!condition) { throw MerkError("ElIfStatement missing condition in ElifStatement::evaluate."); }
    // validateScope(scope, "ElIfStatement::evaluate", condition->toString());

    Node val = Evaluator::evaluateElif(*this, scope, instanceNode);

    DEBUG_FLOW_EXIT();
    return val;
}

Node IfStatement::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::LOW);
    DEBUG_LOG(LogLevel::TRACE, highlight("Validating IfStatement", Colors::red));
    if (!condition){throw MerkError("IfStatement missing condition.");}
    // validateScope(scope, "IfStatement::evaluate", condition->toString());

    Node val = Evaluator::evaluateIf(*this, scope, instanceNode);

    DEBUG_FLOW_EXIT();
    return val;
}

void CodeBlock::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    DEBUG_LOG(LogLevel::TRACE, highlight("Setting CodeBlock Scope", Colors::blue));
    // if (!newScope){throw MerkError("CodeBlock's updated Scope is null");}
    // scope = newScope;
    // for (auto& child : children){
    //     child->setScope(newScope);
    // }
    
}

SharedPtr<Scope> CodeBlock::getScope() const {
    return scope;
}

void ConditionalBlock::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    // scope = newScope;
    // condition->setScope(newScope);
}


// setScope Overrides
void ElseStatement::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    DEBUG_LOG(LogLevel::TRACE, highlight("Setting ElseStatement Scope", Colors::blue));
    // scope = newScope;
    // body->setScope(newScope);
}

void ElifStatement::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    DEBUG_LOG(LogLevel::TRACE, highlight("Setting ElifStatement Scope", Colors::blue));

    // scope = newScope;
    // condition->setScope(newScope);
    // body->setScope(newScope);
}

void IfStatement::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    DEBUG_LOG(LogLevel::TRACE, highlight("Setting IfStatement Scope", Colors::pink));
    DEBUG_LOG(LogLevel::TRACE, highlight("Passed Iniital Check of body Scope", Colors::pink));

    if (!newScope){
        DEBUG_LOG(LogLevel::ERROR, "NewScope Didn't Exist");
        throw MerkError("New Scope in IfStatement Did not exist");
    }

    // scope = newScope;
    // condition->setScope(newScope);
    
    // SharedPtr<Scope> bodyScope = newScope->createChildScope();
    // body->setScope(bodyScope);

    // for (auto& elifNode : elifNodes) {
    //     elifNode->setScope(newScope);
    //     elifNode->getBody()->setScope(bodyScope);
    // }

    // if (elseNode){
    //     elseNode->setScope(newScope);
    //     elseNode->getBody()->setScope(bodyScope);
    // }
}

void WhileLoop::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    scope = newScope;
    condition->setScope(newScope);
    body->setScope(newScope);
}

void LoopBlock::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    // scope = newScope;
    // condition->setScope(newScope);
    // body->setScope(newScope->createChildScope());
}



// NoOp, perhaps will be used in the future. Still unsure, so keeping it.
Node NoOpNode::evaluate([[maybe_unused]] SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    (void)scope; // Not using it.
    DEBUG_LOG(LogLevel::DEBUG, "Evaluating NoOpNode (doing nothing).");

    return {};
}

[[noreturn]] Node Break::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW();

    // validateScope(scope, "Break");

    DEBUG_FLOW_EXIT();
    Evaluator::evaluateBreak(scope, instanceNode);
}


 
