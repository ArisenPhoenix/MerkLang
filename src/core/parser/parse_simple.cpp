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



// Input	Meaning
// self.x	Dot syntax (attribute)
// -3.14	Numeric literal (float)
// fib(3).x	Dot syntax after a call
// 0.5	Float literal
// myFunc().3 Invalid
// myClass().x Valid




Token Parser::handleAttributNotation(){
    DEBUG_FLOW(FlowLevel::HIGH);
    Token variableToken = currentToken();
    if (variableToken.type == TokenType::AccessorVariable){
        return variableToken;
    }
    TokenType tokenType = variableToken.type;

    String varName;
    
    if (!(expect(TokenType::Variable) || expect(TokenType::Argument))){
        throw UnexpectedTokenError(currentToken(), "variable, argument");
    }

    if (insideClass && existing(TokenType::Punctuation, 2)){
        // DEBUG_LOG(LogLevel::DEBUG, highlight("Inside Class and Punctuation exists", Colors::red));

        String accessor = currentToken().value;
        tokenType = TokenType::AccessorVariable;
        advance(); // consume the accessor
        if (currentToken().value != "."){
            throw UnexpectedTokenError(currentToken(), ".");
        }
        advance(); // consume the dot
        // DEBUG_LOG(LogLevel::DEBUG, highlight("Advanced Past DOT, Current Token: ", Colors::red), currentToken().toString());
        // DEBUG_LOG(LogLevel::DEBUG, "Checking a self.<whatever>");
        if (!expect(TokenType::Variable)){
            throw UnexpectedTokenError(currentToken(), "Expected a variable after dot for class Object");
        }


    if (!(expect(TokenType::Variable) || expect(TokenType::Argument))){
        throw UnexpectedTokenError(currentToken(), "variable, argument");
    }

    String attributeName = currentToken().value;

    varName = variableToken.value + String(".") + attributeName;

    } else {
        DEBUG_LOG(LogLevel::DEBUG, "This is A Normal Variable Declaration");
        varName = variableToken.value;
    }

    Token updatedToken = Token(tokenType, varName, currentToken().line, currentToken().column);
    tokens[position] = updatedToken;
    DEBUG_LOG(LogLevel::DEBUG, "Updated Token:", updatedToken.toColoredString());
    DEBUG_LOG(LogLevel::DEBUG, "Updated Token At Position", position, "Should Be the Same: ", tokens[position].toColoredString());
    DEBUG_LOG(LogLevel::DEBUG, "CurrentToken AT Position", position, "Should Be the Same: ", currentToken().toColoredString());
    DEBUG_FLOW_EXIT();
    return updatedToken;
}

