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



// Constructor
Parser::Parser(Vector<Token>& tokens, SharedPtr<Scope> rootScope, bool interpretMode, bool byBlock)
    : tokens(tokens), rootScope(rootScope), currentScope(rootScope), interpretMode(interpretMode), byBlock(byBlock) {
        // DEBUG_FLOW(FlowLevel::HIGH);
        if (!rootScope) {
            throw MerkError("Root scope must not be null when initializing the parser.");
        }
        rootBlock = makeUnique<CodeBlock>(rootScope);
        currentScope = rootScope->createChildScope();
        if (!currentScope){
            throw MerkError("Initial Scope is null");
        }
        // nativeFunctionNames = rootScope->functionNames;
        // DEBUG_LOG(LogLevel::INFO, "Parser initialized with root scope at level: ", rootScope->getScopeLevel(),
        //         " | Memory Loc: ", rootScope.get());
        // DEBUG_FLOW_EXIT();
}

UniquePtr<CodeBlock> Parser::parse() {
    // DEBUG_FLOW(FlowLevel::HIGH);

    // DEBUG_LOG(LogLevel::INFO, "\nParser::parse: running in ", interpretMode ? "Interpret Mode\n" : "Deferred Mode\n");
    if (!currentScope){
        throw MerkError("Initial Scope is null");
    }
    try {
        while (currentToken().type != TokenType::EOF_Token) {
            // DEBUG_LOG(LogLevel::TRACE, "DEBUG Parser::parse: with token:", currentToken().toString());
            // DEBUG_LOG(LogLevel::TRACE, "Scope use_count: ", currentScope.use_count(), ", Address: ", currentScope.get());
    
            if (currentToken().type == TokenType::Newline) {
                advance(); // Skip blank lines
                continue;
            }
    
            auto statement = parseStatement();
            if (!statement) {
                throw MerkError("Parser::parse: Null statement returned during parsing. Token: " + currentToken().toString());
            }
    
            // If interpretMode is enabled and interpretation is by block, evaluate each statement one-by-one, whether it is a control or variable
            if (interpretMode && byBlock){
                try {
                    interpret(statement.get());
                } catch(MerkError& e) {
                    rootBlock->printAST(std::cout, 0);
                    throw MerkError(e.what());
                }
                // DEBUG_LOG(LogLevel::INFO, "\n\n\n\n\n");
                // DEBUG_LOG(LogLevel::INFO, highlight("=========================== Interpreting AST by Block ===========================", Colors::bg_red));
                
                // DEBUG_LOG(LogLevel::INFO, highlight("=========================== Finished Interpreting AST by Block ===========================", Colors::bg_red));
                // DEBUG_LOG(LogLevel::INFO, "\n\n\n\n\n");
            }
            
            // Add the parsed statement to the block
            rootBlock->addChild(std::move(statement));
        }
    
    
        // If interpretMode is enabled and interpretation is not by block, evaluate the entire AST in a batch.
        if (interpretMode && !byBlock){
            // DEBUG_LOG(LogLevel::INFO, "\n\n\n\n\n");
            // DEBUG_LOG(LogLevel::INFO, highlight("=========================== Interpreting Full AST ===========================", Colors::bg_red));
    
    
            // rootBlock->printAST(std::cout);
            rootBlock->printAST(std::cout, 0);
            interpret(rootBlock.get());
            
            // DEBUG_LOG(LogLevel::INFO, highlight("=========================== Finished Interpreting Full AST ===========================", Colors::bg_red));
            // DEBUG_LOG(LogLevel::INFO, "\n\n\n\n\n");
            
            // currentScope->printContext();
            // DEBUG_LOG(LogLevel::INFO, "\n\n\n");
            // currentScope->debugPrint();
        }
    
        // DEBUG_LOG(LogLevel::INFO, "Returning CodeBlock from parse() with scope address: ", currentScope.get(), ", at block address: ", rootBlock.get());
        
        // rootBlock->printAST(std::cout);
        // DEBUG_LOG(LogLevel::INFO, "\n\n\n\n\n");
        // rootBlock->printAST(std::cout);
        // DEBUG_FLOW_EXIT();
        return std::move(rootBlock); // Return the parsed block node
    } catch (MerkError& e) {
        rootScope->printChildScopes();
        throw MerkError(e.what());
    }
    
}


bool Parser::getAllowScopeCreation() const {return allowScopecreation;}

