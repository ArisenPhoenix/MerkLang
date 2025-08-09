// In parse_classes.cpp (or wherever you put your class-parsing methods)

#include "core/node/argument_node.h"
#include "ast/ast_class.h"       // For ClassDef and ClassBody
#include "core/errors.h"
#include "utilities/helper_functions.h"
#include "utilities/debugging_functions.h"

#include "core/tokenizer.h"      // For token types
#include "core/scope.h"
#include "ast/ast_callable.h"
#include "ast/ast_method.h"
#include "core/parser.h"
#include "ast/exceptions.h"


ChainElement createChainElement(const Token& token, const Token& delim, UniquePtr<BaseAST> obj) {
    ChainElement elem;
    elem.name = token.value;
    elem.delimiter = delim.value;
    elem.type = token.type;
    elem.object = std::move(obj);

    if (elem.name == "var") {
        throw MerkError("Chain Element is named var");
    }

    if (elem.object->getAstType() == AstType::CallableCall) {
        auto temp = static_unique_ptr_cast<VariableReference>(elem.object->clone());
        throw MerkError("Cannot use CallableCall as a Chain Element");
    }
    return elem;
}


    

    
// UniquePtr<Chain> Parser::parseChain(bool isDeclaration, bool isConst) {
//     DEBUG_FLOW(FlowLevel::PERMISSIVE);
//     DEBUG_LOG(LogLevel::DEBUG, "Chain Is Parsing as a Declaration: ", isDeclaration);
    
//     // DEBUG_FLOW_EXIT();
//     // return chain;

//     auto chain = makeUnique<Chain>(currentScope);

//     // Base token (variable or function call)
//     Token baseToken = currentToken();
//     UniquePtr<BaseAST> baseExpr;

//     if (baseToken.type == TokenType::Variable || baseToken.type == TokenType::ChainEntryPoint) {
//         baseExpr = makeUnique<VariableReference>(baseToken.value, currentScope);
//         advance();
//     }
//     else if (baseToken.type == TokenType::FunctionCall) {
//         baseExpr = parseFunctionCall();
//     }
//     else {
//         throw UnexpectedTokenError(baseToken, "Variable or FunctionCall");
//     }

//     auto punct = currentToken();
//     chain->addElement(createChainElement(baseToken, punct, std::move(baseExpr)));

//     // Chain loop
//     while (check(TokenType::Punctuation, ".") || check(TokenType::Punctuation, "::")) {
//         Token delim = advance();
//         Token next = currentToken();
//         UniquePtr<BaseAST> rhs;

//         if (next.type == TokenType::FunctionCall) {
//             rhs = parseFunctionCall();
//         } else if (next.type == TokenType::ClassMethodCall) {
//             auto func = parseFunctionCall();
//             rhs = makeUnique<MethodCall>(func->getName(), std::move(func->arguments), func->getScope());
//         } else if (next.type == TokenType::Variable) {
//             rhs = makeUnique<VariableReference>(next.value, currentScope);
//             advance();
//         } else {
//             throw UnexpectedTokenError(next, "Expected variable or call after '.'");
//         }

//         if (check(TokenType::Punctuation, ".") || check(TokenType::Punctuation, "::")) {
//             delim = currentToken();  // Was Not The Last item
//         }

//         chain->addElement(createChainElement(next, delim, std::move(rhs)));
//     }

//     // Handle declaration or assignment
//     Token maybeAssign = currentToken();
//     if (maybeAssign.type == TokenType::VarAssignment) {
//         bool isMutable = maybeAssign.value == "=";
//         advance(); // consume '=' or ':='

//         auto rhsExpr = parseExpression();

//         auto &last = chain->getLast();
//         if (isDeclaration) {
//             DataTypeFlags flags(last.name, isConst, isMutable, false, ResolvedType("Any"));
//             last.object = makeUnique<VariableDeclaration>(last.name, flags, currentScope, std::move(rhsExpr));
//         } else {
//             last.object = makeUnique<VariableAssignment>(last.name, std::move(rhsExpr), currentScope);
//         }
//     }


//     if (chain->getElements().size() == 1) {throw MerkError("Chain Was not Built");}

//     return chain;
// }




