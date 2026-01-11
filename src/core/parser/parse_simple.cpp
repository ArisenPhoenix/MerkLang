#include <stdexcept>
#include <iostream>
#include <string>
#include "core/node/Node.hpp"

#include "core/types.h"
#include "core/errors.h"

#include "ast/AstBase.hpp"
#include "ast/Ast.hpp"
#include "ast/Exceptions.hpp"
#include "ast/AstControl.hpp"
#include "ast/AstFunction.hpp"
#include "core/Scope.hpp"

#include "utilities/utilities.h"
#include "utilities/streaming.h"

#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"
#include "core/Parser.hpp"



UniquePtr<ASTStatement> Parser::parseVariableDeclaration() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    // DEBUG_LOG(LogLevel::NONE, "DEBUG Parser::parseVariableDeclaration: Entering with token: ", currentToken().toColoredString());
    
    // Determine reassignability
    
    Token startToken = currentToken(); // should be var / const
    
    if (peek().type == TokenType::ChainEntryPoint) {
        return parseChainOp();  // this should end up as a variable declaration chain
    }

    bool isConst = startToken.value == "const";
    Vector<String> values = {"var", "const"};
    consume(TokenType::VarDeclaration, values, "Parser::parseVariableDeclaration");

    Token variableToken = currentToken();
    DEBUG_LOG(LogLevel::NONE, "Var Token: ", variableToken.toColoredString());
    if (variableToken.value.empty()) {throw MerkError("VarName is empty in parseVariableDeclaration");}
    
    if (variableToken.type == TokenType::ChainEntryPoint) {throw RunTimeError("Invalid: ChainEntryPoint token passed into VarNode logic.");}

    // consume(TokenType::Variable, "Parser::parseVariableDeclaration");
    if (!consumeIf(TokenType::Variable)) {
        auto next = peek();
        if (next.type == TokenType::VarAssignment || next.type == TokenType::Punctuation) {
            advance(); // move past erroneously added functionReference from tokenizer
        } else {
            throw MerkError("Not A Variable -> Parser::parseVariableDeclaration");
        }
        
    }
    // advance(); //consume variable name

    // std::optional<NodeValueType> typeTag = parseStaticType();
    ResolvedType type = ResolvedType("Any");

    if (consumeIf(TokenType::Punctuation, ":")) {
        // throw MerkError("parsing Type");
        type = parseResolvedType();
        DEBUG_LOG(LogLevel::NONE, "Current Token Is: ", currentToken().toColoredString(), "Current Type Is: ", type.toString());
    }
    bool isMutable = currentToken().value == "=";
    values = {"=", ":="};
    consume(TokenType::VarAssignment, values, "Parser::parseVariableDeclaration");

    auto valueExpression = parseExpression();
    
    
    if (!valueExpression) {throw MerkError("Failed to parse value for variable declaration: " + variableToken.value);}

    // const bool isStatic = false;  
    auto varNodeMeta = DataTypeFlags(variableToken.value, isConst, isMutable, type.getBaseType() != "Any", type);


    if (variableToken.value.empty()) {throw MerkError("varName is empty in parseVariableDeclaration");}
    auto varDec = makeUnique<VariableDeclaration>(
        variableToken.value,
        varNodeMeta,
        currentScope,
        std::move(valueExpression)
    ); 
    DEBUG_FLOW_EXIT();

    // throw MerkError("Ok");
    return varDec;
}

UniquePtr<ASTStatement> Parser::parseVariableAssignment() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    // Peek ahead to check if this is a chain assignment (e.g., obj.prop = val)
    if (peek().type == TokenType::ChainEntryPoint) {
        return parseChainOp();  // handles chain assignments like `a.b.c = 5`
    }

    // Otherwise, fall back to normal variable assignment
    Token variableToken = currentToken();

    if (!consumeIf(TokenType::Variable)) {
        throw MerkError("Not a Variable in parseVariableAssignment: " + variableToken.toColoredString());
    }

    Vector<String> values = {"=", ":="};
    consume(TokenType::VarAssignment, values, "Parser::parseVariableAssignment");

    auto valueNode = parseExpression();
    if (!valueNode) {
        throw MerkError("Failed to parse value for assignment to " + variableToken.value);
    }

    auto varAssign = makeUnique<VariableAssignment>(
        variableToken.value,
        std::move(valueNode),
        currentScope
    );
    DEBUG_FLOW_EXIT();
    return varAssign;
}