UniquePtr<ASTStatement> Parser::parseVariableDeclaration() {
    DEBUG_LOG(LogLevel::DEBUG, "Parser::parseVariableDeclaration with token: ", currentToken().toString());

    // Determine reassignability
    bool isConst = false;

    if (currentToken().value == "var" || currentToken().value == "const") {
        isConst = currentToken().value == "const";
        advance();  // Consume 'var' or 'const'
    } else {
        throw MerkError("Expected 'var' or 'const' keyword for variable declaration. Token: " + currentToken().toString());
    }

     // Expects a variable name
    // Token variableToken = currentToken();
    Token variableToken = handleAttributNotation();
    if (variableToken.type != TokenType::Variable && variableToken.type != TokenType::AccessorVariable) {
        throw MerkError("Expected a variable name after 'var' or 'const'. Token: " + currentToken().toString());
    }

    advance();  // Consume the variable name

    // Expects an assignment operator
    if (currentToken().type != TokenType::VarAssignment) {
        throw MerkError("Expected ':=' or '=' for variable declaration. Token: " + currentToken().toString());
    }

    bool isMutable = (currentToken().value == "=");
    advance();  // Consume ':=' or '='


    auto valueNode = parseExpression();
    if (!valueNode) {
        throw MerkError("Failed to parse value for variable declaration: " + variableToken.value);
    }

    DEBUG_LOG(LogLevel::INFO, highlight("[Parsed ValueNode]: ", Colors::cyan), valueNode->getAstTypeAsString());

    DEBUG_LOG(LogLevel::INFO, highlight("[Value for VarNode]: ", Colors::yellow), valueNode->toString());

    // Implementation placeholder for when making Merk ALSO statically typed.
    // It hasn't been done in the Tokenizer yet, values are currently inferred
    const bool isStatic = false;  
    auto varNode = VarNode(variableToken.value, variableToken.typeAsString(), isConst, isMutable, isStatic);

    DEBUG_LOG(LogLevel::INFO, "[VarNode created]: ", varNode);
    DEBUG_LOG(LogLevel::INFO, "[VarNode Type]: ", nodeTypeToString(varNode.getType()));
    DEBUG_LOG(LogLevel::INFO, "[VarNode Value]: ", varNode.toString());
    
    auto varDec = makeUnique<VariableDeclaration>(
        variableToken.value,
        varNode,
        currentScope,  // Use the currentScope at the time of declaration
        std::nullopt,
        std::move(valueNode)
    );
    return variableToken.type == TokenType::Variable ? std::move(varDec) : makeUnique<AttributeDeclaration>(AttributeDeclaration(std::move(varDec)));
}

UniquePtr<ASTStatement> Parser::parseVariableAssignment() {
    DEBUG_FLOW(FlowLevel::HIGH);
    DEBUG_LOG(LogLevel::DEBUG, "DEBUG Parser: Entering parseVariableAssignment with token: ", currentToken().toString());

    if (!(currentToken().type == TokenType::Variable || currentToken().type == TokenType::AccessorVariable)) {
        throw MerkError("Expected an identifier for variable assignment.");
    }

    Token variableToken = handleAttributNotation();
    advance();

    if (currentToken().type != TokenType::VarAssignment || 
        (currentToken().value != "=" && currentToken().value != ":=")) {
        throw UnexpectedTokenError(currentToken(), "= or :=");
    }


    advance();  // consume assignment operator

    auto valueNode = parseExpression();
    if (!valueNode) {
        throw MerkError("Failed to parse value for assignment to " + variableToken.value);
    }

    // DEBUG_LOG(LogLevel::DEBUG, "Parser: Created VariableAssignment for ", variableToken.toString(), "\n");
    auto varAssign = makeUnique<VariableAssignment>(
        variableToken.value,
        std::move(valueNode),
        currentScope
    );
    DEBUG_FLOW_EXIT();
    return variableToken.type == TokenType::Variable ? std::move(varAssign) : makeUnique<AttributeAssignment>(AttributeAssignment(std::move(varAssign)));
 
}

UniquePtr<ASTStatement> Parser::parseExpression() {
    DEBUG_LOG(LogLevel::DEBUG, "Parser: Entering parseExpression with token: ", currentToken().toString());
    return parseBinaryExpression(0);
}

// Though a bit of a misnomer, it was named BinaryExpression or BinaryOperation due to how it only handles two values at a time (or one)
UniquePtr<ASTStatement> Parser::parseBinaryExpression(int precedence) {
    DEBUG_FLOW(FlowLevel::HIGH);
    DEBUG_LOG(LogLevel::DEBUG, highlight("=============================== Entering parseBinaryExpression ===============================", Colors::yellow), "\n\n\n");
    DEBUG_LOG(LogLevel::DEBUG, "DEBUG Parser: Entering parseExpression with token: ", currentToken().toString());
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
        // else if (op.value == "and" || op.value == "or" || op.value == "!" || op.value == "not" || op.value == "&&" || op.value == "||"){
        //     break;
        // }

        int opPrecedence = getOperatorPrecedence(op.value);
        if (opPrecedence < precedence) {
            DEBUG_LOG(LogLevel::DEBUG, "DEBUG: Operator precedence too low, breaking loop.");
            break;
        }

        advance(); // Consume the operator
        DEBUG_LOG(LogLevel::INFO, "Operator ", op.value, " consumed. Parsing right-hand expression.");

        auto right = parseBinaryExpression(opPrecedence + 1);
        if (!right) {
            throw MerkError("Right operand is null in parseBinaryExpression.");
        }
        DEBUG_LOG(LogLevel::INFO, "Right operand detected: ", right->toString());

        left = makeUnique<BinaryOperation>(
            op.value, std::move(left), std::move(right), currentScope
        );
        DEBUG_LOG(LogLevel::INFO, "Created BinaryOperation node for ", op.value);

    }
    DEBUG_LOG(LogLevel::DEBUG, "\n\n\n", highlight("=============================== Exiting parseBinaryExpression ===============================", Colors::yellow), "\n\n\n");
    DEBUG_FLOW_EXIT();
    return left;
}

