// In parse_classes.cpp (or wherever you put your class-parsing methods)

#include "ast/ast_class.h"       // For ClassDef and ClassBody
#include "core/errors.h"
#include "utilities/helper_functions.h"
#include "utilities/debugging_functions.h"

#include "core/tokenizer.h"      // For token types
#include "core/scope.h"
#include "ast/ast_callable.h"
#include "core/parser.h"





// IdentifierType inferIdentifierType(TokenType type) {
//     switch (type) {
//         case TokenType::Variable: return IdentifierType::Variable;
//         case TokenType::FunctionRef: return IdentifierType::Function;
//         case TokenType::ClassMethodRef: return IdentifierType::Method;
//         case TokenType::ClassRef: return IdentifierType::Class;
//         case TokenType::Parameter: return IdentifierType::Variable;
//         // default: return IdentifierType::Variable;
//         default: throw MerkError("infereIdentifier found no matching tokens from " + tokenTypeToString(type));
//     }
// }


std::tuple<bool, bool, String> Parser::handleIsVarDec() {
    Token startToken = currentToken();
    // DEBUG_LOG(LogLevel::PERMISSIVE, "StartToken: ", startToken.toColoredString());
    bool isDeclaration = startToken.type == TokenType::VarDeclaration;
    bool isConst = false;
    if (!(startToken.type == TokenType::VarDeclaration || startToken.type == TokenType::ChainEntryPoint)) {
        return std::make_tuple(isDeclaration, isConst, "");
    }

    
    isConst = startToken.value == "const";
    if (isDeclaration) {
        advance(); //consume (var/const)
        auto nextToken = currentToken();
        if (nextToken.type == TokenType::Variable || nextToken.type == TokenType::ChainEntryPoint){
            // advance(); //consume (const/var)
            return std::make_tuple(isDeclaration, isConst, String(startToken.value));
        }
    }

    return std::make_tuple(isDeclaration, isConst, "");
    
}

UniquePtr<Chain> Parser::parseChain(bool isDeclaration) {
    (void)isDeclaration;
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    
    auto chain = makeUnique<Chain>(currentScope);
    Token baseToken = currentToken();

    if (baseToken.type != TokenType::ChainEntryPoint && baseToken.type != TokenType::Variable) {
        throw UnexpectedTokenError(baseToken, "Expected ChainEntryPoint or Variable");
    }

    advance();  // consume base token (e.g., `self`, `this`, etc.)

    Token punct = currentToken();
    if (punct.type != TokenType::Punctuation || (punct.value != "." && punct.value != "::")) {
        throw UnexpectedTokenError(punct, "'.' or '::'");
    }

    if (baseToken.value == "Scope") {
        DEBUG_LOG(LogLevel::PERMISSIVE, baseToken.toColoredString());
        throw MerkError("BaseToken.value is Scope");
        }

    // Add base element
    ChainElement baseElem;
    baseElem.name = baseToken.value;
    baseElem.delimiter = punct.value;
    baseElem.type = baseToken.type;
    baseElem.object = makeUnique<VariableReference>(baseToken.value, currentScope);
    chain->addElement(std::move(baseElem));

    // Parse the rest of the dotted chain
    while (currentToken().type == TokenType::Punctuation && (currentToken().value == "." || currentToken().value == "::")) {

        Token delim = currentToken(); // store for later

        advance(); // move past punctuation token

        Token nextToken = currentToken();
        if (nextToken.type != TokenType::Variable && nextToken.type != TokenType::FunctionCall && nextToken.type != TokenType::FunctionRef) {
            throw UnexpectedTokenError(nextToken, "variable after '.' or '::'");
        }

        bool isFunctionCall = nextToken.type == TokenType::FunctionCall;
        UniquePtr<BaseAST> currentObject;
        if (!isFunctionCall){
            currentObject = makeUnique<VariableReference>(nextToken.value, currentScope);
        } else {
            currentObject = parseFunctionCall();
        }

        ChainElement elem;
        elem.name = nextToken.value;
        elem.delimiter = delim.value;
        elem.type = nextToken.type;
        elem.object = std::move(currentObject);
        
        chain->addElement(std::move(elem));
        advance();  // move to next delimiter or assignment
    }

    

    DEBUG_FLOW_EXIT();
    return chain;
}

// UniquePtr<ChainOperation> Parser::parseChainOp() {
//     DEBUG_FLOW(FlowLevel::VERY_HIGH);
    
//     bool isDeclaration = currentToken().type == TokenType::VarDeclaration;
//     // DEBUG_LOG(LogLevel::PERMISSIVE, "IS DECLARATION: ", currentToken().toColoredString());
//     bool isConst = false;
//     if (isDeclaration) {
//         isConst = currentToken().value == "const";
//         advance(); // consume declaration token. 
//     }

