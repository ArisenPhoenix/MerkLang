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
#include "ast/ast_chain.h"
#include "ast/ast_control.h"
#include "ast/ast_function.h"
// #include "ast/ast_import.h"
#include "ast/ast_callable.h"
#include "ast/ast_class.h"

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
   
    Token currentToken() const;
    Token advance();

    bool consume(TokenType type);
    bool consume(String value);
    bool consume(TokenType type, String val1, String val2 = "", String val3 = "");
    Token peek(int number = 1);
    Token find(TokenType type, int limit);
    bool existing(TokenType type, int limit);
    Token previousToken() const;
    bool check(TokenType type, const String& value = "") const;
    bool match(TokenType type, const String& value = "");
    
    int getOperatorPrecedence(const String& op) const;
    std::tuple<bool, bool, String> handleIsVarDec();

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

    UniquePtr<ASTStatement> parseClassCall();
    UniquePtr<ASTStatement> parseClassDefinition();
    UniquePtr<MethodDef> parseClassInitializer();
    UniquePtr<MethodDef> parseClassMethod();

    UniquePtr<ASTStatement> parseProtectedClassAttributes();
    UniquePtr<ASTStatement> parseClassAttributes();
    UniquePtr<Chain> parseChain(bool isDeclaration = false);
    UniquePtr<ChainOperation> parseChainOp();
    String getCurrentClassAccessor();

    void addAccessor(String accessorName);
    void popAccessor();
    
    // UniquePtr<ASTStatement> parseClassMembers();

    UniquePtr<ASTStatement> parseReturnStatement();
    UniquePtr<ASTStatement> parseContinueStatement();

    void processIndent(SharedPtr<Scope> manualScope = nullptr);
    void processDedent(SharedPtr<Scope> manualScope = nullptr);
    bool processNewLines();
    void processBlankSpaces();
    // ParamList parseParameters(TokenType type = TokenType::FunctionDef);

    bool expect(TokenType tokenType, bool strict = false);

    Token handleAttributNotation();
    ParamList handleParameters(TokenType type = TokenType::FunctionDef);
    void reinjectControlToken(const Token& token); // for use with Chain to implement the controlling structure and allow parsing without modifications to architecture
    // For Future Implementation
    // UniquePtr<ImportStatement> parseImport();
    // Vector<UniquePtr<ImportStatement>> parseImports();
    

public:
    explicit Parser(Vector<Token>& tokens, SharedPtr<Scope> scope, bool interpretMode=true, bool byBlock=false);
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