#pragma once
 
#include <vector>
#include <memory>
#include <string>

#include "core/TypesFWD.hpp"
#include "ast/Ast.hpp"
#include "core/node/Node.hpp"

#include "ast/AstControl.hpp"
#include "ast/AstChain.hpp"

#include "core/Tokenizer.hpp"
#include "ast/AstClass.hpp"

class Parser {
private:
    Vector<Token>& tokens;
    size_t position = 0;
    SharedPtr<Scope> rootScope;       // Root scope of the parser
    SharedPtr<Scope> currentScope;    // Current active scope for tracking
    SharedPtr<Scope> detachedAtScope; // For storing the main branch of scope from functions/classes' branches.

    int currentScopeLevel = 0;        // Track the current scope level
    int loopContextCounter = 0;       // Tracks the depth of nested loops
    bool allowScopecreation = true;
    bool insideClass = false;

    Vector<String> classAccessors;
    UniquePtr<CodeBlock> rootBlock;

    Token eofToken = Token(TokenType::EOF_Token, "EOF", 0, 0); // Singleton EOF token
    Token noOpToken = Token(TokenType::NoOp, "NoOp", 0, 0);
    Token currentToken() const;
    Token advance();

    bool consume(TokenType type, String value, String fromWhere = "Parser::consume");
    bool consume(TokenType type, String fromWhere = "Parser::consume");
    bool consume(String value, String fromWhere = "Parser::consume");
    bool consume(TokenType type, Vector<String> values = {}, String fromWhere = "Parser::consume");
    Token peek(int number = 1);
    Token lookBack(int number = 1);
    Token find(TokenType type, int limit);
    bool existing(TokenType type, int limit);
    Token previousToken() const;
    bool check(TokenType type, const String& value = "") const;
    bool consumeIf(TokenType type, const String& value = "");
    
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
    
    std::optional<NodeValueType> getTypeFromString(String typeStr);
    void interpretFlow(BaseAST* CodeBlockForEvaluation) const;
    void interpretFlow(CodeBlock* CodeBlockForEvaluation) const;
    void interpret(BaseAST* ASTStatementForEvaluation) const;
    void interpret(CodeBlock* block) const;


    std::optional<NodeValueType> parseStaticType();

    UniquePtr<ASTStatement> parseClassCall();
    UniquePtr<ASTStatement> parseClassLiteralCall();
    UniquePtr<ASTStatement> parseClassDefinition();

    UniquePtr<ASTStatement> parseProtectedClassAttributes();
    UniquePtr<ASTStatement> parseClassAttributes();

    UniquePtr<FunctionDef> parseFunctionDefinition();
    UniquePtr<FunctionCall> parseFunctionCall();

    UniquePtr<MethodDef> parseClassInitializer();
    UniquePtr<MethodDef> parseClassMethod();

    UniquePtr<NameSpace> parseNameSpace();

    // UniquePtr<Chain> parseChain(bool isDeclaration = false, bool isConst = false);

    UniquePtr<ChainOperation> parseChainOp(UniquePtr<ASTStatement> stmnt = nullptr);
    String getCurrentClassAccessor();

    void addAccessor(String accessorName);
    void popAccessor();
    
    UniquePtr<ASTStatement> parseReturnStatement();
    UniquePtr<ASTStatement> parseContinueStatement();

    void processIndent(SharedPtr<Scope> manualScope = nullptr);
    void processDedent(SharedPtr<Scope> manualScope = nullptr);
    bool processNewLines();
    void processBlankSpaces();

    bool expect(TokenType tokenType, bool strict = false, String fromWhere = "Parser::expect");

    ParamList handleParameters(TokenType type = TokenType::FunctionDef);
    Vector<UniquePtr<ASTStatement>> parseArguments();
    UniquePtr<Arguments> parseAnyArgument();
    void reinjectControlToken(const Token& token); // for use with Chain to implement the controlling structure and allow parsing without modifications to architecture
    void displayPreviousTokens(String baseTokenName, size_t number = 4, String location = "Parser");
    void displayNextTokens(String baseTokenName, size_t number = 4, String location = "Parser");
    ResolvedType parseResolvedType();
    bool validate(Token, Vector<TokenType> types = {}, Vector<String> values = {}, bool requiresBoth = false);
    bool validate(Vector<TokenType>, Vector<String>, bool requiresBoth = false);

    UniquePtr<ASTStatement> parseThrowStatement();
    UniquePtr<ASTStatement> parseKeyWord();
    

public:
    explicit Parser(Vector<Token>& tokens, SharedPtr<Scope> scope, bool interpretMode=true, bool byBlock=false);
    Vector<String> nativeFunctionNames;
    UniquePtr<CodeBlock> parse();
    bool interpretMode;
    bool byBlock;
    
    // Check if the parser is currently inside a loop
    bool isInsideLoop() const { return loopContextCounter > 0; }
    // Increment the loop context counter (called when entering a loop)
    void enterLoop() { ++loopContextCounter; }

    // Decrement the loop context counter (called when exiting a loop)
    void exitLoop();

    void setAllowScopeCreation(bool allow) {  allowScopecreation = allow; }
    bool getAllowScopeCreation() const;
    
};