//     UniquePtr<Chain> lhs = parseChain(isDeclaration);  // Handles `a.b.c` chains
//     Token assign = currentToken();
//     bool isAssignment = assign.type == TokenType::VarAssignment;
//     bool isStatic = false;                  // placeholder for when static variables are included.
//     bool isMutable = assign.value == "=";
//     if (isAssignment){
//         advance(); // consume `=` or `:=`
//     }


//     if (isDeclaration || isAssignment) {
//         UniquePtr<ASTStatement> rhs = parseExpression();

//         auto& last = lhs->getLast();
//         // DEBUG_LOG(LogLevel::PERMISSIVE, "IS DECLARATION: ", isDeclaration);
//         if (isDeclaration) {
//             if (last.type == TokenType::Variable) {
//                 auto varNode = VarNode(last.name, tokenTypeToString(last.type), isConst, isMutable, isStatic);
//                 last.object = makeUnique<VariableDeclaration>(last.name, varNode, currentScope, std::nullopt, std::move(rhs));
//             }
//             DEBUG_FLOW_EXIT();

//             return makeUnique<ChainOperation>(
//                 std::move(lhs),
//                 nullptr,
//                 isDeclaration ? ChainOpKind::Declaration : ChainOpKind::Assignment,
//                 currentScope,
//                 isDeclaration ? isConst : false,
//                 isDeclaration ? isMutable : true,
//                 isStatic
//             );
//         }
        
//         DEBUG_FLOW_EXIT();
//         return makeUnique<ChainOperation>(
//             std::move(lhs),
//             std::move(rhs),
//             isDeclaration ? ChainOpKind::Declaration : ChainOpKind::Assignment,
//             currentScope,
//             isDeclaration ? isConst : false,
//             isDeclaration ? isMutable : true,
//             isStatic
//         );
//     } else {
//         // DEBUG_LOG(LogLevel::PERMISSIVE, "NOT IS DECLARATION: ", isDeclaration);

//         // Just a value reference
//         DEBUG_FLOW_EXIT();
//         return makeUnique<ChainOperation>(
//             std::move(lhs),
//             nullptr,
//             ChainOpKind::Reference,
//             currentScope
//         );
//     }
// }


UniquePtr<ChainOperation> Parser::parseChainOp() {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    
    bool isDeclaration = currentToken().type == TokenType::VarDeclaration;
    // DEBUG_LOG(LogLevel::PERMISSIVE, "IS DECLARATION: ", currentToken().toColoredString());
    bool isConst = false;
    std::optional<NodeValueType> typeTag = std::nullopt;

    if (isDeclaration) {
        isConst = currentToken().value == "const";
        advance(); // consume declaration token.
        // if (currentToken().type == TokenType::Variable && peek().value == ":" && peek(2).value != ":") {
        //     typeTag = getTypeFromString(peek().value);
        // }
    }

    UniquePtr<Chain> lhs = parseChain(isDeclaration);  // Handles `a.b.c` chains
    Token assign = currentToken();
    bool isAssignment = assign.type == TokenType::VarAssignment;
    bool isStatic = typeTag.has_value();                  // placeholder for when static variables are included.
    bool isMutable = assign.value == "=";
    if (isAssignment){
        advance(); // consume `=` or `:=`
    }


    if (isDeclaration || isAssignment) {
        UniquePtr<ASTStatement> rhs = parseExpression();

        auto& last = lhs->getLast();
        // DEBUG_LOG(LogLevel::PERMISSIVE, "IS DECLARATION: ", isDeclaration);
        if (isDeclaration) {
            if (last.type == TokenType::Variable) {
                auto varNode = VarNode(last.name, tokenTypeToString(last.type), isConst, isMutable, isStatic);
                // auto varNode = VarNode(last.name, tokenTypeToString(last.type), isConst, isMutable, isStatic, typeTag);
                last.object = makeUnique<VariableDeclaration>(last.name, varNode, currentScope, typeTag, std::move(rhs));
            }
            DEBUG_FLOW_EXIT();

            return makeUnique<ChainOperation>(
                std::move(lhs),
                nullptr,
                isDeclaration ? ChainOpKind::Declaration : ChainOpKind::Assignment,
                currentScope,
                isDeclaration ? isConst : false,
                isDeclaration ? isMutable : true,
                isStatic
            );
        } else if (isAssignment) {
            if (last.type == TokenType::Variable) {
                    // auto varNode = VarNode(last.name, tokenTypeToString(last.type), isConst, isMutable, isStatic);
                    last.object = makeUnique<VariableAssignment>(last.name, std::move(rhs), currentScope);
                }
                DEBUG_FLOW_EXIT();

                return makeUnique<ChainOperation>(
                    std::move(lhs),
                    nullptr,
                    isDeclaration ? ChainOpKind::Declaration : ChainOpKind::Assignment,
                    currentScope,
                    isDeclaration ? isConst : false,
                    isDeclaration ? isMutable : true,
                    isStatic
                );
        }
        
        DEBUG_FLOW_EXIT();
        return makeUnique<ChainOperation>(
            std::move(lhs),
            std::move(rhs),
            isDeclaration ? ChainOpKind::Declaration : ChainOpKind::Assignment,
            currentScope,
            isDeclaration ? isConst : false,
            isDeclaration ? isMutable : true,
            isStatic
        );
    } else {
        // DEBUG_LOG(LogLevel::PERMISSIVE, "NOT IS DECLARATION: ", isDeclaration);

        // Just a value reference
        DEBUG_FLOW_EXIT();
        return makeUnique<ChainOperation>(
            std::move(lhs),
            nullptr,
            ChainOpKind::Reference,
            currentScope
        );
    }
}