// Enter a new scope
void Parser::enterScope(SharedPtr<Scope> manualScope) {
    SharedPtr<Scope> thisScope = manualScope ? manualScope : currentScope;
    // DEBUG_FLOW(FlowLevel::HIGH);
    // DEBUG_LOG(LogLevel::DEBUG, "allowScopeCreation:", highlight(getAllowScopeCreation() ? "true" : "false", Colors::red));

    if (!thisScope) {
        throw MerkError("Cannot enter a new scope because currentScope is null.");
    }

    if (getAllowScopeCreation()){
        // DEBUG_LOG(LogLevel::TRACE, "\n=========================================================================================");
        // DEBUG_LOG(LogLevel::TRACE, "DEBUG: Entering new Scope. Current Level: ", currentScope->getScopeLevel(), 
        //          " | New Scope Level: ", thisScope->getScopeLevel() + 1);

        // Create a child scope and update the currentScope
        auto newScope = thisScope->createChildScope();

        // DEBUG_LOG(LogLevel::TRACE, "DEBUG: New Scope Created. Scope Address: ", thisScope, 
        //          " | Parent Scope Address: ", thisScope->getParent());
        // debugLog(true, "=========================================================================================\n");
    
        currentScope = newScope;
        if (!currentScope) {
            throw MerkError("Cannot enter this newly created scope because it is null.");
        }
        currentScopeLevel++;
    }
    
    // DEBUG_FLOW_EXIT();
}

// Exit the current scope
void Parser::exitScope(SharedPtr<Scope> manualScope) {
    SharedPtr<Scope> thisScope = manualScope ? manualScope : currentScope;

    DEBUG_FLOW(FlowLevel::HIGH);
    if (getAllowScopeCreation()){
        if (!currentScope) {
            throw MerkError("Cannot exit scope because currentScope is null.");
        }
        
        DEBUG_LOG(LogLevel::TRACE, "DEBUG: Exiting Scope. Returning to Parent Level: ", 
                 currentScope->getParent()->getScopeLevel(), 
                 " | Current Scope Level: ", currentScope->getScopeLevel());
   
        // Get the parent scope
        auto parentScope = currentScope->getParent();
        if (!parentScope) {
            throw MerkError("Cannot exit scope because the current scope has no parent.");
        }
    
        currentScope = parentScope;
        if (!currentScope) {
            throw MerkError("Cannot exit the newly created scope because it is null.");
        }

        DEBUG_LOG(LogLevel::TRACE, "DEBUG: Scope Restored. Now at Level: ", currentScope->getScopeLevel(), 
                " | Scope Address: ", currentScope);
        DEBUG_LOG(LogLevel::TRACE, "=========================================================================================\n");
    
        currentScopeLevel--;
    }
    
    DEBUG_FLOW_EXIT();

}

void Parser::interpret(CodeBlock* block) const {
    // DEBUG_FLOW(FlowLevel::HIGH);

    // DEBUG_LOG(LogLevel::INFO, highlight("=========================== Interpreting CodeBlock ===========================", Colors::yellow));

    try {
        block->evaluate(); // Evaluate the updated statement
    } catch (const std::exception& e) {
        DEBUG_LOG(LogLevel::INFO, "DEBUG Parser::parse: Runtime Error during evaluation: ", e.what());
        throw MerkError(e.what());
    }
    // DEBUG_LOG(LogLevel::INFO, highlight("=========================== Finished Interpreting CodeBlock ===========================", Colors::yellow));

    // DEBUG_FLOW_EXIT();
}

void Parser::interpret(ASTStatement* ASTStatement) const {
    // DEBUG_FLOW(FlowLevel::HIGH);

    // DEBUG_LOG(LogLevel::INFO, highlight("=========================== Interpreting ASTStatement ===========================", Colors::yellow));

    try {
        ASTStatement->evaluate(currentScope); // Evaluate the updated statement

        // if (debugParser){
        //     currentScope->printContext();
        //     currentScope->debugPrint();
        // }

    } catch (const std::exception& e) {
        // DEBUG_LOG(LogLevel::INFO, "DEBUG Parser::parse: Runtime Error during evaluation: ", e.what());
        throw MerkError(e.what());
    }
    // DEBUG_LOG(LogLevel::INFO, highlight("=========================== Finished Interpreting ASTStatement ===========================", Colors::yellow));
    // DEBUG_FLOW_EXIT();
}

