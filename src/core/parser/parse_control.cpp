
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
    // DEBUG_LOG(LogLevel::TRACE, "Parser::parseIfStatement: Parsing 'if' statement. Current Scope Level: ",
    //          currentScope->getScopeLevel());

    // DEBUG_LOG(LogLevel::ERROR, "Processing If Now");
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
        throw UnexpectedTokenError(currentToken(), "Expected ':' after 'if' condition.");
    }
    advance(); // Consume ':'

    // Parse the "then" block
    // DEBUG_LOG(LogLevel::ERROR, "Calling thenBLock parseBlock()");
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

        // Parse the condition for `elif`
        auto elifCondition = parseExpression();

        DEBUG_LOG(LogLevel::TRACE, highlight("Processed elifCondition", Colors::pink));

        if (!elifCondition) {
            throw MerkError("Parser::parseIfStatement: Failed to parse 'elif' condition.");
        }

        if (currentToken().type != TokenType::Punctuation || currentToken().value != ":") {
            throw UnexpectedTokenError(currentToken(), "Expected ':' after 'elif' condition.");
        }
     

        advance(); // Consume ':'

        // Parse the `elif` block
        // DEBUG_LOG(LogLevel::DEBUG, highlight("Attempting elifBlock", Colors::pink));

        auto elifBlock = parseBlock(blocksScope);
        // DEBUG_LOG(LogLevel::ERROR, highlight("Processed elifBlock", Colors::pink));

        if (!elifBlock) {
            throw MerkError("Parser::parseIfStatement: Failed to parse 'elif' block.");
        }

        // Create an ElIfStatement and add it to the vector
        // DEBUG_LOG(LogLevel::TRACE, "Elif Condition In Parser: ");
        auto elifNode = makeUnique<ElifStatement>(
            std::move(elifCondition),
            std::move(elifBlock),  // Pass unique block
            statementsScope
        );

        elifNodes.push_back(std::move(elifNode));
        // DEBUG_LOG(LogLevel::TRACE, highlight("Elif Added ", Colors::pink), "Current Scope Level: ", currentScope->getScopeLevel());
    }

    // Parse an optional `else` block
    // DEBUG_LOG(LogLevel::TRACE, "Processing Else Now");
    UniquePtr<ElseStatement> elseNode = nullptr;
    if (currentToken().type == TokenType::Keyword && currentToken().value == "else") {
        advance(); // Consume 'else'

        if (currentToken().type != TokenType::Punctuation || currentToken().value != ":") {
            throw UnexpectedTokenError(currentToken(), "Expected ':' after 'else'.");
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
    // DEBUG_LOG(LogLevel::TRACE, "Parser::parseWhileLoop: Entering");

    if (currentToken().value != "while") {
        throw UnexpectedTokenError(currentToken(), "while");
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

    // DEBUG_LOG(LogLevel::INFO, "Parser::parseBlock: Entering parseBlock with token: ", currentToken().toString());

    auto codeBlock = makeUnique<CodeBlock>(blockScope); // Use current scope here.

    processNewLines();
    
    processIndent(blockScope);

    // DEBUG_LOG(LogLevel::TRACE, "Current Scope Level: ", blockScope->getScopeLevel());

    while (currentToken().type != TokenType::Dedent && currentToken().type != TokenType::EOF_Token) {
        if (processNewLines()){
            continue;
        }

        auto statement = parseStatement();
        if (statement) {
            // DEBUG_LOG(LogLevel::INFO, "parseBlock: Adding statement to block. Type: ", statement->getAstTypeAsString());
            codeBlock->addChild(std::move(statement));
            // DEBUG_LOG(LogLevel::INFO,"Statement added to CodeBlock. Current children count: ", 
            // codeBlock->getChildren().size());
        } else {
            // DEBUG_LOG(LogLevel::INFO, "parseBlock: CodeBlock has children.");
        }

    }

    processDedent(blockScope);

    processNewLines();

    // DEBUG_LOG(LogLevel::TRACE, highlight("Parser::parseBlock: After processing block. Current Scope Level: ", Colors::pink), 
    // blockScope->getScopeLevel(), " | Current Scope Loc: ", blockScope.get());
    
    // DEBUG_FLOW_EXIT();
    return codeBlock;
}