// Skeleton: Parse protected member declarations in the class.
UniquePtr<ASTStatement> Parser::parseClassAttributes() {
    DEBUG_FLOW(FlowLevel::LOW);
    
    auto varDec = parsePrimaryExpression();
    DEBUG_FLOW_EXIT();
    return varDec;
}

UniquePtr<ASTStatement> Parser::parseProtectedClassAttributes() {
    DEBUG_FLOW(FlowLevel::LOW);
    auto varDec = parseVariableDeclaration();

    DEBUG_FLOW_EXIT();
    return varDec;
}

// Skeleton: Parse the constructor (named "construct").
UniquePtr<MethodDef> Parser::parseClassInitializer() {
    DEBUG_FLOW(FlowLevel::HIGH);
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
    DEBUG_FLOW(FlowLevel::HIGH);
    Token token = currentToken();
    DEBUG_LOG(LogLevel::INFO, "Parsing function definition...", "Token: ", token.toString());

    if (token.type != TokenType::ClassMethodDef) {
        throw SyntaxError("Expected TokenType 'def' or TokenType 'function'.", token);
    }

    if (!(token.value == "def" || token.value == "function")){
        throw SyntaxError("Expected 'def' or 'function' keyword.", token);
    }

    String methodDefType = token.value;
    DEBUG_LOG(LogLevel::INFO, highlight("Function Type: ", Colors::red), methodDefType);
    advance(); // Consume 'def' or 'function'


    if (currentToken().type != TokenType::ClassMethodRef) {
        throw SyntaxError("Expected function name.", currentToken());
    }

    String methodName = currentToken().value;
    DEBUG_LOG(LogLevel::DEBUG, highlight("METHOD NAME: ", Colors::pink), highlight(methodName, Colors::blue));
    advance(); // Consume method name

    if (currentToken().type != TokenType::Punctuation || currentToken().value != "(") {
        throw SyntaxError("Expected '(' after method name.", currentToken());
    }

    advance(); // Consume '('

    ParamList parameters = handleParameters();
    
    advance(); // Consume ')'

    if (!expect(TokenType::Punctuation) || currentToken().value != ":") {
        throw SyntaxError("Expected ':' after method definition.", currentToken());
    }
    
    advance(); // Consume ':'

    UniquePtr<CodeBlock> bodyBlock = parseBlock();

    if (!bodyBlock) {
        throw SyntaxError("Methodbody block could not be parsed.", currentToken());
    }

    UniquePtr<MethodBody> methodBlock = makeUnique<MethodBody>(std::move(bodyBlock));
    // methodBlock->getScope()->owner = generateScopeOwner("Method", methodName);

    DEBUG_LOG(LogLevel::INFO, "MethodBody type: ", typeid(*methodBlock).name());
    
    CallableType methodType;

    if (methodDefType == "def"){
        methodType = CallableType::DEF;
    }

    else if (methodDefType == "function"){
        methodType = CallableType::FUNCTION;
    } 
    
    else {
        DEBUG_LOG(LogLevel::INFO, "Method definition parsed unsuccessfully: ", methodName, ":", methodDefType);
        throw MerkError("Function Type: " + methodDefType + " is not Valid");
    }

    
    DEBUG_LOG(LogLevel::DEBUG, "CurrentToken Before Leaving parseMethodDefinitions: ", currentToken().toString());
    UniquePtr<MethodDef> methodDef = makeUnique<MethodDef>(
        methodName,
        std::move(parameters),
        std::move(methodBlock),
        methodType,  
        currentScope
    );
    
    DEBUG_LOG(LogLevel::INFO, "Function definition parsed successfully: ", methodName);
    DEBUG_FLOW_EXIT();
    return methodDef;
}


