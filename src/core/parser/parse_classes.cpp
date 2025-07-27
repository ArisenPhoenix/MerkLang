// In parse_classes.cpp (or wherever you put your class-parsing methods)

#include "ast/ast_class.h"       // For ClassDef and ClassBody
#include "core/errors.h"
#include "utilities/helper_functions.h"
#include "utilities/debugging_functions.h"

#include "core/tokenizer.h"      // For token types
#include "core/scope.h"
#include "ast/ast_callable.h"
#include "ast/ast_method.h"
#include "core/parser.h"


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

UniquePtr<Chain> Parser::parseChain(bool isDeclaration, bool isConst) {
    DEBUG_FLOW(FlowLevel::NONE);
    DEBUG_LOG(LogLevel::DEBUG, "Chain Is Parsing as a Declaration: ", isDeclaration);
    
    auto chain = makeUnique<Chain>(currentScope);
    Token baseToken = currentToken();

    if (baseToken.type != TokenType::ChainEntryPoint && baseToken.type != TokenType::Variable) {
        throw UnexpectedTokenError(baseToken, "ChainEntryPoint or Variable", "Parser::parseChain");
    }

    Token punct = advance();  // consume base token (e.g., `self`, `this`, etc.)

    if (punct.type != TokenType::Punctuation || (punct.value != "." && punct.value != "::")) {
        throw UnexpectedTokenError(punct, "'.' or '::'", "Parser::parseChain");
    }

    chain->addElement(std::move(createChainElement(baseToken, punct, makeUnique<VariableReference>(baseToken.value, currentScope))));

    // Parse the rest of the dotted chain
    while (currentToken().type == TokenType::Punctuation && (currentToken().value == "." || currentToken().value == "::")) {

        Token delim = currentToken(); // store for later
        Token nextToken = advance(); // move past delim/Punctuation token

        if (nextToken.type != TokenType::Argument && nextToken.type != TokenType::Variable && nextToken.type != TokenType::FunctionCall && nextToken.type != TokenType::FunctionRef && nextToken.type != TokenType::ClassMethodCall && nextToken.type != TokenType::ClassMethodRef) {
            throw UnexpectedTokenError(nextToken, "(evaluable to node) after '.' or '::'", "Parser::parseChain");
        }

        bool isFunctionCall = nextToken.type == TokenType::FunctionCall;
        bool isMethodCall = nextToken.type == TokenType::ClassMethodCall;
        UniquePtr<BaseAST> currentObject;
        if (isFunctionCall) {
            currentObject = parseFunctionCall();
        }
        if (isMethodCall){
            auto func = parseFunctionCall();
            DEBUG_LOG(LogLevel::PERMISSIVE, "PARSED METHOD CALL");
            currentObject = makeUnique<MethodCall>(func->getName(), std::move(func->arguments), func->getScope());
        } else {
            currentObject = makeUnique<VariableReference>(nextToken.value, currentScope);
        }
        
        chain->addElement(std::move(createChainElement(nextToken, delim, std::move(currentObject))));

        DEBUG_LOG(LogLevel::PERMISSIVE, "Current Token After Parsing Chain Element: ", currentToken().toColoredString());

        if (currentToken().type != TokenType::Punctuation) { //in case this is an argument to another function
            advance();  // move to next delimiter or assignment
        }
        
    }

    Token maybeAssignmentOrType = currentToken(); // placeholder for special logic
    bool isAssignment = maybeAssignmentOrType.type == TokenType::VarAssignment;
    bool isMutable = maybeAssignmentOrType.value == "=";

    std::optional<NodeValueType> typeTag = std::nullopt;  //This will later go the way of the dodos
    ResolvedType type = ResolvedType("Any");

    if (isAssignment){
        maybeAssignmentOrType = advance(); // consume `=` or `:=`
    }

    if (isDeclaration || isAssignment){
        if (maybeAssignmentOrType.type == TokenType::Punctuation) { // means there is a type association
            if (consumeIf(TokenType::Punctuation, ":")) {
                type = parseResolvedType();
                isMutable = currentToken().value == "=";
                advance(); //consume mutability type
            }
            
        }
        auto& last = chain->getLast();
        UniquePtr<ASTStatement> rhs = parseExpression();  // gets the expression or value in which to assign variable to later
        if (isDeclaration) {
            bool isStatic = typeTag.has_value();
            if (last.type == TokenType::Variable) {
                auto varNode = VarNode(last.name, tokenTypeToString(last.type), isConst, isMutable, isStatic);
                last.object = makeUnique<VariableDeclaration>(last.name, varNode, currentScope, typeTag, std::move(rhs));
            }
            DEBUG_FLOW_EXIT();

            return chain;
        } 
        else if (isAssignment) {
            if (last.type == TokenType::Variable) {
                    last.object = makeUnique<VariableAssignment>(last.name, std::move(rhs), currentScope);
                }

                return chain;
                DEBUG_FLOW_EXIT();
            }
    }

    DEBUG_FLOW_EXIT();
    return chain;
}




