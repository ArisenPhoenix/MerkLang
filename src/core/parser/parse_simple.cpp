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






UniquePtr<ASTStatement> Parser::parseVariableDeclaration() {
    DEBUG_FLOW(FlowLevel::NONE);
    // DEBUG_LOG(LogLevel::NONE, "DEBUG Parser::parseVariableDeclaration: Entering with token: ", currentToken().toColoredString());
    
    // Determine reassignability
    bool isConst = false;
    Token startToken = currentToken(); // should be var / const
    if (!(startToken.value == "var" || startToken.value == "const")) {
        throw MerkError("Expected 'var' or 'const' keyword for variable declaration. Token: " + startToken.toString());

    }

    if (startToken.value == "var" || startToken.value == "const") {
        if (peek().type == TokenType::ChainEntryPoint) {
            return parseChainOp();  // this should end up as a variable declaration chain
        }

        isConst = startToken.value == "const";
        advance();  // Consume 'var' or 'const'
    } else {
        // throw MerkError("Expected 'var' or 'const' keyword for variable declaration. Token: " + startToken.toString());
        throw UnexpectedTokenError(startToken, "var, const", "Parser::parseVariableDeclaration");
    }

    Token variableToken = currentToken();
    if (variableToken.type == TokenType::ChainEntryPoint) {throw std::logic_error("Invalid: ChainEntryPoint token passed into VarNode logic.");}

    advance(); //consume variable name

    // std::optional<NodeValueType> typeTag = parseStaticType();
    ResolvedType type = ResolvedType("Any");
    // std::optional<NodeValueType> typeTag = std::nullopt;   // used until ResolvedType is integrated into the Node system

    if (consumeIf(TokenType::Punctuation, ":")) {
        // throw MerkError("parsing Type");
        type = parseResolvedType();
        DEBUG_LOG(LogLevel::PERMISSIVE, "Current Token Is: ", currentToken().toColoredString(), "Current Type Is: ", type.toString());
    }


    
    

    // typeTag = getTypeFromString(type.getBaseType());


    Token assignment = currentToken();    

    if (assignment.type != TokenType::VarAssignment) {throw UnexpectedTokenError(assignment, "=, :=", "Parser::parseVariableDeclaration");}

    bool isMutable = (assignment.value == "=");
    advance();  // Consume ':=' or '='


    auto valueNode = parseExpression();
    if (!valueNode) {throw MerkError("Failed to parse value for variable declaration: " + variableToken.value);}

    const bool isStatic = false;  
    
    

    auto varNode = VarNode(variableToken.value, isConst, isMutable, isStatic);
    
    auto varDec = makeUnique<VariableDeclaration>(
        variableToken.value,
        varNode,
        currentScope,
        type,
        // typeTag,
        std::move(valueNode)
    );
    DEBUG_FLOW_EXIT();
    return varDec;
}

UniquePtr<ASTStatement> Parser::parseVariableAssignment() {
    DEBUG_FLOW(FlowLevel::NONE);
    // DEBUG_LOG(LogLevel::NONE, "DEBUG Parser::parseVariableAssignment: Entering with token: ", currentToken().toColoredString());

    // Token startToken = currentToken();
    Token variableToken = currentToken();

    if (!(variableToken.type == TokenType::Variable || variableToken.type == TokenType::ChainEntryPoint)) {
        throw MerkError("Expected an identifier for variable assignment.");
    }

    Token assignment = advance(); // consume variable

    if (assignment.type != TokenType::VarAssignment || 
        (assignment.value != "=" && assignment.value != ":=")) {
        throw UnexpectedTokenError(assignment, "= or :=");
    }


    advance();  // consume assignment operator

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
    DEBUG_FLOW(FlowLevel::NONE);
    // DEBUG_LOG(LogLevel::NONE, "Parser: Entering parseExpression with token: ", currentToken().toString());
    Token token = currentToken();
    if (check(TokenType::LeftBracket, "[") || check(TokenType::LeftArrow, "<") || check(TokenType::Operator, "{")){
        return parseClassLiteralCall();
    }
    // processNewLines();
    DEBUG_FLOW_EXIT();
    return parseBinaryExpression(0);
}

