#include "core/types.h"
#include "core/tokenizer.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"
#include "core/errors.h"


bool Tokenizer::isLogicOperator() {
    size_t lookahead = position;
    while (lookahead < sourceLength &&
           (std::isalpha(static_cast<unsigned char>(source[lookahead])) || source[lookahead] == '_')) {
        lookahead++;
    }

    String value = source.substr(position, lookahead - position);
    return (value == "and" || value == "or" || value == "not");
}



Vector<Token> Tokenizer::tokenize() {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    sourceLength = source.size();
    while (position < sourceLength) {
        DEBUG_LOG(LogLevel::DEBUG, "Processing char: ", source[position], " at position: ", position);
        
        // Skip whitespace
        while (position < sourceLength && isWhitespace(source[position])) {
            position++;
            column++;
        }

        if (source[position] == '\n') {
            // Secondary Backslash for viewing properly in the output
            tokens.emplace_back(TokenType::Newline, "NewLine", line, column);
            position++;
            line++;
            column = 1;
            handleIndentation(tokens);
            continue;
        }

        // Handle comments - perhaps will use them for something else later
        if (position < sourceLength && source[position] == '#') {
            while (position < sourceLength && source[position] != '\n') {
                position++;
            }
            continue;
        }

        else if (source[position] == '(') {
            tokens.push_back(Token(TokenType::Punctuation, "(", line, column));
            position++;
            column++;
            continue;
        } 

        else if (source[position] == ',') {
            tokens.push_back(Token(TokenType::Punctuation, ",", line, column));
            position++;
            column++;
            continue;
        }
        else if (source[position] == ')') {
            tokens.push_back(Token(TokenType::Punctuation, ")", line, column));
            position++;
            column++;
            insideArgs = false;
            insideParams = false;
            continue;
        }

        // Handle compound operators first
        if ((isOperator(source[position]) || isPunctuation(source[position])) &&
            (isOperator(peek()) || isPunctuation(peek()))) {
            tokens.push_back(readCompoundOperatorOrPunctuation());
            position += 2;
            column += 2;
            continue;
        }
        // Handle single-character operators
        else if (isOperator(source[position]) || isPunctuation(source[position])) {
            tokens.push_back(readOperatorOrPunctuation());
            position++;  // Move past the single character
            column++;
            continue;
            
        }

        // Handle identifiers, numbers, and strings
        else if (isLetter(source[position])) {
            Token identifier = readIdentifier();  // advances past identifier
        
            char nextChar = position < sourceLength ? source[position] : '\0';
        
            char nextNextChar = peek();
        
            if (identifier.type == TokenType::Variable && 
                (nextChar == '.' || (nextChar == ':' && nextNextChar == ':'))) {
                identifier.type = TokenType::ChainEntryPoint;
            }
        
            tokens.push_back(identifier);
            skipWhitespace();
            
            if (previousToken().type == TokenType::Variable && handleOptionalType(tokens)) {
                continue;  // already tokenized the type annotation
            }
        } else if (isDigit(source[position])) {
            tokens.push_back(readNumber());
        } else if (source[position] == '"') {
            tokens.push_back(readString());
        } else {
            DEBUG_LOG(LogLevel::DEBUG, "Unknown token at line ", line, ", column ", column, " (", source[position], ")");
            tokens.emplace_back(TokenType::Unknown, String(1, source[position]), line, column);
            position++;
            column++;
        }
    }

    finalizeIndentation(tokens);
    tokens.emplace_back(TokenType::EOF_Token, "EOF", line, column);
    DEBUG_FLOW_EXIT();
    return tokens;
}

Token Tokenizer::readCompoundOperatorOrPunctuation() {
    int startColumn = column;
    String twoCharOp = String(1, source[position]) + peek(); // Look ahead
    // TokenType::VarAssignment
    if (twoCharOp == ":="){
        return Token(TokenType::VarAssignment, twoCharOp, line, startColumn);
    }
    if (twoCharOp == "==" || twoCharOp == "!=" ||
        twoCharOp == "<=" || twoCharOp == ">=" || twoCharOp == "&&" ||
        twoCharOp == "||" || twoCharOp == "+=" || twoCharOp == "-=" ||
        twoCharOp == "*=" || twoCharOp == "/=" || twoCharOp == "%=") {

        return Token(TokenType::Operator, twoCharOp, line, startColumn);
    }

    throw UnknownTokenError("Unexpected compound operator: " + twoCharOp, line, column, currentLineText);
}