UniquePtr<ChainOperation> Parser::parseChainOp() {
    DEBUG_FLOW(FlowLevel::NONE);
    
    bool isDeclaration = currentToken().type == TokenType::VarDeclaration;
    bool isConst = false;
    
    if (isDeclaration) {
        isConst = currentToken().value == "const";
        advance(); // consume declaration token.
    }

    UniquePtr<Chain> lhs = parseChain(isDeclaration, isConst);  // Handles `a.b.c` chains

    DEBUG_FLOW_EXIT();
    return makeUnique<ChainOperation>(
        std::move(lhs),
        nullptr,
        ChainOpKind::Reference,
        currentScope
    );
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
    // Token token = currentToken();
    // DEBUG_LOG(LogLevel::INFO, "Parsing function definition...", "Token: ", token.toString());

    // if (token.type != TokenType::ClassMethodDef) {
    //     throw SyntaxError("Expected TokenType 'def' or TokenType 'function'.", token);
    // }

    // if (!(token.value == "def" || token.value == "function")){
    //     throw SyntaxError("Expected 'def' or 'function' keyword.", token);
    // }

    // String methodDefType = token.value;
    // DEBUG_LOG(LogLevel::INFO, highlight("Function Type: ", Colors::red), methodDefType);
    // Token controllingToken = advance(); // Consume 'def' or 'function'


    // if (controllingToken.type != TokenType::ClassMethodRef) {
    //     throw SyntaxError("Expected function name.", controllingToken);
    // }

    // String methodName = controllingToken.value;
    // DEBUG_LOG(LogLevel::DEBUG, highlight("METHOD NAME: ", Colors::pink), highlight(methodName, Colors::blue));
    // controllingToken = advance(); // Consume method name

    // if (controllingToken.type != TokenType::Punctuation || controllingToken.value != "(") {
    //     throw SyntaxError("Expected '(' after method name.", controllingToken);
    // }

    // controllingToken = advance(); // Consume '('

    // ParamList parameters = handleParameters();
    
    // controllingToken = advance(); // Consume ')'

    // if (!expect(TokenType::Punctuation) || controllingToken.value != ":") {
    //     throw SyntaxError("Expected ':' after method definition.", controllingToken);
    // }
    
    // controllingToken = advance(); // Consume ':'

    // UniquePtr<CodeBlock> bodyBlock = parseBlock();
    

    // if (!bodyBlock) {
    //     throw SyntaxError("Methodbody block could not be parsed.", controllingToken);
    // }

    // UniquePtr<MethodBody> methodBlock = makeUnique<MethodBody>(std::move(bodyBlock));
    // methodBlock->getScope()->owner = generateScopeOwner("MethodDef", methodName);
    // for (auto& child : methodBlock->getChildren()) {
    //     auto astType = child->getAstTypeAsString();
    //     child->getScope()->owner = generateScopeOwner("MethodBody<" + astType + ">", methodName);
    // }

    // DEBUG_LOG(LogLevel::INFO, "MethodBody type: ", typeid(*methodBlock).name());
    
    // CallableType methodType;

    // if (methodDefType == "def"){methodType = CallableType::DEF;}

    // else if (methodDefType == "function"){methodType = CallableType::FUNCTION;} 
    
    // else {
    //     DEBUG_LOG(LogLevel::INFO, "Method definition parsed unsuccessfully: ", methodName, ":", methodDefType);
    //     throw MerkError("Function Type: " + methodDefType + " is not Valid");
    // }

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

    methodDef->getScope()->owner = generateScopeOwner("MethodDef", methodName);
    
    DEBUG_FLOW_EXIT();
    return methodDef;
}