void Parser::interpret(BaseAST* block_or_ast) const {
    // DEBUG_FLOW(FlowLevel::HIGH);

    // DEBUG_LOG(LogLevel::INFO, highlight("=========================== Interpreting BaseAST ===========================", Colors::yellow));
    try {
        if (block_or_ast->getBranch() == "Block"){
            block_or_ast->evaluate(); // Evaluate the updated statement
        } else if (block_or_ast->getBranch() == "AST"){
            block_or_ast->evaluate(currentScope);
        } else {
            block_or_ast->evaluate();
        }

        // if (debugParser){
        //     currentScope->printContext();
        //     currentScope->debugPrint();
        // }

    } catch (const std::exception& e) {
        DEBUG_LOG(LogLevel::INFO, "DEBUG Parser::parse: Runtime Error during evaluation: ", e.what());
        throw MerkError(e.what());
    }
    // DEBUG_LOG(LogLevel::INFO, highlight("=========================== Finished Interpreting BaseAST ===========================", Colors::yellow));

    // DEBUG_FLOW_EXIT();
}


Token Parser::currentToken() const {
    if (position < tokens.size()) {
        return tokens[position];
    }
    return eofToken;
}

Token Parser::advance() {
    DEBUG_LOG(LogLevel::TRACE, "Advancing From Current Token: ", currentToken().toString());
    if (position < tokens.size() - 1) {
        ++position;

    } else if (position == tokens.size() - 1) {
        ++position;
        DEBUG_LOG(LogLevel::TRACE, "DEBUG Parser advance(): Reached EOF.");
        return eofToken;
    }
    DEBUG_LOG(LogLevel::TRACE, "New Current Token: ", currentToken().toString());

    return currentToken();
}

Token Parser::peek(int number){
    // ++position;
    position += number;
    Token nextToken = currentToken();
    position -= number;
    // --position;
    return nextToken;
}

bool Parser::consume(TokenType type) {
    if (match(type)) { // match only checks the type here because no value is provided
        // return previousToken(); // Return the matched token
        advance();
        return true;
    }
    throw UnexpectedTokenError(currentToken(), "Expected Type: " + tokenTypeToString(type));
}

bool Parser::consume(String value) {
    if (currentToken().value == value){
        advance();
        return true;
    }
    throw UnexpectedTokenError(currentToken(), value);
}


bool Parser::consume(TokenType type, String val1, String val2, String val3) {
    Token token = currentToken();
    String val = token.value;
    if (token.type == type && (val == val1 || val == val2 || val == val3)){
        advance();
        return true;
    }

    String msg = val1;
    if (!val2.empty()){
        msg += " | " + val2;
        if (!val3.empty()){
            msg += " | " + val3;
        }
    }
    throw UnexpectedTokenError(token, msg);
}


bool Parser::check(TokenType type, const String& value) const {
    return currentToken().type == type && (value.empty() || currentToken().value == value);
}

bool Parser::match(TokenType type, const String& value) {
    if (currentToken().type == type && (value.empty() || currentToken().value == value)) {
        advance(); // Consume the token
        return true;
    }
    return false;
};




bool Parser::existing(TokenType type, int limit){
    int currentPosition = position;
    Token lastToken = currentToken();
    int count = 0;
    while (lastToken.type != eofToken.type && count < limit) {
        ++position;
        ++count;
        lastToken = currentToken();
        if (lastToken.type == type) {
            position = currentPosition;  // Restore original position.
            return true;
        }
    }
    position = currentPosition;
    return false;
}


Token Parser::find(TokenType type, int limit) {
    int currentPosition = position;
    Token lastToken = currentToken();
    int count = 0;
    while (lastToken.type != eofToken.type && count < limit) {
        ++position;
        ++count;

        lastToken = currentToken();
        if (lastToken.type == type) {
            position = currentPosition;  // Restore original position.
            return lastToken;
        }
    }

    position = currentPosition;
    String msg = " Expected token type " + tokenTypeToString(type) + ", but found " + lastToken.toString();
    throw RunTimeError(msg);
}



Token Parser::previousToken() const {
    if (position > 0) {
        return tokens[position - 1];
    }
    throw MerkError("No previous token available. Token: " + currentToken().toString());
}

String Parser::getCurrentClassAccessor() {
    return classAccessors.back();
}

void Parser::addAccessor(String accessorName) {
    classAccessors.emplace_back(accessorName);
}
void Parser::popAccessor() {
    if (classAccessors.size() > 0){
        classAccessors.pop_back();
    }
}

