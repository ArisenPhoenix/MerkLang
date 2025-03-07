#include <stdexcept>
#include <iostream>
#include <string>

#include "core/types.h"
#include "core/errors.h"
#include "core/node.h"

#include "ast/ast_base.h"
#include "ast/ast.h"
#include "ast/ast_control.h"
#include "ast/ast_function.h"
#include "core/scope.h"

#include "utilities/utilities.h"
#include "utilities/streaming.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"

#include "core/parser.h"





UniquePtr<FunctionDef> Parser::parseFunctionDefinition() {
    DEBUG_FLOW(FlowLevel::HIGH);
    DEBUG_LOG(LogLevel::INFO, "Parsing function definition...");
    Token token = currentToken();

    if (token.type != TokenType::FunctionDef || (token.value != "def" && token.value != "function")) {
        throw SyntaxError("Expected 'def' or 'function' keyword.", token);
    }

    String functionDefType = token.value;
    DEBUG_LOG(LogLevel::INFO, highlight("Function Type: ", Colors::red), functionDefType);
    advance(); // Consume 'def' or 'function'


    if (currentToken().type != TokenType::FunctionRef) {
        throw SyntaxError("Expected function name.", currentToken());
    }

    String functionName = currentToken().value;

    advance(); // Consume function name

    if (currentToken().type != TokenType::Punctuation || currentToken().value != "(") {
        throw SyntaxError("Expected '(' after function name.", currentToken());
    }

    advance(); // Consume '('

    ParamList parameters;
    
    if (!(expect(TokenType::Parameter) && currentToken().value == ")")) { // If not empty
        do {
            if (currentToken().type != TokenType::Parameter) {
                throw SyntaxError("Expected parameter name.", currentToken());
            }

            String paramName = currentToken().value;
            advance(); // Consume parameter name
            
            // Check if the parameter has a type annotation
            std::optional<NodeValueType> paramType = parseStaticType();
            
            if (expect(TokenType::VarAssignment)){
                advance(); // consume '='
                auto token = currentToken();
                DEBUG_LOG(LogLevel::INFO, "Param Token:", token.toString());
                if (token.type == TokenType::Number || token.type == TokenType::String || token.type == TokenType::Bool){
                    ParamNode paramNode(paramName, token.value, false, false);
                    parameters.addParameter(paramNode);
                    advance(); // consume param value.
                } else {
                    throw MerkError("Placeholder Param Error For Accurately Accounting For Assignment");
                }
                
            } else {
                // Create parameter node
                NodeValueType finalParamType = paramType ? paramType.value() : NodeValueType::Any;
                ParamNode paramNode = ParamNode(paramName, finalParamType, false, false);
                parameters.addParameter(paramNode);
            }

            DEBUG_LOG(LogLevel::INFO, "Param Added: ");
            if (expect(TokenType::Punctuation) && currentToken().value == ",") {
                advance(); // Consume ','
            } else {
                break; // No more parameters
            }
        } while (true);

        if (!expect(TokenType::Punctuation) || currentToken().value != ")") {
            throw SyntaxError("Expected ')' after parameters.", currentToken());
        }
    }
    
    advance(); // Consume ')'

    if (!expect(TokenType::Punctuation) || currentToken().value != ":") {
        throw SyntaxError("Expected ':' after function declaration.", currentToken());
    }
    
    advance(); // Consume ':'

    UniquePtr<CodeBlock> bodyBlock = parseBlock();

    if (!bodyBlock) {
        throw SyntaxError("Function body block could not be parsed.", currentToken());
    }

    UniquePtr<FunctionBody> functionBlock = makeUnique<FunctionBody>(std::move(bodyBlock));

    DEBUG_LOG(LogLevel::INFO, "FunctionBody type: ", typeid(*functionBlock).name());
    
    FunctionType funcType;

    if (functionDefType == "def"){
        funcType = FunctionType::DEF;
    }

    else if (functionDefType == "function"){
        funcType = FunctionType::FUNCTION;
    } 
    
    else {
        DEBUG_LOG(LogLevel::INFO, "Function definition parsed unsuccessfully: ", functionName, ":", functionDefType);
        throw MerkError("Function Type: " + functionDefType + " is not Valid");
    }
    
    UniquePtr<FunctionDef> functionNode = makeUnique<FunctionDef>(
        functionName,
        std::move(parameters),
        std::move(functionBlock),
        funcType,  
        currentScope
    );
    
    DEBUG_LOG(LogLevel::INFO, "Function definition parsed successfully: ", functionName);
    DEBUG_FLOW_EXIT();
    return functionNode;
}

UniquePtr<FunctionCall> Parser::parseFunctionCall() {
    DEBUG_FLOW(FlowLevel::HIGH);

    if (!expect(TokenType::FunctionCall)) {
        throw UnexpectedTokenError(currentToken(), "FunctionCall");
    }

    String functionName = currentToken().value;
    advance();  // Consume function name

    if (!expect(TokenType::Punctuation) || currentToken().value != "(") {
        throw UnexpectedTokenError(currentToken(), "'(' after function name", "parseFunctionCall");
    }

    advance();  // Consume '('

    // Parse function arguments
    Vector<UniquePtr<ASTStatement>> arguments;
    while (!expect(TokenType::Punctuation) || currentToken().value != ")") {
        DEBUG_LOG(LogLevel::INFO, "parsing function argument");
        auto argument = parseExpression();
        if (!argument) {
            throw SyntaxError("Invalid function argument.", currentToken());
        }

        DEBUG_LOG(LogLevel::INFO, "[Parser::parseFunctionCall] Parsed argument: ", argument->toString());
        arguments.push_back(std::move(argument));        

        if (expect(TokenType::Punctuation) && currentToken().value == ",") {
            advance();  // Consume ','
        } else {
            break;  // No more arguments
        }
    }

    // Ensure proper function call closure
    if (!expect(TokenType::Punctuation) || currentToken().value != ")") {
        throw UnexpectedTokenError(currentToken(), ")", "parseFunctionCall");
    }
    advance();  // Consume ')'

    DEBUG_LOG(LogLevel::INFO, "[Parser::parseFunctionCall] Completed parsing function call '", functionName,
             "' with ", arguments.size(), " arguments.");
    
    DEBUG_FLOW_EXIT();
    return makeUnique<FunctionCall>(functionName, std::move(arguments), currentScope);
}

UniquePtr<ASTStatement> Parser::parseReturnStatement() {
    DEBUG_FLOW(FlowLevel::HIGH);

    DEBUG_LOG(LogLevel::INFO, "DEBUG Parser::parseReturnStatement: Entering with token: ", currentToken().toString());

    if (!expect(TokenType::Keyword) || currentToken().value != "return") {
        throw SyntaxError("Expected 'return' keyword.", currentToken());
    }

    advance(); // Consume `return`

    // Ensure the return statement has a value
    if (currentToken().type == TokenType::Newline || currentToken().type == TokenType::EOF_Token) {
        throw SyntaxError("Return statement must return a value.", currentToken());
    }

    // Parse the return expression
    auto returnValue = parseExpression();
    if (!returnValue) {
        throw SyntaxError("Return statement must return a valid expression.", currentToken());
    }

    DEBUG_LOG(LogLevel::INFO, "DEBUG Parser::parseReturnStatement: Parsed return expression.");

    // Ensure the arguments are in the correct order
    DEBUG_FLOW_EXIT();
    return makeUnique<Return>(currentScope, std::move(returnValue));
}

