UniquePtr<ChainOperation> Parser::parseChainOp(UniquePtr<ASTStatement> stmnt) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    
    bool isDeclaration = currentToken().type == TokenType::VarDeclaration;
    bool isConst = false;
    
    if (isDeclaration) {
        isConst = currentToken().value == "const";
        advance(); // consume declaration token.
    }


    auto chain = makeUnique<Chain>(currentScope);
    String value;
    if (stmnt) {
        auto delim = currentToken();
        advance(); //consume delim
        Token next(TokenType::Unknown, "Unknown", 0, 0);
        switch (stmnt->getAstType()) {
        case AstType::FunctionCall:
            next.type = TokenType::FunctionCall;
            break;
        case AstType::ClassCall:
            next.type = TokenType::ClassCall;
            break;
        case AstType::ClassMethodCall:
            next.type = TokenType::ClassMethodCall;
            break;
        default:
            throw MerkError(astTypeToString(stmnt->getAstType()) + " was passed by BinaryOperation");
    }
        next.value = static_cast<CallableCall*>(stmnt.get())->getName();

        chain->addElement(createChainElement(next, delim, std::move(stmnt)));
    }


    

    // Base token (variable or function call)
    Token baseToken = currentToken();
    UniquePtr<BaseAST> baseExpr;

    if (baseToken.type == TokenType::Variable || baseToken.type == TokenType::ChainEntryPoint || baseToken.type == TokenType::Argument) {
        baseExpr = makeUnique<VariableReference>(baseToken.value, currentScope);
        advance();
    } else if ( baseToken.type == TokenType::FunctionRef && (peek().type == TokenType::VarAssignment || peek().type == TokenType::Punctuation)) {
        baseExpr = makeUnique<VariableReference>(baseToken.value, currentScope);
        advance();
    }
    
    else if (baseToken.type == TokenType::FunctionCall) {
        baseExpr = parseFunctionCall();
    }
    else {
        throw UnexpectedTokenError(baseToken, "Variable or FunctionCall");
    }

    auto punct = currentToken();
    chain->addElement(createChainElement(baseToken, punct, std::move(baseExpr)));

    // Chain loop
    while (check(TokenType::Punctuation, ".") || check(TokenType::Punctuation, "::")) {
        Token delim = advance();
        Token next = currentToken();
        UniquePtr<BaseAST> rhs;

        if (next.type == TokenType::FunctionCall) {
            rhs = parseFunctionCall();
        } else if (next.type == TokenType::ClassMethodCall) {
            auto func = parseFunctionCall();
            rhs = makeUnique<MethodCall>(func->getName(), std::move(func->arguments), func->getScope());
        } else if (next.type == TokenType::Variable || next.type == TokenType::Argument || next.type == TokenType::ChainEntryPoint) {
            rhs = makeUnique<VariableReference>(next.value, currentScope);
            advance();
        } 
        else {
            throw UnexpectedTokenError(next, "Expected variable or call after '.'");
        }

        if (check(TokenType::Punctuation, ".") || check(TokenType::Punctuation, "::")) {
            delim = currentToken();  // Was Not The Last item
        }

        chain->addElement(createChainElement(next, delim, std::move(rhs)));
    }

    // Handle declaration or assignment
    Token maybeAssign = currentToken();
    if (maybeAssign.type == TokenType::VarAssignment) {
        bool isMutable = maybeAssign.value == "=";
        advance(); // consume '=' or ':='

        auto rhsExpr = parseExpression();

        auto &last = chain->getLast();
        if (isDeclaration) {
            DataTypeFlags flags(last.name, isConst, isMutable, false, ResolvedType("Any"));
            last.object = makeUnique<VariableDeclaration>(last.name, flags, currentScope, std::move(rhsExpr));
        } else {
            last.object = makeUnique<VariableAssignment>(last.name, std::move(rhsExpr), currentScope);
        }
    }


    if (chain->getElements().size() == 1) {throw MerkError("Chain Was not Built");}

    // return chain;

    // UniquePtr<Chain> lhs = parseChain(isDeclaration, isConst);  // Handles `a.b.c` chains



    // displayNextTokens(currentToken().value, 20, "Parser::parseChainOp");

    DEBUG_LOG(LogLevel::PERMISSIVE, "Got ChainOp");
    DEBUG_FLOW_EXIT();
    return makeUnique<ChainOperation>(
        std::move(chain),
        nullptr,
        ChainOpKind::Reference,
        currentScope
    );
}


// Skeleton: Parse protected member declarations in the class.
UniquePtr<ASTStatement> Parser::parseClassAttributes() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    
    auto varDec = parsePrimaryExpression();
    DEBUG_FLOW_EXIT();
    return varDec;
}

UniquePtr<ASTStatement> Parser::parseProtectedClassAttributes() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    auto varDec = parseVariableDeclaration();

    DEBUG_FLOW_EXIT();
    return varDec;
}

// Skeleton: Parse the constructor (named "construct").
UniquePtr<MethodDef> Parser::parseClassInitializer() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    // Expect the current token to be a method definition and that its name is "construct".
    Token token = currentToken();
    String methodName = peek().value;
    if (token.type != TokenType::ClassMethodDef || methodName != "construct") {
        throw SyntaxError("Class constructor must be defined as 'construct'.", token);
    }

    Token accessor = find(TokenType::Parameter, 6);
    DEBUG_LOG(LogLevel::DEBUG, "accessor value: ", accessor.value);
    auto constructorMethod = parseClassMethod();

    String accessorVal = String(accessor.value);
    DEBUG_LOG(LogLevel::DEBUG, "Accessor is being set");
    constructorMethod->setMethodAccessor(accessorVal);
    DEBUG_LOG(LogLevel::DEBUG, "Access Properly Set");
    DEBUG_FLOW_EXIT();
    return constructorMethod;
}


