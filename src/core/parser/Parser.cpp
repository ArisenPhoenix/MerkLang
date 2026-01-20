#include <stdexcept>
#include <iostream>
#include <string>

#include "core/types.h"
#include "core/errors.h"
#include "core/node/Node.hpp"

#include "ast/AstBase.hpp"
#include "ast/Ast.hpp"
#include "ast/AstControl.hpp"
#include "core/Scope.hpp"

#include "utilities/utilities.h"
#include "utilities/streaming.h"

#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"
#include "core/Parser.hpp"



// Constructor
Parser::Parser(Vector<Token>& tokens, SharedPtr<Scope> rootScope, bool interpreting, bool byBlock)
    : tokens(tokens), rootScope(rootScope), currentScope(rootScope), interpretMode(interpreting), byBlock(byBlock) {
        // DEBUG_FLOW(FlowLevel::HIGH);
        if (!rootScope) {
            throw MerkError("Root scope must not be null when initializing the parser.");
        }
        rootBlock = makeUnique<CodeBlock>(rootScope);
        currentScope = rootScope->createChildScope();
        if (!currentScope){
            throw MerkError("Initial Scope is null");
        }
}

UniquePtr<CodeBlock> Parser::parse() {
    // DEBUG_FLOW(FlowLevel::HIGH);

    if (!currentScope){ throw MerkError("Initial Scope is null"); }
    if (tokens[0].type == TokenType::SOF_Token) {
        advance();
    }

    try {
        setAllowScopeCreation(false);
        while (currentToken().type != TokenType::EOF_Token) {
            processNewLines();
            if (currentToken().type == eofToken.type) { break; }

            auto statement = parseStatement();
            if (!statement) { throw MerkError("Parser::parse: Null statement returned during parsing. Token: " + currentToken().toString()); }
            if (interpretMode && byBlock){
                setAllowScopeCreation(true);
                try {
                    DEBUG_LOG(LogLevel::PERMISSIVE, highlight("EVALUATING AST TYPE: " + statement->getAstTypeAsString(), Colors::orange));
                    // interpret(statement.get());
                    interpretFlow(statement.get());
                } catch(MerkError& e) {
                    // rootBlock->printAST(std::cout, 0);
                    throw MerkError(e.what());
                }
            }
            rootBlock->addChild(std::move(statement));
        }
    
        setAllowScopeCreation(true);
        if (interpretMode && !byBlock){
            // interpret(rootBlock.get());
            interpretFlow(rootBlock.get());
        }

        return std::move(rootBlock); // Return the parsed block node
    } catch (MerkError& e) {
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

void Parser::interpretFlow(CodeBlock* codeBlock) const {
    try {
        codeBlock->evaluateFlow(); // Evaluate the updated statement

    } catch (const std::exception& e) {
        throw MerkError(e.what());
    }
  
}

void Parser::interpretFlow(BaseAST* ASTStatement) const {
    // DEBUG_FLOW(FlowLevel::HIGH);

    // DEBUG_LOG(LogLevel::INFO, highlight("=========================== Interpreting ASTStatement ===========================", Colors::yellow));
    // if (interpretMode) {

    // }
    try {
        ASTStatement->evaluateFlow(currentScope); // Evaluate the updated statement

        // if (debugParser){
        //     currentScope->printContext();
        //     currentScope->debugPrint();
        // }

    } catch (const std::exception& e) {
        // DEBUG_LOG(LogLevel::INFO, "DEBUG Parser::parse: Runtime Error during evaluation: ", e.what());
        // ASTStatement->printAST(std::cout, 0);
        throw MerkError(e.what());
    }
    // DEBUG_LOG(LogLevel::INFO, highlight("=========================== Finished Interpreting ASTStatement ===========================", Colors::yellow));
    // DEBUG_FLOW_EXIT();
}

void Parser::interpret(CodeBlock* block) const {
    // DEBUG_FLOW(FlowLevel::HIGH);

    // DEBUG_LOG(LogLevel::INFO, highlight("=========================== Interpreting CodeBlock ===========================", Colors::yellow));

    try {
        block->evaluate(); // Evaluate the updated statement
    } catch (const std::exception& e) {
        DEBUG_LOG(LogLevel::INFO, "DEBUG Parser::parse: Runtime Error during evaluation: ", e.what());
        // block->printAST(std::cout, 0);
        throw MerkError(e.what());
    }
    // DEBUG_LOG(LogLevel::INFO, highlight("=========================== Finished Interpreting CodeBlock ===========================", Colors::yellow));

    // DEBUG_FLOW_EXIT();
}

void Parser::interpret(BaseAST* block_or_ast) const {
    // DEBUG_FLOW(FlowLevel::HIGH);

    // DEBUG_LOG(LogLevel::INFO, highlight("=========================== Interpreting BaseAST ===========================", Colors::yellow));
    try {
       block_or_ast->evaluate(currentScope); // Evaluate the updated statement

    } catch (const std::exception& e) {
        DEBUG_LOG(LogLevel::INFO, "DEBUG Parser::parse: Runtime Error during evaluation: ", e.what());
        // block_or_ast->printAST(std::cout, 0);
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


Token Parser::lookBack(int number) {
    position -= number;
    Token last = currentToken();
    position += number;
    return last;
}

Token Parser::peek(int number){
    // ++position;
    position += number;
    Token nextToken = currentToken();
    position -= number;
    // --position;
    return nextToken;
}

bool Parser::consume(TokenType type, String value, String fromWhere) {
    if (currentToken().type == type && currentToken().value == value) { // match only checks the type here because no value is provided
        advance();
        return true;
    }

    throw UnexpectedTokenError(currentToken(), " Type: " + tokenTypeToString(type), " Parser::consume -> " + fromWhere);
}

bool Parser::consume(TokenType type, String fromWhere) {
    if (currentToken().type == type) { // match only checks the type here because no value is provided
        advance();
        return true;
    }
    throw UnexpectedTokenError(currentToken(), " Type: " + tokenTypeToString(type), " Parser::consume -> " + fromWhere);
}

bool Parser::consume(String value, String fromWhere) {
    if (currentToken().value == value){
        advance();
        return true;
    }
    throw UnexpectedTokenError(currentToken(), value, " Parser::consume -> " + fromWhere);
}


bool Parser::consume(TokenType type, Vector<String> values, String fromWhere) {
    Token token = currentToken();
    String val = token.value;
    if (token.type == type){
        if (values.size() > 0) {
            for (auto& value : values) {
                if (val == value) {
                    advance();
                    return true;
                }
            }

        } else {
            advance();
            return true;
        }
    }

    String msg = tokenTypeToString(type) + " | " + joinVectorStrings(values);

    displayPreviousTokens(token.value, 5, "Parser::consume");
    displayNextTokens(token.value, 5, "Parser::consume");
    throw UnexpectedTokenError(token, msg, "Parser::consume -> " + fromWhere);
}


bool Parser::check(TokenType type, const String& value) const {
    return currentToken().type == type && (value.empty() || currentToken().value == value);
}

bool Parser::consumeIf(TokenType type, const String& value) {
    if (check(type, value)) {
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
    throw MerkError(msg);
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

// int Parser::getOperatorPrecedence(const String& op) const {
//     static const std::unordered_map<String, int> precedenceMap = {
//         {"*", 3}, {"/", 3}, {"%", 3},
//         {"+", 2}, {"-", 2}, {"+=", 2}, {"++", 2},
//         {"==", 1}, {"!=", 1}, {"<", 1}, {">", 1}, 
//         {"<=", 1}, {">=", 1}, {"+=", 1}, {"-=", 1},
//         {"*=", 1}, {"/=", 1}

//         // {"+=", 0}, {"-=", 0}, {"*=", 0}, {"/=", 0},
//         // {"*", 3}, {"/", 3}, {"%", 3},
//         // {"+", 2}, {"-", 2},
//         // {"==", 1}, {"!=", 1}, {"<", 1}, {">", 1}, {"<=", 1}, {">=", 1}
//     };

//     auto it = precedenceMap.find(op);
//     if (it != precedenceMap.end()) {
//         return it->second;
//     }
//     return 0; // Default precedence for unknown operators
// }

int Parser::getOperatorPrecedence(const String& op) const {
    static const std::unordered_map<String, int> precedenceMap = {
        {"*", 3}, {"/", 3}, {"%", 3},
        {"+", 2}, {"-", 2},
        {"==", 1}, {"!=", 1}, {"<", 1}, {">", 1}, {"<=", 1}, {">=", 1},

        // If you truly want these as binary operators, keep them,
        // otherwise they should NOT be here:
        {"+=", 0}, {"-=", 0}, {"*=", 0}, {"/=", 0},
        {"++", 4}, // (but ++ is usually unary/postfix; see below)
    };

    auto it = precedenceMap.find(op);
    if (it != precedenceMap.end()) return it->second;
    return -1; // ✅ unknown: not a binary operator
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
    auto val = stringToNodeType(typeStr);
    if (val == NodeValueType::UNKNOWN) {
        // This will be Where user defined types are pulled from scope
        // throw MerkError("Unknown Type");
        return NodeValueType::UNKNOWN;
    }
    else {return std::nullopt;}
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
    while (currentToken().type == TokenType::Newline || currentToken().type == TokenType::Comment){
        advance(); //Consume NewLine Token
        count += 1;
    }

    return count > 0;
};


void Parser::reinjectControlToken(const Token& token) {
    tokens.insert(tokens.begin() + position, token);
}

void Parser::processBlankSpaces() {
    // placeholder
};


bool Parser::expect(TokenType tokenType, bool strict, String fromWhere) {
    if (currentToken().type == tokenType){
        return true;
    }

    if (strict){
        throw UnexpectedTokenError(currentToken(), tokenTypeToString(tokenType), fromWhere);
    }
    
    return false;
}


void Parser::displayPreviousTokens(String baseTokenName, size_t number, String location) {
    for (size_t i = number + 1; i > 1; i--) {
        if (int(position - i) < 0) { continue; }
        debugLog(true, baseTokenName, " Token For ", location, " Is: ", lookBack(i).toColoredString(), "back: ", i-1);
    }
    debugLog(true, "Primary " + baseTokenName, " Token For ", location, " Is: ", currentToken().toColoredString());
}


void Parser::displayNextTokens(String baseTokenName, size_t number, String location) {
    debugLog(true, "Primary " + baseTokenName, " Token For ", location, " Is: ", currentToken().toColoredString());
    for (size_t i = 1; i < number + 1; i++) {
        debugLog(true, baseTokenName, " Token For ", location, " Is: ", peek(i).toColoredString());
    }
}


ResolvedType Parser::parseResolvedType() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    if (consumeIf(TokenType::Operator, "<")) {
        auto inner = parseResolvedType();
        consume(TokenType::Operator, ">", "Parser::parseResolvedType -> Operator");
        return ResolvedType("Array", { inner });
    }

    if (consumeIf(TokenType::LeftBracket, "[")) {
        auto inner = parseResolvedType();
        consume(TokenType::RightBracket, "]", "Parser::parseResolvedType -> RightBracket");
        return ResolvedType("List", { inner });
    }
    
    if (consumeIf(TokenType::Operator, "{")) {
        auto first = parseResolvedType();
        if (consumeIf(TokenType::Punctuation, ",")) {
            auto second = parseResolvedType();
            consume(TokenType::Operator, "}", "Parser::parseResolvedType -> Operator");
            return ResolvedType("Dict", { first, second });
        } else {
            consume(TokenType::Operator, "}", "Parser::parseResolvedType -> Operator");
            return ResolvedType("Set", { first });
        }
    }

    if (isTypeStart(currentToken().type)) {
        advance();
        return ResolvedType(previousToken().value);
    }
    if (consumeIf(TokenType::Type)) {
        DEBUG_FLOW_EXIT();
        return ResolvedType(previousToken().value);
    }
    DEBUG_FLOW_EXIT();
    throw MerkError("Invalid type annotation: " + currentToken().toColoredString());
}



bool Parser::validate(Token token, Vector<TokenType> types, Vector<String> values, bool requiresBoth) {
    for (auto type : types) {
        if (token.type == type) {
            if (requiresBoth) {break;}
            return true;
        }
    }
    for (auto& val : values) {
        if (token.value == val) {
            return true;
        }
    }

    return false;
}

bool Parser::validate(Vector<TokenType> types, Vector<String> values, bool requiresBoth) {
    return validate(currentToken(), types, values, requiresBoth);
}

void Parser::exitLoop() {
    if (loopContextCounter > 0) {
        --loopContextCounter;
    } else {
        throw MerkError("Unexpected loop context underflow. This indicates a parser logic error.");
    }
}