UniquePtr<ASTStatement> Parser::parseExpression() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    // DEBUG_LOG(LogLevel::NONE, "Parser: Entering parseExpression with token: ", currentToken().toString());
    Token token = currentToken();
    // if (check(TokenType::LeftBracket, "[") || check(TokenType::LeftArrow, "<") || check(TokenType::Operator, "{")) { return parseClassLiteralCall(); }
    // processNewLines();
    DEBUG_FLOW_EXIT();
    return parseBinaryExpression(0);
}

// Though a bit of a misnomer, it was named BinaryExpression or BinaryOperation due to how it only handles two values at a time (or one)
UniquePtr<ASTStatement> Parser::parseBinaryExpression(int precedence) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    // DEBUG_LOG(LogLevel::NONE, "DEBUG Parser::parseBinaryExpression: Entering with token: ", currentToken().toColoredString());

    auto left = parsePrimaryExpression();
    auto current = currentToken();
    if (current.value == "." || current.value == "::") {
        left = parseChainOp(std::move(left));
    }
    if (!left) {
        throw SyntaxError("Expected a valid left-hand side expression.", currentToken());
    }

    DEBUG_LOG(LogLevel::INFO, "Left operand detected: ", left->toString());

    while (true) {
        Token op = currentToken();
        DEBUG_LOG(LogLevel::INFO, "Checking for operator, got: ", op.toString());

        if (op.type != TokenType::Operator) {
            DEBUG_LOG(LogLevel::INFO, "No operator found, breaking loop.");
            break;
        }
        
        // These options plan on being added later
        // else if (op.value == "and" || op.value == "or" || op.value == "!" || op.value == "not" || op.value == "&&" || op.value == "||"){
        //     break;
        // }

        int opPrecedence = getOperatorPrecedence(op.value);
        if (opPrecedence < precedence) {
            break;
        }

        advance(); // Consume the operator
        if (op.value == "++") {
            left = makeUnique<BinaryOperation>(
                op.value, std::move(left), std::move(makeUnique<NoOpNode>(currentScope)), currentScope
            );
            return left;
        }

        auto right = parseBinaryExpression(opPrecedence + 1);
        if (!right) {
            throw MerkError("Right operand is null in parseBinaryExpression.");
        }
        DEBUG_LOG(LogLevel::INFO, "Right operand detected: ", right->toString());
        if (!currentScope) {
            throw MerkError("Current Scope In Parser::parseBinaryExpression is null");
        }
        left = makeUnique<BinaryOperation>(
            op.value, std::move(left), std::move(right), currentScope
        );
        DEBUG_LOG(LogLevel::INFO, "Created BinaryOperation node for ", op.value);

    }
    DEBUG_FLOW_EXIT();
    return left;
}