UniquePtr<MethodDef> Parser::parseClassMethod() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    auto functionDef = parseFunctionDefinition();
    auto methodName = functionDef->getName();

    auto methodDef = makeUnique<MethodDef>(std::move(functionDef));

    // UniquePtr<MethodDef> methodDef = makeUnique<MethodDef>(
    //     methodName,
    //     std::move(parameters),
    //     std::move(methodBlock),
    //     methodType,  
    //     currentScope
    // );

    // methodDef->getScope()->owner = generateScopeOwner("MethodDef", methodName);
    
    DEBUG_FLOW_EXIT();
    return methodDef;
}


UniquePtr<ASTStatement> Parser::parseClassCall() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    if (!expect(TokenType::ClassCall)) { throw UnexpectedTokenError(currentToken(), "ClassCall", "Parser::parseClassCall"); }

    String className = currentToken().value;
    Token controllingToken = advance();  // consume class name

    if (!expect(TokenType::Punctuation) || controllingToken.value != "(") { throw UnexpectedTokenError(controllingToken, "Expected '(' after class name in instantiation", "Parser::parseClassCall"); }


    // Vector<UniquePtr<ASTStatement>> arguments = parseArguments();
    auto arguments = parseAnyArgument();

    return makeUnique<ClassCall>(className, std::move(arguments), currentScope);
}




// Parse a class definition.
UniquePtr<ASTStatement> Parser::parseClassDefinition() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    insideClass = true;
    // Expect the 'Class' keyword.
    Token controllingToken = currentToken();
    if (controllingToken.type != TokenType::ClassDef) {
        throw SyntaxError("Expected 'Class' keyword.", controllingToken);
    }
    DEBUG_LOG(LogLevel::DEBUG, "got class keyword: ", controllingToken.toString());
    controllingToken = advance(); // Consume 'Class'
    
    // Next token should be the class name.
    if (controllingToken.type != TokenType::ClassRef) {
        throw SyntaxError("Expected class name after 'Class'.", controllingToken);
    }

    DEBUG_LOG(LogLevel::DEBUG, "got class name: ", controllingToken.toString());

    String className = currentToken().value;
    controllingToken = advance(); // Consume class name

    // Expect a colon after the class name.
    if (!expect(TokenType::Punctuation) || controllingToken.value != ":") {
        throw SyntaxError("Expected ':' after class name.", controllingToken);
    }
    controllingToken = advance(); // Consume ':'
    DEBUG_LOG(LogLevel::DEBUG, "got class end ':' : ", controllingToken.toString());

    // Expect a newline.
    if (controllingToken.type != TokenType::Newline) {
        throw SyntaxError("Expected newline after class header.", controllingToken);
    }

    processNewLines();
    DEBUG_LOG(LogLevel::DEBUG, "Processed NewLines After Class header");
    // Process indent to enter the class body.
    processIndent();

    processNewLines();
    DEBUG_LOG(LogLevel::DEBUG, "Processed NewLines After Indent");

    // Create a new ClassBody node in the current scope.
    auto classBody = makeUnique<ClassBody>(currentScope);
    if (!classBody->getScope()){
        throw MerkError("ClassBody Was Not provided a valid Scope around line 371 in Parser::parseClassDefinition");
    }

    // classBody->getScope()->owner = generateScopeOwner("ClassDef", className);

    bool foundConstructor = false;
    String accessor;

    ParamList classParams;
    while (currentToken().type != TokenType::Dedent && currentToken().type != TokenType::EOF_Token) {
        
        if (processNewLines()){ // if there were blank lines, then go again
            continue;
        }

        controllingToken = currentToken();

        if (controllingToken.type == TokenType::VarDeclaration) {
            auto attr = parseProtectedClassAttributes();
            classBody->addChild(std::move(attr));
        }

        else if (controllingToken.type == TokenType::ClassMethodDef) {
            if (!foundConstructor) {
                if (peek().value != "construct") {
                    throw SyntaxError("The first method in a class must be 'construct'.", currentToken());
                }
                foundConstructor = true;
                auto constructor = parseClassInitializer();
                accessor = constructor->getMethodAccessor();
                classParams = constructor->getParameters();
                
                if (constructor) {
                    classBody->addChild(static_unique_ptr_cast<BaseAST>(std::move(constructor)));
                }
            } 
            else if (peek().value == "construct"){
                if (controllingToken.value == "def"){
                    throw MerkError("Attempting to reassign the constructor using the def keyword");
                }
                auto constructor = parseClassInitializer();
                // constructor->getScope()->owner = generateScopeOwner("constructor", className);
                // constructor->getBody()->getScope()->owner = generateScopeOwner("constructor", className);
                classBody->addChild(static_unique_ptr_cast<BaseAST>(std::move(constructor)));

            }
            
            else {
                // Parse subsequent methods normally.
                auto method = parseClassMethod();
                if (method) {
                    // method->getBody()->getScope()->owner = generateScopeOwner("Method", method->getName());
                    // method->getScope()->owner = generateScopeOwner("Method", method->getName());
                    classBody->addChild(static_unique_ptr_cast<BaseAST>(std::move(method)));
                }
            }
        }
        else {
            // For any other statement, treat it normally.
            DEBUG_LOG(LogLevel::DEBUG, "Parsing Statement From Parser::parseClassDefinition");
            auto stmt = parseStatement();
            if (stmt) {
                classBody->addChild(std::move(stmt));
            } else {
                break;
            }
        }
    }

    // Process dedent to exit the class body.
    processDedent();

    if (!currentScope){
        throw MerkError("No viable scope present in Parser::parseClassDefinition");
    }
    UniquePtr<ASTStatement> classDefNode = makeUnique<ClassDef>(className, std::move(classParams), std::move(classBody), accessor, currentScope);
    // classDefNode->getScope()->owner = generateScopeOwner("ClassDef", className);
    DEBUG_FLOW_EXIT();
    insideClass = false;
    return classDefNode;
} 


