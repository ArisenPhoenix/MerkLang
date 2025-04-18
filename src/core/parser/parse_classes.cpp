// In parse_classes.cpp (or wherever you put your class-parsing methods)
#include "ast/ast_class.h"       // For ClassDef and ClassBody
#include "core/errors.h"
#include "utilities/debugging_functions.h"
#include "core/tokenizer.h"      // For token types
#include "ast/ast_callable.h"
#include "core/parser.h"

IdentifierType inferIdentifierType(TokenType type) {
    switch (type) {
        case TokenType::Variable: return IdentifierType::Variable;
        case TokenType::FunctionRef: return IdentifierType::Function;
        case TokenType::ClassMethodRef: return IdentifierType::Method;
        case TokenType::ClassRef: return IdentifierType::Class;
        case TokenType::Parameter: return IdentifierType::Variable;
        // default: return IdentifierType::Variable;
        default: throw MerkError("infereIdentifier found no matching tokens from " + tokenTypeToString(type));
    }
}


UniquePtr<Chain> Parser::parseChain() {
    auto chain = makeUnique<Chain>(currentScope);
    Token parentToken = currentToken();
    DEBUG_LOG(LogLevel::ERROR, "parseChain initiated by: ", parentToken.toColoredString());


    if (!(parentToken.type == TokenType::VarDeclaration || parentToken.type == TokenType::ChainEntryPoint)) {  
        throw UnexpectedTokenError(parentToken, "VarDeclaration, ChainEntryPoint");
    }

    // The element to be declared, if in fact this is a declaration, is the last item to have a delimiter before it
    bool isDeclaration = parentToken.type == TokenType::VarDeclaration;
    Token declarationToken = currentToken();
    Token chainStartToken = peek();

    Token targetToken = chainStartToken;
    // Assuming if VarDeclaration, then the next will be ChainEntryPoint
    // if only ChainEntryPoint then assuming VarAssignment
    if (isDeclaration){
        if (declarationToken.type != TokenType::VarDeclaration) {
            throw UnexpectedTokenError(declarationToken, "VarDeclaration");
        }
        advance(); // consume var / const
        if (chainStartToken.type != TokenType::ChainEntryPoint) {
            throw UnexpectedTokenError(chainStartToken, "ChainEntryPoint");

        } 
        chainStartToken.type = TokenType::Variable;
        tokens[position] = chainStartToken;

        DEBUG_LOG(LogLevel::ERROR, "After mutation, token at position: ", tokens[position].toColoredString());


        if (currentToken().type != TokenType::Variable){
            throw MerkError("Failed to Mutate ChainEntryPoint to Variable in parseChain");
        }
    } else {
        // This is an assignment or access chain — mutate token
        if (parentToken.type == TokenType::ChainEntryPoint) {
            Token& current = tokens[position];
            if (current.type != TokenType::ChainEntryPoint) {
                throw MerkError("Expected ChainEntryPoint at start of parseChain.");
            }
            current.type = TokenType::Variable;
            DEBUG_LOG(LogLevel::ERROR, "Mutated ChainEntryPoint to Variable for assignment chain. Token now: ", current.toColoredString());
        }
    }
    // Handles Accessor/Variable
    Token token = currentToken(); // should be a variable / accessor - modified form ChainEntryPoint.
    if (token.type != TokenType::Variable){
        throw UnexpectedTokenError(token, "Variable");
    }

    // Start parsing chain elements:
    while (true) {
        Token token = currentToken();  // Should be self, then x, etc.
        
        // Peek ahead for the delimiter **before** parsing the expression
        String delimiter = peek().value;;
        // String delimiter = next
    
        // If there is no valid delimiter, we stop before parsing the last piece
        if (!(delimiter == "." || delimiter == "::")) {
            break;
        }
    
        auto ref = parsePrimaryExpression();  // Only parse if there's a valid chain continuation
    
        ChainElement elem;
        elem.name = token.value;
        elem.delimiter = delimiter;
        elem.object = std::move(ref);
        elem.type = token.type;
    
        chain->addElement(std::move(elem));
        advance();  // consume delimiter
    }
    
    if (isDeclaration || parentToken.type == TokenType::VarAssignment) {
        UniquePtr<ASTStatement> lastElement;

        targetToken = currentToken();
        if (targetToken.type != TokenType::Variable){
            throw UnexpectedTokenError(targetToken, "Variable");
        }

        // tokens[position+1] = targetToken;
        // tokens.insert(tokens.begin() + position + 1, targetToken);

        if (isDeclaration) {
            tokens.insert(tokens.begin() + position, declarationToken);
            DEBUG_LOG(LogLevel::TRACE, "Inserted declarationToken: ", declarationToken.toColoredString());

            lastElement = parseVariableDeclaration();
        } else {
            lastElement = parseVariableAssignment();
        }

        ChainElement finalElem;
        finalElem.name = targetToken.value;  // Can be refined later
        finalElem.delimiter = "";  // Final element should not carry a delimiter
        finalElem.object = std::move(lastElement);
        finalElem.type = targetToken.type;
        chain->replaceLastElementWith(std::move(finalElem));
    }
    DEBUG_LOG(LogLevel::ERROR, "Chain parsing complete. Returning Chain: ", chain->toString());

    return chain;
}

        // if (token.value == parentToken.value && token.type == parentToken.type && token.column == parentToken.column) {
        //     if (token.type == TokenType::ChainEntryPoint){
        //         token.type = TokenType::Variable;
        //         tokens[position] = token;
        //         if (currentToken().type != TokenType::Variable){
        //             throw UnexpectedTokenError(token, "ChainEntryPoint Not Set To Variable");
        //         }
        //     } else {
        //         throw UnexpectedTokenError(token, "ChainEntryPoint");
        //     }
            
        // } 



// Skeleton: Parse protected member declarations in the class.
UniquePtr<ASTStatement> Parser::parseClassAttributes() {
    DEBUG_FLOW(FlowLevel::LOW);
    // For now, simply call parseVariableDeclaration() and return the resulting node.
    // Later you might want to combine multiple declarations into a single ClassAttribute node.
    
    auto varDec = parsePrimaryExpression();
    // if (varDec->getAstType() != AstType::Chain){
    //     throw MerkError("Expected Chain")
    // }
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
    // // Consume the method keyword ("def" or "function")
    // advance();

    // Now the next token should be the method name.
    // Parse the method parameters and body using your normal parseFunctionDefinition logic
    // (or a specialized version if needed for classes).

    Token accessor = find(TokenType::Parameter, 6);

    // String accessor = String("self");

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
                classBody->addChild(static_unique_ptr_cast<BaseAST>(std::move(constructor)));

                // constructors.emplace_back(constructor);
            }
            
            else {
                // Parse subsequent methods normally.
                auto method = parseClassMethod();
                if (method) {
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

    UniquePtr<ASTStatement> classDefNode = makeUnique<ClassDef>(className, std::move(classParams), std::move(classBody), accessor, currentScope);

    DEBUG_FLOW_EXIT();
    insideClass = false;
    return classDefNode;
}