template <typename T, typename... Args>
UniquePtr<T> createNode(SharedPtr<Scope> scope, Args&&... args) {
    return makeUnique<T>(std::forward<Args>(args)..., scope);
}

int Parser::getOperatorPrecedence(const String& op) const {
    static const std::unordered_map<String, int> precedenceMap = {
        {"*", 3}, {"/", 3}, {"%", 3},
        {"+", 2}, {"-", 2},
        {"==", 1}, {"!=", 1}, {"<", 1}, {">", 1}, {"<=", 1}, {">=", 1}
    };

    auto it = precedenceMap.find(op);
    if (it != precedenceMap.end()) {
        return it->second;
    }
    return 0; // Default precedence for unknown operators
}

std::optional<std::type_index> getType(Token token){
    switch (token.type)
    {
    case TokenType::Number:
        return typeid(int);
    
    case TokenType::String:
        return typeid(String);

    case TokenType::Bool:
        return typeid(bool);
    
    default:
        return typeid(nullptr);
    }

}

std::optional<NodeValueType> Parser::getTypeFromString(String typeStr) {
    if (typeStr == "Int") return NodeValueType::Int;
    if (typeStr == "Float") return NodeValueType::Float;
    if (typeStr == "Double") return NodeValueType::Double;
    if (typeStr == "Long") return NodeValueType::Long;
    if (typeStr == "Bool") return NodeValueType::Bool;
    if (typeStr == "Char") return NodeValueType::Char;
    if (typeStr == "String") return NodeValueType::String;
    if (typeStr == "Vector") return NodeValueType::Vector;
    if (typeStr == "Function") return NodeValueType::Function;
    if (typeStr == "Class") return NodeValueType::Class;
    if (typeStr == "Method") return NodeValueType::Method;
    if (typeStr == "Null") return NodeValueType::Null;
    if (typeStr == "Any") return NodeValueType::Any;
    else {
        return std::nullopt;
    }
}

std::optional<NodeValueType> Parser::parseStaticType() {
    if (currentToken().value != ":") {
        return std::nullopt; // No type annotation found
    }
    advance(); // Consume ':'

    Token typeToken = currentToken();
    if (typeToken.type != TokenType::Type) {
        throw SyntaxError("Expected type name after ':'.", currentToken());
    }

    String typeStr = typeToken.value;
    advance(); // Consume type name

    // Convert string to NodeValueType
    return getTypeFromString(typeStr);
    // throw SyntaxError("Unknown type annotation: " + typeStr, typeToken);
}

void Parser::processIndent(SharedPtr<Scope> manualScope){
    TokenType type = currentToken().type;
    if (type == TokenType::Indent) {
        advance(); // Consume Indent Token
        enterScope(manualScope);
    } else {
        throw InvalidIndentationError(currentToken());
    }
}
void Parser::processDedent(SharedPtr<Scope> manualScope) {
    TokenType type = currentToken().type;

    if (type == TokenType::Dedent){
        advance();  // Consume Dedent Token
        exitScope(manualScope);
        // type = currentToken().type;
    } else {
        throw UnclosedBlockError(currentToken());
    }
}
bool Parser::processNewLines(){
    int count = 0;
    while (currentToken().type == TokenType::Newline){
        advance(); //Consume NewLine Token
        count += 1;
    }

    return count > 0;
};


void Parser::reinjectControlToken(const Token& token) {
    tokens.insert(tokens.begin() + position, token);
    DEBUG_LOG(LogLevel::PERMISSIVE, "Reinjected token at position ", position, ": ", token.toColoredString());

}

void Parser::processBlankSpaces() {
    // placeholder
};


bool Parser::expect(TokenType tokenType, bool strict) {
    if (currentToken().type == tokenType){
        return true;
    }

    if (strict){
        throw UnexpectedTokenError(currentToken(), tokenTypeToString(tokenType));
    }
    
    return false;
}


void Parser::displayNextTokens(String baseTokenName, size_t number, String location) {
    DEBUG_LOG(LogLevel::PERMISSIVE, baseTokenName, " Token For ", location, " Is: ", currentToken().toColoredString());
    for (size_t i = 1; i < number + 1; i++) {
        DEBUG_LOG(LogLevel::PERMISSIVE, baseTokenName, " Token For ", location, " Is: ", peek(i).toColoredString());
    }
}