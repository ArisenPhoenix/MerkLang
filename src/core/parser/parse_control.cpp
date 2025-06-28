
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
    // DEBUG_FLOW(FlowLevel::HIGH);
    if (currentToken().value != "if") {
        throw MissingTokenError(currentToken(), "Expected 'if' keyword.");
    }
    advance(); // Consume 'if'

    // Parse the condition
    auto condition = parseExpression();
    if (!condition) {
        throw MerkError("Parser::parseIfStatement: Failed to parse 'if' condition.");
    }

    if (currentToken().type != TokenType::Punctuation || currentToken().value != ":") {
        throw UnexpectedTokenError(currentToken(), "Expected ':' after 'if' condition.", "Parser::parseIfStatement");
    }
    advance(); // Consume ':'

    auto thenBlock = parseBlock();

    SharedPtr<Scope> statementsScope = currentScope;             // Controlling Statement Scope
    SharedPtr<Scope> blocksScope = thenBlock->getScope();        // Controlling Body Scope

    if (!blocksScope){
        throw MerkError("BlockScope Does Not Exist");
    }
    if (!statementsScope){
        throw MerkError("StatementScope Does Not Exist");
    }

    DEBUG_LOG(LogLevel::INFO, "If Condition In Parser: ");
    
    if (!thenBlock) {
        throw MerkError("Parser::parseIfStatement: Failed to parse 'if' block.");
    }


    DEBUG_LOG(LogLevel::TRACE, "Parser::parseIfStatement: Parsed 'if' block. Current Scope Level: ", currentScope->getScopeLevel());

    // Vector to hold elif nodes
    Vector<UniquePtr<ElifStatement>> elifNodes;

    DEBUG_LOG(LogLevel::DEBUG, "Processing Elifs Now");

    // Parse `elif` blocks
    while (currentToken().type == TokenType::Keyword && currentToken().value == "elif") {
        advance(); // Consume 'elif'

        auto elifCondition = parseExpression();

        DEBUG_LOG(LogLevel::TRACE, highlight("Processed elifCondition", Colors::pink));

        if (!elifCondition) {
            throw MerkError("Parser::parseIfStatement: Failed to parse 'elif' condition.");
        }

        if (currentToken().type != TokenType::Punctuation || currentToken().value != ":") {
            throw UnexpectedTokenError(currentToken(), "Expected ':' after 'elif' condition.", "Parser::parseIfStatement");
        }
     

        advance(); // Consume ':'

        auto elifBlock = parseBlock(blocksScope);

        if (!elifBlock) {
            throw MerkError("Parser::parseIfStatement: Failed to parse 'elif' block.");
        }

        // Create an ElIfStatement and add it to the vector
        auto elifNode = makeUnique<ElifStatement>(
            std::move(elifCondition),
            std::move(elifBlock),  // Pass unique block
            statementsScope
        );

        elifNodes.push_back(std::move(elifNode));
    }

    UniquePtr<ElseStatement> elseNode = nullptr;
    if (currentToken().type == TokenType::Keyword && currentToken().value == "else") {
        advance(); // Consume 'else'

        if (currentToken().type != TokenType::Punctuation || currentToken().value != ":") {
            throw UnexpectedTokenError(currentToken(), "Expected ':' after 'else'.", "Parser::parseIfStatement");
        }
        advance(); // Consume ':'

        auto elseBlock = parseBlock(blocksScope);
        if (!elseBlock) {
            throw MerkError("Parser::parseIfStatement: Failed to parse 'else' block.");
        }

        elseNode = makeUnique<ElseStatement>(std::move(elseBlock), statementsScope); // Pass unique block
        DEBUG_LOG(LogLevel::TRACE, "Parser::parseIfStatement: Parsed 'else' block. Current Scope Level: ", 
                 currentScope->getScopeLevel());
    }
    auto ifNode = makeUnique<IfStatement>(
        
        std::move(condition),
        std::move(thenBlock),  // Pass unique block
        currentScope
    );
    
    for (auto& elifNode : elifNodes) {
        ifNode->addElifNode(std::move(elifNode));
    }

    ifNode->setElseNode(std::move(elseNode));


    // DEBUG_FLOW_EXIT();
    return ifNode;
}

UniquePtr<WhileLoop> Parser::parseWhileLoop() {
    // DEBUG_FLOW(FlowLevel::HIGH);
    if (currentToken().value != "while") {
        throw UnexpectedTokenError(currentToken(), "while", "Parser::parseWhileLoop");
    }
    advance(); // Consume 'while'

    auto condition = parseExpression(); // Parse the loop condition
    if (!condition) {
        throw MissingTokenError(currentToken());
    }

    if (currentToken().type != TokenType::Punctuation || currentToken().value != ":") {
        throw MissingTokenError(currentToken());
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
