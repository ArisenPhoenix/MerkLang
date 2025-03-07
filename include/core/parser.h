#ifndef PARSER_H
#define PARSER_H
 
#include <vector>
#include <memory>
#include <string>

#include "core/types.h"
#include "core/errors.h"
#include "core/node.h"
#include "core/tokenizer.h"
#include "ast/ast.h"
#include "ast/ast_control.h"
#include "ast/ast_function.h"

class Parser {
private:
    const Vector<Token>& tokens;
    size_t position = 0;
    SharedPtr<Scope> rootScope;       // Root scope of the parser
    SharedPtr<Scope> currentScope;    // Current active scope for tracking

    SharedPtr<Scope> detachedAtScope; // For storing the main branch of scope from functions/classes' branches.

    int currentScopeLevel = 0;        // Track the current scope level
    int loopContextCounter = 0;       // Tracks the depth of nested loops

    UniquePtr<CodeBlock> rootBlock;

    Token eofToken = Token(TokenType::EOF_Token, "EOF", 0, 0); // Singleton EOF token
   
    Token currentToken() const;
    Token advance();

    Token consume(TokenType type, const String& errorMessage) {
        if (match(type)) { // match only checks the type here because no value is provided
            return previousToken(); // Return the matched token
        }
        throw std::runtime_error(errorMessage + " at line " +
                                std::to_string(currentToken().line) +
                                ", column " + std::to_string(currentToken().column));
    }
    Token peek();
    Token previousToken() const;
    bool check(TokenType type, const String& value = "") const {
        return currentToken().type == type && (value.empty() || currentToken().value == value);
    }
    bool match(TokenType type, const String& value = "") {
        if (currentToken().type == type && (value.empty() || currentToken().value == value)) {
            advance(); // Consume the token
            return true;
        }
        return false;
    };
    int getOperatorPrecedence(const String& op) const;

    // Scope Management
    void enterScope(SharedPtr<Scope> manualScope = nullptr);  // Enter a new child scope
    void exitScope(SharedPtr<Scope> manualScope = nullptr);   // Exit the current scope
     
    // Components
    UniquePtr<ASTStatement> parseVariableDeclaration();
    UniquePtr<ASTStatement> parseVariableAssignment();
    UniquePtr<ASTStatement> parseExpression();
    UniquePtr<ASTStatement> parseBinaryExpression(int precedence); 
    UniquePtr<ASTStatement> parsePrimaryExpression();
    UniquePtr<ASTStatement> parseBreakStatement();

    UniquePtr<IfStatement> parseIfStatement();
    UniquePtr<WhileLoop> parseWhileLoop();

    UniquePtr<CodeBlock> parseBlock(SharedPtr<Scope> = nullptr);
    UniquePtr<BaseAST> parseStatement();

    std::optional<std::type_index> getStaticType();
    void interpret(CodeBlock* CodeBlockForEvaluation) const;
    void interpret(ASTStatement* CodeBlockForEvaluation) const;
    void interpret(BaseAST* ASTStatementForEvaluation) const;

    std::optional<NodeValueType> parseStaticType();

    UniquePtr<FunctionDef> parseFunctionDefinition();
    UniquePtr<FunctionCall> parseFunctionCall();

    UniquePtr<ASTStatement> parseReturnStatement();

    void processIndent(SharedPtr<Scope> manualScope = nullptr);
    void processDedent(SharedPtr<Scope> manualScope = nullptr);
    bool processNewLines();
    void processBlankSpaces();

    bool expect(TokenType tokenType, bool strict = false);
    
    bool allowScopecreation = true;

public:
    explicit Parser(const Vector<Token>& tokens, SharedPtr<Scope> scope, bool interpretMode=true, bool byBlock=false);
    Vector<String> nativeFunctionNames;
    UniquePtr<CodeBlock> parse();
    bool interpretMode;
    bool byBlock;
    
    // Check if the parser is currently inside a loop
    bool isInsideLoop() const {
        return loopContextCounter > 0;
    }
    // Increment the loop context counter (called when entering a loop)
    void enterLoop() {
        ++loopContextCounter;
    }

    // Decrement the loop context counter (called when exiting a loop)
    void exitLoop() {
        if (loopContextCounter > 0) {
            --loopContextCounter;
        } else {
            throw MerkError("Unexpected loop context underflow. This indicates a parser logic error.");
        }
    }

    void setAllowScopeCreation(bool allow) {
        allowScopecreation = allow;
    }
    bool getAllowScopeCreation() const;
    
};

#endif // PARSER_H