UniquePtr<ASTStatement> Parser::parsePrimaryExpression() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    // processNewLines();

    Token token = currentToken();
    // DEBUG_LOG(LogLevel::NONE, "DEBUG Parser::parsePrimaryExpression: Entering with token: ", currentToken().toColoredString());

    if (check(TokenType::LeftBracket, "[") || check(TokenType::Operator, "<") || check(TokenType::Operator, "{")){
        return parseClassLiteralCall(); 
    }

        

    if (token.type == TokenType::Number || token.type == TokenType::String || token.type == TokenType::Bool || token.type == TokenType::Char) {
        DEBUG_LOG(LogLevel::NONE, token.toColoredString());
        // throw MerkError("SEE LITERAL ABOVE:" );
        // LitNode nodeLiteral = LitNode(token.value, token.typeAsString());
        
        auto nodeLiteral = LitNode(token.value, tokenTypeToString(token.type, false));
        if (!nodeLiteral.isValid() || !nodeLiteral.isValid()) {throw MerkError("LiterNode Is Invalid After Construction");}
        // throw MerkError("SEE LITERAL ABOVE:" );
        

        auto literalVal = makeUnique<LiteralValue>(
            nodeLiteral,
            currentScope
        );

        advance();  // Consume Literal Value
        DEBUG_FLOW_EXIT();
        return literalVal;
    }


    if (token.type == TokenType::Punctuation && token.value == "(") {
        advance();  // consume '('
    
        auto expr = parseBinaryExpression(0);  // recursively parse inner expression
    
        if (currentToken().type != TokenType::Punctuation || currentToken().value != ")") {
            throw UnexpectedTokenError(currentToken(), "expected ')'");
        }
    
        advance();  // consume ')'
    
        return expr;
    }

    if (token.type == TokenType::Operator && (token.value == "-" || token.value == "!" || token.value == "not" || token.value == "and" || token.value == "or")) {
        String op = token.value;
        advance(); // move past '-' etc.
        Token current = currentToken();
        
        if (current.type != TokenType::Number && op == "-"){
            throw MerkError("Cannot Make Type " + current.typeAsString() + " a negative");
        }

        auto operand = parsePrimaryExpression(); // recursively parse next value
        DEBUG_LOG(LogLevel::ERROR, "Operand: ", operand->toString());
        return makeUnique<UnaryOperation>(op, std::move(operand), currentScope);
    }

    // Chain-based variable, method, class references like a.b.c or Foo::bar
    if (token.type == TokenType::ChainEntryPoint) {
        return parseChainOp();  // handles the entire chain from here
    }
    

    if (token.type == TokenType::Variable || token.type == TokenType::Argument) {
        if (peek().value == ".") {
            tokens[position].type = TokenType::ChainEntryPoint;
            return parseChainOp();
        }

        auto varRefNode = makeUnique<VariableReference>(token.value, currentScope);
        if (peek().value == "?") {
            advance(); // consume Var Name
            advance(); // consume ?
            return makeUnique<UnaryOperation>("?", std::move(varRefNode), currentScope);
        }
        advance();  // Consume Variable Name
        DEBUG_FLOW_EXIT();
        return varRefNode;
    }

    else if (token.type == TokenType::FunctionRef){
        auto varRefNode = makeUnique<FunctionRef>(token.value, currentScope);
        advance();
        DEBUG_FLOW_EXIT();
        return varRefNode;
    }

    // else if (token.type == TokenType::Argument){
    //     if (peek().value == ".") {
    //         tokens[position].type = TokenType::ChainEntryPoint;
    //         return parseChainOp();
    //     }

    //     auto varRefNode = makeUnique<VariableReference>(token.value, currentScope);
    //     advance(); // consume Argument
    //     DEBUG_FLOW_EXIT();
    //     return varRefNode;
    // }

    else if (token.type == TokenType::FunctionCall){
        auto functionCall = parseFunctionCall();
        DEBUG_FLOW_EXIT();
        return functionCall;
    }

    else if (token.type == TokenType::ClassCall) {
        auto classCall = parseClassCall();
        DEBUG_FLOW_EXIT();
        return classCall;
    }

    DEBUG_FLOW_EXIT();
    throw UnexpectedTokenError(token, "ClassLiteral, ClassCall, FunctionCall, Argument, FunctionRef, Variable, ChainEntryPoint, Operator, Punctuation, Literal", "in parsePrimaryExpression");
}

