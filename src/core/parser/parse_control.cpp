
#include <stdexcept>
#include <iostream>
#include <variant>
#include <string>

#include "core/types.h"
#include "core/errors.h"
#include "core/node.h"

#include "ast/ast_base.h"
#include "ast/ast.h"
#include "ast/ast_control.h"
#include "ast/ast_function.h"
#include "core/scope.h"

#include "utilities/streaming.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"
#include "core/parser.h"


UniquePtr<IfStatement> Parser::parseIfStatement() {
    DEBUG_FLOW(FlowLevel::HIGH);
    consume(TokenType::Keyword, "if", "Parser::parseIfStatement");
    auto condition = parseExpression();
    if (!condition) {throw MerkError("Parser::parseIfStatement: Failed to parse 'if' condition.");}
    consume(TokenType::Punctuation, ":", "Parser::parseIfStatement");
    SharedPtr<Scope> conditionScope = condition->getScope();             // Controlling Statement Scope
    SharedPtr<Scope> blocksScope = conditionScope->createChildScope();        // Controlling Body Scope

    auto thenBlock = parseBlock(blocksScope);
    if (!thenBlock) {throw MerkError("Parser::parseIfStatement: Failed to parse 'if' block.");}
    

    auto ifNode = makeUnique<IfStatement>(
        std::move(condition),
        std::move(thenBlock),
        currentScope
    );
   

    if (!blocksScope){throw MerkError("BlockScope Does Not Exist");}
    if (!conditionScope){throw MerkError("StatementScope Does Not Exist");}

    while (consumeIf(TokenType::Keyword, "elif")) {
        auto elifCondition = parseExpression();
        // elifCondition->setScope(conditionScope);

        DEBUG_LOG(LogLevel::TRACE, highlight("Processed elifCondition", Colors::pink));

        if (!elifCondition) { throw MerkError("Parser::parseIfStatement: Failed to parse 'elif' condition."); }

        consume(TokenType::Punctuation, ":", "Parser::parseIfStatement -> elif");

        auto elifBlock = parseBlock(blocksScope);

        if (!elifBlock) {throw MerkError("Parser::parseIfStatement: Failed to parse 'elif' block.");}

        // Create an ElIfStatement and add it to the vector
        auto elifNode = makeUnique<ElifStatement>(
            std::move(elifCondition),
            std::move(elifBlock),
            conditionScope
        );

        // elifNodes.push_back(std::move(elifNode));
        ifNode->addElifNode(std::move(elifNode));
    }

    if (consumeIf(TokenType::Keyword, "else")) {
        consume(TokenType::Punctuation, ":", "Parser::parseIfStatement -> else");

        auto elseBlock = parseBlock(blocksScope);
        if (!elseBlock) { throw MerkError("Parser::parseIfStatement: Failed to parse 'else' block."); }
        

        UniquePtr<ElseStatement> elseNode = makeUnique<ElseStatement>(std::move(elseBlock), conditionScope);
        ifNode->setElseNode(std::move(elseNode));
    }

    DEBUG_FLOW_EXIT();
    return ifNode;
}

UniquePtr<WhileLoop> Parser::parseWhileLoop() {
    // DEBUG_FLOW(FlowLevel::HIGH);

    Token controllingToken = currentToken();
    if (controllingToken.value != "while") {
        throw UnexpectedTokenError(currentToken(), "while", "Parser::parseWhileLoop");
    }
    controllingToken = advance(); // Consume 'while'

    auto condition = parseExpression(); // Parse the loop condition
    if (!condition) {
        throw MissingTokenError(currentToken());
    }
    controllingToken = currentToken();
    if (controllingToken.type != TokenType::Punctuation || controllingToken.value != ":") {
        throw MissingTokenError(controllingToken);
    }
    advance(); // Consume ':'

    enterLoop();
    auto body = parseBlock();
    exitLoop();

    DEBUG_LOG(LogLevel::TRACE, highlight("Creating while loop condition", Colors::orange));
    auto conditionalBlock = makeUnique<ConditionalBlock>(std::move(condition), currentScope);
    
    // DEBUG_FLOW_EXIT();
    return makeUnique<WhileLoop>(std::move(conditionalBlock), std::move(body), currentScope);
}

UniquePtr<CodeBlock> Parser::parseBlock(SharedPtr<Scope> controlScope) {
    // DEBUG_FLOW(FlowLevel::HIGH);
    
    auto blockScope = currentScope;
    if (controlScope){
        blockScope = controlScope;
    }

    auto codeBlock = makeUnique<CodeBlock>(blockScope);

    processNewLines();
    
    processIndent(blockScope);

    // DEBUG_LOG(LogLevel::TRACE, "Current Scope Level: ", blockScope->getScopeLevel());

    while (currentToken().type != TokenType::Dedent && currentToken().type != TokenType::EOF_Token) {
        if (processNewLines()){
            continue;
        }

        auto statement = parseStatement();
        if (statement) {
            codeBlock->addChild(std::move(statement));
            
        } else {
            // DEBUG_LOG(LogLevel::INFO, "parseBlock: CodeBlock has children.");
        }

    }

    processDedent(blockScope);

    processNewLines();
    
    // DEBUG_FLOW_EXIT();
    return codeBlock;
}