Token Tokenizer::readOperatorOrPunctuation() {
    char c = source[position];

    // Handle assignment "=" (but not "==", which is handled in readCompoundOperatorOrPunctuation)
    if (c == '=') {
        return Token(TokenType::VarAssignment, "=", line, column);
    }

    // Handle other mathematical and logical operators
    if (isOperator(c)) return Token(TokenType::Operator, String(1, c), line, column);
    if (isPunctuation(c)) return Token(TokenType::Punctuation, String(1, c), line, column);

    throw UnknownTokenError(String(1, c), line, column, currentLineText);
}

Token Tokenizer::readNumber() {
    size_t start = position;
    int startColumn = column;

    // Read leading digits
    while (position < sourceLength && isDigit(source[position])) {
        position++;
        column++;
    }

    // Check for decimal part
    if (position < sourceLength && source[position] == '.') {
        if (isDigit(peek())) {
            position++;
            column++;

            while (position < sourceLength && isDigit(source[position])) {
                position++;
                column++;
            }
        }
    }

    String number = source.substr(start, position - start);
    return Token(TokenType::Number, number, line, startColumn);
}


Token Tokenizer::readString() {
    int startColumn = column;

    // Move past the opening double-quote
    position++;
    column++;

    String result;
    while (position < sourceLength && source[position] != '"') {
        if (source[position] == '\\') { // Handle escape sequences
            if (position + 1 >= sourceLength) {
                throw TokenizationError("Unfinished escape sequence in string literal.", line, column);
            }
            char nextChar = source[++position];
            column++;
            switch (nextChar) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case '\\': result += '\\'; break;
                case '"': result += '"'; break;
                default:
                    // throw RunTimeError("Unknown escape sequence: \\" + String(1, nextChar));
                    throw UnknownTokenError(std::to_string(nextChar), line, column, currentLineText);
            }
        } else {
            result += source[position];
        }
        position++;
        column++;
    }

    if (position >= sourceLength || source[position] != '"') {
        throw UnmatchedQuoteError(line, column, currentLineText);
    }

    position++;
    column++;

    return Token(TokenType::String, result, line, startColumn);
}

Token Tokenizer::readIdentifier() {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    DEBUG_LOG(LogLevel::DEBUG, "Entering readIdentifier at position: ", position);

    if (position >= sourceLength) {
        throw TokenizationError("Unexpected end of source in readIdentifier.", line, column, currentLineText);
    }

    size_t start = position;
    int startColumn = column;

    // Read identifier (letters, digits, underscores)
    while (position < sourceLength && (isLetter(source[position]) || isDigit(source[position]) || source[position] == '_')) {
        position++;
        column++;
    }

    String value = source.substr(start, position - start);
    TokenType type = TokenType::Variable;  // Default to Variable

    DEBUG_LOG(LogLevel::DEBUG, "Found identifier: ", value);
    const Token& prev = lastToken();  // Do once

    // Class definition keyword: "Class"
    if (value == "Class") {
        type = TokenType::ClassDef;
        insideClass = true;
        classIndentLevel = currentIndent;
    }

    else if (lastTokenWas(TokenType::ClassDef)) {
        type = TokenType::ClassRef;
        classes.insert(value);
    }

    else if (value == "and" || value == "or" || value == "not"){
        type = TokenType::Operator;
    }
    

    // Function Definition Keyword?
    else if (value == "function" || value == "def") {
        type = insideClass ? TokenType::ClassMethodDef : TokenType::FunctionDef; 
        insideParams = true; // Keep this active until after ')'
    }


    else if (!tokens.empty() && prev.type == TokenType::ClassDef) {
        type = TokenType::ClassRef;
        classes.insert(value);
    }
    
    // Function Name (Immediately after `def`)
    else if (tokens.size() > 0 && (prev.type == TokenType::FunctionDef || prev.type == TokenType::ClassMethodDef) && (previousToken().value != "=" && previousToken().value != ":=")){
        type = insideClass ? TokenType::ClassMethodRef : TokenType::FunctionRef;
        functions.insert(value);
    }

    
    else if (isClass(value)) {
        type = TokenType::ClassCall;
        insideArgs = true;
    }

    // Function Name (Immediately after `FunctionDef`
    else if (isFunction(position)) {
        type = insideClass ? TokenType::ClassMethodCall : TokenType::FunctionCall;
        insideArgs = true;
    }

    else if (isFunction(value) && previousToken().type != TokenType::VarDeclaration){
        type = insideClass ? TokenType::ClassMethodRef : TokenType::FunctionRef;
    }

    // Function Argument (Immediately after `call` and inside function call lists)
    else if (insideArgs && prev.type == TokenType::Punctuation){
        type = TokenType::Argument;
    }

    // Function Parameters (Only inside function parameter lists)
    else if (insideParams) {
        type = TokenType::Parameter;
    }

    // Variable Declaration?
    else if (value == "var" || value == "const") {
        type = TokenType::VarDeclaration;
    }

    // Control Flow Keywords
    else if (value == "if" || value == "elif" || value == "else") {
        type = TokenType::Keyword;
    }

    // Recognize import-related keywords:
    else if (value == "import" || value == "from") {
        type = TokenType::Keyword;
    }

    // Loop Keywords
    else if (value == "for" || value == "while" || value == "break" || value == "return" || value == "continue") {
        type = TokenType::Keyword;
    }

    // Boolean Literals
    else if (value == "true" || value == "false") {
        type = TokenType::Bool;
    }

    DEBUG_LOG(LogLevel::DEBUG, "Exiting readIdentifier with value: ", value, " (Type: ", tokenTypeToString(type), ")");
    DEBUG_FLOW_EXIT();
    return Token(type, value, line, startColumn);
}