UniquePtr<ASTStatement> Parser::parseClassCall() {
    DEBUG_FLOW(FlowLevel::HIGH);

    if (!expect(TokenType::ClassCall)) {
        throw UnexpectedTokenError(currentToken(), "ClassCall", "Parser::parseClassCall");
    }

    String className = currentToken().value;
    Token controllingToken = advance();  // consume class name

    if (!expect(TokenType::Punctuation) || controllingToken.value != "(") {
        throw UnexpectedTokenError(controllingToken, "Expected '(' after class name in instantiation", "Parser::parseClassCall");
    }


    // Vector<UniquePtr<ASTStatement>> arguments = parseArguments();
    auto arguments = parseAnyArgument();

    return makeUnique<ClassCall>(className, std::move(arguments), currentScope);
}




// Parse a class definition.
UniquePtr<ASTStatement> Parser::parseClassDefinition() {
    DEBUG_FLOW(FlowLevel::HIGH);
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

    classBody->getScope()->owner = generateScopeOwner("ClassDef", className);

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
    classDefNode->getScope()->owner = generateScopeOwner("ClassDef", className);
    DEBUG_FLOW_EXIT();
    insideClass = false;
    return classDefNode;
}


UniquePtr<ASTStatement> Parser::parseClassLiteralCall() {
    DEBUG_FLOW(FlowLevel::NONE);
    Token token = currentToken();
    DEBUG_LOG(LogLevel::NONE, "DEBUG Parser::parseClassLiteralCall: Entering with token: ", currentToken().toColoredString());

    if (consumeIf(TokenType::LeftBracket, "[")) {
        // Vector<UniquePtr<ASTStatement>> elements;
        // Vector<Argument> elements;
        UniquePtr<ArgumentType> elements;

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
        UniquePtr<ArgumentType> elements;
        // Vector<UniquePtr<ASTStatement>> elements;

        if (!check(TokenType::Operator, ">")) {
            do {
                // elements.push_back(parsePrimaryExpression());
                elements->addPositional(parsePrimaryExpression());
            } while (consumeIf(TokenType::Punctuation, ","));
        }

        consume(TokenType::Operator, ">", "Parser::parseClassLiteralCall -> Operator");

        auto call = makeUnique<ClassCall>("Array", std::move(elements), currentScope);
        
        // throw MerkError("Literal Class call above");
        processNewLines();
        DEBUG_FLOW_EXIT();
        return call;
    }

    // String type;

    if (consumeIf(TokenType::Operator, "{")) {
        // Vector<UniquePtr<ASTStatement>> elements;
        UniquePtr<ArgumentType> elements;
        if (!check(TokenType::Operator, "}")) {
            do {
                auto expr = parsePrimaryExpression();
                expr->printAST(std::cout);
                elements->addPositional(std::move(expr));
                // elements.push_back(std::move(expr));
            } while (consumeIf(TokenType::Punctuation, ":"));
        }

        consume(TokenType::Operator, "}", "Parser::parseClassLiteralCall -> Operator");

        auto call = makeUnique<ClassCall>("Dict", std::move(elements), currentScope);
        processNewLines();
        call->printAST(std::cout);
        DEBUG_FLOW_EXIT();
        return call;
    }


    DEBUG_FLOW_EXIT();
    throw UnexpectedTokenError(currentToken(), "Expected list or array literal.", "parseClassLiteralCall");

}