// Though a bit of a misnomer, it was named BinaryExpression or BinaryOperation due to how it only handles two values at a time (or one)
UniquePtr<ASTStatement> Parser::parseBinaryExpression(int precedence) {
    DEBUG_FLOW(FlowLevel::NONE);
    // DEBUG_LOG(LogLevel::NONE, "DEBUG Parser::parseBinaryExpression: Entering with token: ", currentToken().toColoredString());

    auto left = parsePrimaryExpression();
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
    DEBUG_FLOW(FlowLevel::NONE);

    // processNewLines();

    Token token = currentToken();
    // DEBUG_LOG(LogLevel::NONE, "DEBUG Parser::parsePrimaryExpression: Entering with token: ", currentToken().toColoredString());

    if (check(TokenType::LeftBracket, "[") || check(TokenType::LeftArrow, "<") || check(TokenType::Operator, "{")){
        return parseClassLiteralCall();
    }

        

    if (token.type == TokenType::Number || token.type == TokenType::String || token.type == TokenType::Bool) {

        LitNode nodeLiteral = LitNode(token.value, token.typeAsString());

        auto literalVal = makeUnique<LiteralValue>(
            nodeLiteral,
            currentScope
            // token.type == TokenType::String,
            // token.type == TokenType::Bool
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
        advance(); // move past '-'
        Token current = currentToken();
        
        if (current.type != TokenType::Number && op == "-"){
            throw MerkError("Cannot Make Type " + current.typeAsString() + " a negative");
        }

        if ((op == "and" || op == "or" || op == "not" || op == "!") && 
        (current.type != TokenType::Number && current.type != TokenType::Bool && current.type != TokenType::String)) {
            throw MerkError("Cannot Perform a Logical Operation On Type " + current.typeAsString());
  
        }
        auto operand = parsePrimaryExpression(); // recursively parse next value
        DEBUG_LOG(LogLevel::ERROR, "Operand: ", operand->toString());
        return makeUnique<UnaryOperation>(op, std::move(operand), currentScope);
    }
    // Chain-based variable, method, class references like a.b.c or Foo::bar
    if (token.type == TokenType::ChainEntryPoint) {
        return parseChainOp();  // handles the entire chain from here
    }
    

    if (token.type == TokenType::Variable) {
        auto varRefNode = makeUnique<VariableReference>(token.value, currentScope);
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

    else if (token.type == TokenType::Argument){
        if (peek().value == ".") {
            tokens[position].type = TokenType::ChainEntryPoint;
            return parseChainOp();
        }

        auto varRefNode = makeUnique<VariableReference>(token.value, currentScope);
        advance(); // consume Argument
        DEBUG_FLOW_EXIT();
        return varRefNode;
    }

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
    DEBUG_FLOW(FlowLevel::NONE);
    // DEBUG_LOG(LogLevel::NONE, "DEBUG Parser::parseStatement: Entering with token: ", currentToken().toColoredString());

    Token token = currentToken();
    // auto token = currentToken();
    // if (token.type == TokenType::Operator &&
    //     (token.value == "<" || token.value == "[" || token.value == "{")) {
    //     return parseClassLiteralCall();
    // }
    // Due to IfStatement inheriting from at one time (and maybe again) ElifStatement, this check is necessary.
    if (token.value == "else" || token.value == "elif"){
        throw MerkError("if Statement must come before elif or else");
    }
   

    switch (token.type) {
        case TokenType::VarDeclaration:
            DEBUG_FLOW_EXIT();
            if (peek().type == TokenType::ChainEntryPoint) {
                return parseChainOp();  //  `var self.x = ...`
            } else {
                return parseVariableDeclaration();  // `var x = ...`
            }
        
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
                return parseChainOp();  // <- Just call this directly now

            
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
            if (token.value == "if" || token.value == "while") {

                UniquePtr<ASTStatement> statement;
                if (token.value == "if") {
                    statement = parseIfStatement();
                }
                else if (token.value == "while"){
                    statement = parseWhileLoop();
                }

                DEBUG_FLOW_EXIT();
                return statement;
            } 
            else if (token.value == "return") {
                DEBUG_LOG(LogLevel::INFO, "Detected 'return', calling parseReturnStatement()");
                auto statement = parseReturnStatement();
                DEBUG_FLOW_EXIT();

                return statement; 
                
            } 

            else if (token.value == "continue") {
                auto statement = parseContinueStatement();
                DEBUG_FLOW_EXIT();

                return statement;
            }

            else if (token.value == "break") {
                // Ensure 'break' is inside a valid loop
                if (!isInsideLoop()) {  // Implement `isInsideLoop()`
                    throw SyntaxError("Unexpected 'break' outside loop: ", currentToken());
                }

                DEBUG_LOG(LogLevel::INFO, "Detected 'break', returning BreakStatement node");
                advance(); // consume break
                DEBUG_FLOW_EXIT();
                return makeUnique<Break>(currentScope);
            }


            
            else {
                
                DEBUG_FLOW_EXIT();
                throw SyntaxError("Unexpected Keyword token in parseStatement: ", currentToken());
            }

        default:
            DEBUG_FLOW_EXIT();
            throw UnexpectedTokenError(token, "Keyword, Variable, FunctionCall, FunctionDef, ClassDef, VarDeclaration", "Parser::parseStatement");

    }
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