void Tokenizer::skipWhitespace() {
    while (position < sourceLength && isWhitespace(source[position])) {
        position++;
        column++;
    }

    if (position >= sourceLength) {
        OutOfBoundsError(line, column, currentLineText);
    }
}

int Tokenizer::countLeadingSpaces(const String& line) {
    int count = 0;
    for (char ch : line) {
        if (ch == ' ') {
            count++;
        } else if (ch == '\t') {
            count += 4;  // Assuming 1 tab = 4 spaces
        } else {
            break;  // Stop at the first non-whitespace character
        }
    }
    return count;
}

bool Tokenizer::isCompoundOperator(char c) const {
    // Look ahead to check for `=`
    return (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
            c == '&' || c == '|' || c == '^' || c == '<' || c == '>') && peek() == '=';
}

bool Tokenizer::isComparisonOperator(char c) const {
    // Look ahead to check for second character
    return (c == '=' || c == '!' || c == '<' || c == '>') && peek() == '=';
}

// Checks if the character is a mathematical operator (+, -, *, /, %)
bool Tokenizer::isMathOperator(char c) const {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%';
}

// Checks if the character is a comparison or logical operator (=, ==, !=, <=, >=, &&, ||, !)
bool Tokenizer::isComparisonOrLogicalOperator(char c) const {
    return c == '=' || c == '<' || c == '>' || c == '!' || c == '&' || c == '|';
}

// Peeks at the next character without advancing position
char Tokenizer::peek(size_t offset) const {
    char result = (position + offset < sourceLength) ? source[position + offset] : '\0';
    return result;
}



bool Tokenizer::isWhitespace(char c) const {
    return c == ' ' || c == '\t';
}

bool Tokenizer::isLetter(char c) const {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool Tokenizer::isDigit(char c) const {
    if (position >= sourceLength) {
        throw OutOfBoundsError(line, column, currentLineText);
    }
    return std::isdigit(static_cast<unsigned char>(c));
}

bool Tokenizer::isOperator(char c) const {
    return c == '+' || c == '-' || c == '*' || c == '/' || 
           c == '%' || c == '=' || c == '<' || c == '>' || 
           c == '&' || c == '|' || c == '!';
}

bool Tokenizer::isPunctuation(char c) const {
    return c == ':' || c == ',' || c == '.' || c == '(' || c == ')' || c == ';' || c == '[' || c == ']';
}


bool Tokenizer::handleOptionalType(Vector<Token>& tokens) {
    if (position < sourceLength && source[position] == ':') {
        int colonCol = column;
        if (!isFunction(previousToken().value) && !isClass(previousToken().value)){
            skipWhitespace();  // Allow optional space
            position++;
            column++;
            skipWhitespace();
            if (isCapitalizedType(position)) {
            // Parse the type
                size_t typeStart = position;
                int typeCol = column;
                while (position < sourceLength &&
                    (isLetter(source[position]) || isDigit(source[position]))) {
                    position++;
                    column++;
                }

                String typeStr = source.substr(typeStart, position - typeStart);
                tokens.emplace_back(TokenType::Punctuation, ":", line, colonCol);
                tokens.emplace_back(TokenType::Type, typeStr, line, typeCol);
                return true;
            } else {
                throw TokenizationError("Expected a capitalized type after ':'", line, column, currentLineText);
            }
        }
        
    }
    return false;
}