UniquePtr<ASTStatement> Parser::parsePrimaryExpression() {
    DEBUG_FLOW(FlowLevel::HIGH);

    DEBUG_LOG(LogLevel::INFO, "DEBUG Parser::parsePrimaryExpression: Entering with token: ", currentToken().toString());
    
    Token token = currentToken();
    

    if (token.type == TokenType::Number || token.type == TokenType::String || token.type == TokenType::Bool) {
        LitNode nodeLiteral = LitNode(token.value, token.typeAsString());

        auto literalVal = makeUnique<LiteralValue>(
            nodeLiteral,
            currentScope,
            token.type == TokenType::String,
            token.type == TokenType::Bool
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
    if ((token.type == TokenType::Variable || 
        token.type == TokenType::FunctionRef || 
        token.type == TokenType::ClassRef || 
        token.type == TokenType::Parameter) && (peek().value == "." || peek().value == "::")) {

        return parseChain();
}

    if (token.type == TokenType::Variable) {
        Token variableToken = handleAttributNotation();

        auto varRefNode = makeUnique<VariableReference>(variableToken.value, currentScope);
        advance();  // Consume Variable Name
        DEBUG_FLOW_EXIT();
        return variableToken.type == TokenType::Variable ? std::move(varRefNode) : makeUnique<AttributeReference>(AttributeReference(std::move(varRefNode)));
    }

    else if (token.type == TokenType::Argument){
        Token argumentToken = handleAttributNotation();

        auto varRefNode = makeUnique<VariableReference>(argumentToken.value, currentScope);
        advance(); // consume Argument
        DEBUG_FLOW_EXIT();
        return varRefNode;
    }

    else if (token.type == TokenType::FunctionCall){
        // Indicates that a variable is being set to a function output value
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
    throw UnexpectedTokenError(token, "Not: " + currentToken().toString(), "in parsePrimaryExpression");
}

UniquePtr<BaseAST> Parser::parseStatement() {
    DEBUG_FLOW(FlowLevel::HIGH);

    DEBUG_LOG(LogLevel::INFO, "Parser::parseStatement: Entering with token: ", currentToken().toString());

    processNewLines();

    Token token = currentToken();

    DEBUG_LOG(LogLevel::INFO, "Parser::parseStatement: Processing token: ", currentToken().toString());

    // Due to IfStatement inheriting from ElifStatement, this check is necessary.
    if (token.value == "else" || token.value == "elif"){
        throw MerkError("if Statement must come before elif or else");
    }

    switch (token.type) {
        case TokenType::VarDeclaration:
            DEBUG_LOG(LogLevel::INFO, "Detected VarDeclaration, calling parseVariableDeclaration()");
            return parseVariableDeclaration();
        
        case TokenType::ClassDef:
            DEBUG_LOG(LogLevel::INFO, "Detected VarDeclaration, calling parseVariableDeclaration()");
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
                UniquePtr<ASTStatement> statement;

                statement = parsePrimaryExpression();  //Resolve to A FunctionCall

                DEBUG_LOG(LogLevel::INFO, "Detected FunctionCall, calling parseFunctionCall()");
                DEBUG_FLOW_EXIT();
                return statement;
            } else {
                throw MerkError("Token Is Not A Function Call");
            }
        
        case TokenType::ClassCall:
            if (token.type == TokenType::ClassCall){
                UniquePtr<ASTStatement> statement;

                statement = parsePrimaryExpression();  //Resolve to A ClassCall

                DEBUG_LOG(LogLevel::INFO, "Detected FunctionCall, calling parseFunctionCall()");
                DEBUG_FLOW_EXIT();
                return statement;
            } else {
                throw MerkError("Was A CLASS CALL, THEN NOT");
            }
            



        case TokenType::Variable:
            if (token.type == TokenType::Variable)
            {
                Token nextToken = peek(); 
                if (nextToken.type == TokenType::VarAssignment) {  
                    DEBUG_LOG(LogLevel::INFO, "Detected Variable Assignment, calling parseVariableAssignment()");
                    return parseVariableAssignment();
                }
                
                else if (nextToken.type == TokenType::Punctuation && nextToken.value == ".") {
                    handleAttributNotation();
                    // advance(); // consume varName
                    return parseVariableAssignment();
                }
                DEBUG_LOG(LogLevel::INFO, "Detected Variable Reference, treating as expression.");
                DEBUG_FLOW_EXIT();
                return parseExpression(); // Could be used in an expression (like a function argument)
            } else {
                throw MerkError("Was A VARIABLE, THEN NOT");
            }

        case TokenType::Keyword:
            // String val = token.value;
            if (token.value == "if" || token.value == "while") {
                // Entering a new scope here ensures that all branches of the 'if' statement 
                // (then, elif, else) share the same scope. Allowing them to share scope since they are
                // conditionally executed -- saving resources.

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
                return parseReturnStatement(); 
                
            } 

            else if (token.value == "continue") {
                return parseContinueStatement();
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

        // case TokenType::Operator:
        //     return parseExpression();

        default:
            DEBUG_FLOW_EXIT();
            throw UnexpectedTokenError(token, "Keyword, Variable, FunctionCall, FunctionDef, ClassDef, VarDeclaration", "ClassCall");

    }
}

UniquePtr<ASTStatement> Parser::parseBreakStatement() {
    DEBUG_FLOW(FlowLevel::HIGH);

    DEBUG_LOG(LogLevel::INFO, "Parser: parseBreakStatement with token: ", currentToken().toString());

    if (!isInsideLoop()) {
        throw MerkError("Break statement not allowed outside of a loop.");
    }
    
    DEBUG_LOG(LogLevel::INFO, "Parsing break statement at token: ", currentToken().toString());


    advance(); // Consume 'break'

    if (position < tokens.size() &&
        currentToken().type != TokenType::Newline &&
        currentToken().type != TokenType::EOF_Token) {
        DEBUG_FLOW_EXIT();
        throw UnexpectedTokenError(currentToken(), "");
    }
    DEBUG_FLOW_EXIT();
    return makeUnique<Break>(currentScope);
}



UniquePtr<ASTStatement> Parser::parseContinueStatement() {
    DEBUG_FLOW(FlowLevel::HIGH);

    DEBUG_LOG(LogLevel::INFO, "Parser: parseContinueStatement with token: ", currentToken().toString());

    if (!isInsideLoop()) {
        throw MerkError("Continue statement not allowed outside of a loop.");
    }
    
    DEBUG_LOG(LogLevel::INFO, "Parsing break statement at token: ", currentToken().toString());


    advance(); // Consume 'continue'

    if (position < tokens.size() &&
        currentToken().type != TokenType::Newline &&
        currentToken().type != TokenType::EOF_Token) {
        DEBUG_FLOW_EXIT();
        throw UnexpectedTokenError(currentToken(), "");
    }
    DEBUG_FLOW_EXIT();
    return makeUnique<Continue>(currentScope);
}