UniquePtr<ASTStatement> Parser::parseClassCall() {
    DEBUG_FLOW(FlowLevel::HIGH);

    if (!expect(TokenType::ClassCall)) {
        throw UnexpectedTokenError(currentToken(), "ClassCall");
    }

    String className = currentToken().value;
    advance();  // consume class name

    if (!expect(TokenType::Punctuation) || currentToken().value != "(") {
        throw UnexpectedTokenError(currentToken(), "Expected '(' after class name in instantiation");
    }

    advance();  // consume '('

    Vector<UniquePtr<ASTStatement>> arguments;
    while (!expect(TokenType::Punctuation) || currentToken().value != ")") {
        auto arg = parseExpression();
        if (!arg) {
            throw SyntaxError("Invalid argument to class constructor", currentToken());
        }
        arguments.push_back(std::move(arg));

        if (expect(TokenType::Punctuation) && currentToken().value == ",") {
            advance();
        } else {
            break;
        }
    }

    if (!expect(TokenType::Punctuation) || currentToken().value != ")") {
        throw UnexpectedTokenError(currentToken(), "Expected ')' to close class instantiation");
    }

    advance();  // consume ')'

    return makeUnique<ClassCall>(className, std::move(arguments), currentScope);
}




// Parse a class definition.
UniquePtr<ASTStatement> Parser::parseClassDefinition() {
    DEBUG_FLOW(FlowLevel::HIGH);
    insideClass = true;
    // Expect a 'Class' keyword.
    Token token = currentToken();
    if (token.type != TokenType::ClassDef) {
        throw SyntaxError("Expected 'Class' keyword.", token);
    }
    DEBUG_LOG(LogLevel::DEBUG, "got class keyword: ", token.toString());
    advance(); // Consume 'Class'
    
    // Next token should be the class name.
    if (currentToken().type != TokenType::ClassRef) {
        throw SyntaxError("Expected class name after 'Class'.", currentToken());
    }

    DEBUG_LOG(LogLevel::DEBUG, "got class name: ", currentToken().toString());

    String className = currentToken().value;
    advance(); // Consume class name

    // Expect a colon after the class name.
    if (!expect(TokenType::Punctuation) || currentToken().value != ":") {
        throw SyntaxError("Expected ':' after class name.", currentToken());
    }
    advance(); // Consume ':'
    DEBUG_LOG(LogLevel::DEBUG, "got class end ':' : ", currentToken().toString());

    // Expect a newline.
    if (currentToken().type != TokenType::Newline) {
        throw SyntaxError("Expected newline after class header.", currentToken());
    }
    // advance(); // Consume newline
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
    // In the class body, we might have protected attribute declarations first,
    // then method definitions. We also need to ensure the first method is 'construct'.
    bool foundConstructor = false;
    String accessor;

    ParamList classParams;
    // Vector<UniquePtr<MethodDef>> constructors;
    while (currentToken().type != TokenType::Dedent && currentToken().type != TokenType::EOF_Token) {

        if (processNewLines()){ // if there were blank lines, then go again
            continue;
        }

        if (currentToken().type == TokenType::VarDeclaration) {
            auto attr = parseProtectedClassAttributes();
            classBody->addChild(std::move(attr));
        }

        else if (currentToken().type == TokenType::ClassMethodDef) {
        //  ||      currentToken().type == TokenType::FunctionDef) {
            // The first method must be the constructor.
            if (!foundConstructor) {
                if (peek().value != "construct") {
                    throw SyntaxError("The first method in a class must be 'construct'.", currentToken());
                }
                foundConstructor = true;
                auto constructor = parseClassInitializer();
                accessor = constructor->getMethodAccessor();
                classParams = constructor->getParameters();
                
                if (constructor) {
                    // classBody->setInitializer(std::move(constructor));
                    // classBody->me
                    classBody->addChild(static_unique_ptr_cast<BaseAST>(std::move(constructor)));
                    // constructors.emplace_back(constructor);
                }
            } 
            else if (peek().value == "construct"){
                if (currentToken().value == "def"){
                    throw MerkError("Attempting to reassign the constructor using the def keyword");
                }
                auto constructor = parseClassInitializer();
                constructor->getScope()->owner = generateScopeOwner("constructor", className);
                constructor->getBody()->getScope()->owner = generateScopeOwner("constructor", className);
                classBody->addChild(static_unique_ptr_cast<BaseAST>(std::move(constructor)));

            }
            
            else {
                // Parse subsequent methods normally.
                auto method = parseClassMethod();
                if (method) {
                    method->getBody()->getScope()->owner = generateScopeOwner("Method", method->getName());
                    method->getScope()->owner = generateScopeOwner("Method", method->getName());
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

    DEBUG_FLOW_EXIT();
    insideClass = false;
    return classDefNode;
}