UniquePtr<BaseAST> Parser::parseStatement() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    Token token = currentToken();
    if (token.value == "else" || token.value == "elif"){
        throw MerkError("if Statement must come before elif or else");
    }
   

    switch (token.type) {
        case TokenType::VarDeclaration:
            DEBUG_FLOW_EXIT();
            // if (peek().type == TokenType::ChainEntryPoint) {
            //     return parseChainOp();  //  `var self.x = ...`
            // } else {
            //     return parseVariableDeclaration();  // `var x = ...`
            // }

            return parseVariableDeclaration(); 
        
        case TokenType::ClassDef:
            DEBUG_LOG(LogLevel::INFO, "Detected VarDeclaration, calling parseVariableDeclaration()");
            DEBUG_FLOW_EXIT();

            return parseClassDefinition();
            
        case TokenType::FunctionDef:
            if (token.type == TokenType::FunctionDef){
                UniquePtr<ASTStatement> statement;
                
                statement = parseFunctionDefinition();
            
                DEBUG_LOG(LogLevel::INFO, "Detected FunctionDef, calling parseFunctionDefinition()");
                DEBUG_FLOW_EXIT();
                return statement;
            } else {
                throw MerkError("Token Is Not A Function Definition");
            }
            
        case TokenType::FunctionCall:
            if (token.type == TokenType::FunctionCall){
                auto statement = parsePrimaryExpression();  //Resolve to A FunctionCall

                DEBUG_LOG(LogLevel::INFO, "Detected FunctionCall, calling parseFunctionCall()");
                DEBUG_FLOW_EXIT();

                return statement;
            } else {
                DEBUG_FLOW_EXIT();

                throw MerkError("Token Is Not A Function Call");
            }
        
        case TokenType::ClassCall:
            if (token.type == TokenType::ClassCall){
                auto statement = parsePrimaryExpression();  //Resolve to A ClassCall

                DEBUG_LOG(LogLevel::INFO, "Detected FunctionCall, calling parseFunctionCall()");
                DEBUG_FLOW_EXIT();
                return statement;
            } else {
                throw MerkError("Was A CLASS CALL, THEN NOT");
            }
            
            case TokenType::ChainEntryPoint:
                return parseExpression(); 

            
        case TokenType::Variable:
        {

            if (peek().type == TokenType::VarAssignment) {
                DEBUG_FLOW_EXIT();

                return parseVariableAssignment();
            }

            DEBUG_FLOW_EXIT();

            return parseExpression();
        }


        case TokenType::Keyword:
            return parseKeyWord();

        default:
            DEBUG_FLOW_EXIT();
            displayPreviousTokens(currentToken().value, 5, "Parser::ParseStatement");
            displayNextTokens(currentToken().value, 5, "Parser::ParseStatement");
            // return parseStatement();
            throw UnexpectedTokenError(token, "Keyword, Variable, FunctionCall, FunctionDef, ClassDef, VarDeclaration", "Parser::parseStatement");

    }
}


UniquePtr<ASTStatement> Parser::parseKeyWord() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    Token token = currentToken();
    UniquePtr<ASTStatement> statement;
    if (token.value == "if") { statement = parseIfStatement(); } 

    else if (token.value == "while") { statement = parseWhileLoop(); }
    else if (token.value == "return") { statement = parseReturnStatement(); } 

    else if (token.value == "continue") { statement = parseContinueStatement(); }

    else if (token.value == "break") {
        // Ensure 'break' is inside a valid loop
        if (!isInsideLoop()) { throw SyntaxError("Unexpected 'break' outside loop: ", currentToken());
        }

        DEBUG_LOG(LogLevel::INFO, "Detected 'break', returning BreakStatement node");
        advance(); // consume break
        statement = makeUnique<Break>(currentScope);
    }


    
    else {
        
        DEBUG_FLOW_EXIT();
        throw SyntaxError("Unexpected Keyword token in parseStatement: ", currentToken());
    }

    return statement;
}


UniquePtr<ASTStatement> Parser::parseBreakStatement() {
    DEBUG_FLOW(FlowLevel::NONE);

    Token controllingToken = currentToken();

    DEBUG_LOG(LogLevel::TRACE, "Parser: parseBreakStatement with token: ", controllingToken.toColoredString());

    if (!isInsideLoop()) {
        throw MerkError("Break statement not allowed outside of a loop.");
    }


    controllingToken = advance(); // Consume 'break'

    if (position < tokens.size() && controllingToken.type != TokenType::Newline && controllingToken.type != TokenType::Dedent && controllingToken.type != TokenType::EOF_Token) {
        DEBUG_FLOW_EXIT();
        throw UnexpectedTokenError(controllingToken, "");
    }
    DEBUG_FLOW_EXIT();
    return makeUnique<Break>(currentScope);
}



UniquePtr<ASTStatement> Parser::parseContinueStatement() {
    DEBUG_FLOW(FlowLevel::NONE);
    Token controllingToken = currentToken();
    DEBUG_LOG(LogLevel::TRACE, "Parser: parseContinueStatement with token: ", controllingToken.toString());

    if (!isInsideLoop()) {
        throw MerkError("Continue statement not allowed outside of a loop.");
    }
    
    DEBUG_LOG(LogLevel::INFO, "Parsing break statement at token: ", controllingToken.toString());


    controllingToken = advance(); // Consume 'continue'

    if (position < tokens.size() && controllingToken.type != TokenType::Newline && controllingToken.type != TokenType::Dedent && controllingToken.type != TokenType::EOF_Token) {
        DEBUG_FLOW_EXIT();
        throw UnexpectedTokenError(controllingToken, "");
    }
    DEBUG_FLOW_EXIT();
    return makeUnique<Continue>(currentScope);
}