UniquePtr<ASTStatement> Parser::parseClassLiteralCall() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    Token token = currentToken();
    DEBUG_LOG(LogLevel::NONE, "DEBUG Parser::parseClassLiteralCall: Entering with token: ", currentToken().toColoredString());
    UniquePtr<Arguments> elements = makeUnique<Arguments>(currentScope);
    if (consumeIf(TokenType::LeftBracket, "[")) {
        // Vector<UniquePtr<ASTStatement>> elements;
        // Vector<Argument> elements;
        // UniquePtr<ArgumentType> elements;

        if (!check(TokenType::RightBracket, "]")) {
            do {
                elements->addPositional(parsePrimaryExpression());
                // elements.push_back(parsePrimaryExpression());
            } while (consumeIf(TokenType::Punctuation, ","));
        }

        consume(TokenType::RightBracket, "]", "Parser::parseClassLiteralCall -> RightBracket");

        auto call = makeUnique<ClassCall>("List", std::move(elements), currentScope);
        processNewLines();
        DEBUG_FLOW_EXIT();
        return call;
    }

    if (consumeIf(TokenType::Operator, "<")) {
        if (!check(TokenType::Operator, ">")) {
            do {
                elements->addPositional(parsePrimaryExpression());
            } while (consumeIf(TokenType::Punctuation, ","));
        }

        consume(TokenType::Operator, ">", "Parser::parseClassLiteralCall -> Operator");

        auto call = makeUnique<ClassCall>("Array", std::move(elements), currentScope);
        
        
        processNewLines();
        DEBUG_FLOW_EXIT();
        return call;
    }


    if (consumeIf(TokenType::Operator, "{")) {
        bool isDict = false;

        if (!check(TokenType::Operator, "}")) {
            do {
                auto keyExpr = parsePrimaryExpression();

                if (check(TokenType::Punctuation, ":")) {
                    // Dict entry
                    isDict = true;
                    consume(TokenType::Punctuation, ":", "Parser::parseClassLiteralCall -> ':'");
                    // auto valExpr = ;
                    elements->addKeyword(std::move(keyExpr), parsePrimaryExpression());
                } else {
                    // Set entry
                    elements->addPositional(std::move(keyExpr));
                }
            } while (consumeIf(TokenType::Punctuation, ","));
        }

        consume(TokenType::Operator, "}", "Parser::parseClassLiteralCall -> Operator");

        auto call = makeUnique<ClassCall>(
            isDict ? "Dict" : "Set", 
            std::move(elements), 
            currentScope
        );

        processNewLines();
        // call->printAST(std::cout);
        DEBUG_FLOW_EXIT();
        // call.release();
        // throw MerkError("Testing");
        return call;
    }

    DEBUG_FLOW_EXIT();
    throw UnexpectedTokenError(currentToken(), "Expected list or array literal.", "parseClassLiteralCall");
}





UniquePtr<ASTStatement> Parser::parseThrowStatement() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    consume(TokenType::Keyword, "throw", "Parser::parseThrowStatement");

    UniquePtr<ASTStatement> expr = parseExpression();  // what to throw
    auto throwNode = makeUnique<Throw>(std::move(expr), currentScope);

    processNewLines();
    return throwNode;
}