#include "core/types.h"
#include "core/Tokenizer.hpp"
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

const std::unordered_set<std::string> knownTypes = {
    "Int", "Float", "Long", "Bool", "String", "Array", "List", "Map", "Schema", "Set", "Dict", "Double", "Http", "File"
};

const std::unordered_set<String> keywords = {
    "if", "elif", "else", "from", "as", "import", "while", "continue", "break", "return", "for", "throw"
};

const std::unordered_set<char> typeContainers = {
    '{', '<', '[', ']', '>', '}'
};

static bool isCompoundPair(char a, char b) {
    switch (a) {
        case '<': return b == '=' || b == '<';
        case '>': return b == '=' || b == '>';
        case '=': return b == '=';
        case '!': return b == '=';
        case '+': return b == '=' || b == '+';
        case '-': return b == '=' || b == '-' || b == '>';
        case '*': return b == '=' || b == '*';
        case '/': return b == '=' || b == '/';
        case '&': return b == '&' || b == '=';
        case '|': return b == '|' || b == '=';
        case ':': return b == ':' || b == '=';
        default: return false;
    }
}


bool Tokenizer::handleContainers() {
    bool found = false;
    if (source[position] == '[') {
        tokens.emplace_back(TokenType::LeftBracket, "[", line, column);
        found = true;
    }

    else if (source[position] == ']') {
        tokens.emplace_back(TokenType::RightBracket, "]", line, column);
        found = true;
    }

    else if (source[position] == '(') {
        tokens.push_back(Token(TokenType::Punctuation, "(", line, column));
        found = true;
    }

    else if (source[position] == ')') {
        tokens.push_back(Token(TokenType::Punctuation, ")", line, column));
        insideArgs = false;
        insideParams = false;
        found = true;
    }

    else if (source[position] == '<') {
        if (peek() == '=' || peek() == '<') {return false;}
        tokens.push_back(Token(TokenType::Operator, "<", line, column));
        found = true;
    }

    else if (source[position] == '>') {
        if (peek() == '=' || peek() == '>') {return false;}
        tokens.push_back(Token(TokenType::Operator, ">", line, column));
        found = true;
    }

    else if (source[position] == '{') {
        tokens.push_back(Token(TokenType::Operator, "{", line, column));
        found = true;
    }

    else if (source[position] == '}') {
        tokens.push_back(Token(TokenType::Operator, "}", line, column));
        found = true;
    }

    if (found) {
        position++;
        column++;
    }

    return found;
}

Tokenizer::Tokenizer(const String& sourceCode)
    : source(sourceCode), position(0), line(1), column(1), currentIndent(0) {
        indentStack.push_back(0); // Initialize with base indent level
        tokens.emplace_back(Token(TokenType::SOF_Token, "SOF_TOKEN", 0, 0));
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

        if (source[position] == '\n' || source[position] == ';') {
            String val = "";
            source[position] == '\n' ? val += "NewLine" : val += ";";
            tokens.emplace_back(Token(TokenType::Newline, val, line, column));
            position++;
            line++;
            column = 1;
            handleIndentation();
            continue;
        }


        // Handle comments - perhaps will use them for something else later
        if (position < sourceLength && source[position] == '#') {
            while (position < sourceLength && source[position] != '\n') {
                position++;
            }
            continue;
        }

        if (source[position] == ':' && peek() == ';') {
            // ':' ends clause, ';' means "indent next statement"
            tokens.push_back(Token(TokenType::Punctuation, ":", line, column));

            // End-of-line semantics (but not an actual '\n' in source)
            tokens.push_back(Token(TokenType::Newline, ";", line, column + 1));

            // Create an indent token so the parser sees a block
            tokens.push_back(Token(TokenType::Indent, "->", line, 1));

            position += 2;
            column += 2;
            continue;
        }

        else if (source[position] == ',') {
            tokens.push_back(Token(TokenType::Punctuation, ",", line, column));
            position++;
            column++;
            continue;
        }

        if (position + 1 < sourceLength && isCompoundPair(source[position], peek())) {
            tokens.push_back(readCompoundOperatorOrPunctuation());
            position += 2;
            column += 2;
            continue;
        }

        if (handleContainers()) {
            continue;
        }

        if (isOperator(source[position]) || isPunctuation(source[position])) {
            tokens.push_back(readOperatorOrPunctuation());
            position++;
            column++;
            continue;
        }

        else if (isLetter(source[position])) {
            Token identifier = readIdentifier();  // advances past identifier
            
            if (knownTypes.count(identifier.value)) {
                identifier.type = TokenType::Type;
            }

            char nextChar = position < sourceLength ? source[position] : '\0';
            char nextNextChar = peek();
        
            if (identifier.type == TokenType::Variable &&
                previousToken().value != "." &&
                (nextChar == '.' || (nextChar == ':' && nextNextChar == ':'))) {
                identifier.type = TokenType::ChainEntryPoint;
            }

            if (identifier.type == TokenType::Type && nextChar == '(') {
                if (knownTypes.count(identifier.value)) {
                    identifier.type = TokenType::ClassCall;
                }
                else {
                    identifier.type = TokenType::FunctionCall;
                }
            }

            if (identifier.value == "null" && identifier.type == TokenType::Variable) {
                identifier.type = TokenType::String;
            }
        
            tokens.push_back(identifier);
            skipWhitespace();

            
            // if (previousToken().type == TokenType::Variable) {
            //     if (source[position] == ':' && peek() != '=') {
            //         handleOptionalType();
            //     } 
            //     continue;  // already tokenized the type annotation
            // }

            if (previousToken().type == TokenType::Variable ||
                previousToken().type == TokenType::ChainEntryPoint) {
                if (source[position] == ':' && peek() != '=') {
                    handleOptionalType();
                }
                continue;
            }

        } else if (isDigit(source[position])) {
            tokens.push_back(readNumber());
        } else if (source[position] == '"' || source[position] == '\'' || source[position] == '`') {
            tokens.push_back(readString());
        } else {
            DEBUG_LOG(LogLevel::DEBUG, "Unknown token at line ", line, ", column ", column, " (", source[position], ")");
            tokens.emplace_back(TokenType::Unknown, String(1, source[position]), line, column);
            position++;
            column++;
        }
    }

    finalizeIndentation();
    tokens.emplace_back(TokenType::EOF_Token, "EOF", line, column);
    DEBUG_FLOW_EXIT();
    return tokens;
}

Token Tokenizer::readOperatorOrPunctuation() {
    char c = source[position];
    if (c == '=') { return Token(TokenType::VarAssignment, "=", line, column); }

    if (isOperator(c)) {
        if (c == '<' && lastTokenWas(TokenType::VarAssignment)) {
            return Token(TokenType::Operator, String(1, c), line, column);
        } 
        return Token(TokenType::Operator, String(1, c), line, column);
    };

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

bool handleSpecialChar(char nextChar, char startChar, String& resultAccum) {
    if (nextChar == startChar) { return true; }
    switch (nextChar) {
        case 'n': resultAccum += '\n'; break;
        case 't': resultAccum += '\t'; break;
        case 'r': resultAccum += '\r'; break;
        case '\\': resultAccum += '\\'; break;
        
        case '\'': resultAccum += '\''; break;
        case '"': resultAccum += '"'; break;
        default:
            return false;
    }
    return true;
}

Token Tokenizer::readString() {
    int startColumn = column;
    String result;

    char startChar = source[position];
    TokenType type = startChar == '"' ? TokenType::String : startChar == '\'' ? TokenType::Char : TokenType::Text;

    position++;
    column++;

    while (position < sourceLength && source[position] != startChar) {
        if (source[position] == '\\') { // Handle escape sequences
            if (position + 1 >= sourceLength) {
                throw TokenizationError("Unfinished escape sequence in string literal.", line, column);
            }

            char nextChar = source[++position];
            column++;
            if (!handleSpecialChar(nextChar, startChar, result)) {
                throw UnknownTokenError(std::to_string(nextChar), line, column, currentLineText);
            }

        } else {
            result += source[position];
        }

        position++;
        column++;
    }

    if (position >= sourceLength || source[position] != startChar) {
        throw UnmatchedQuoteError(line, column, currentLineText);
    }

    position++;
    column++;

    return Token(type, result, line, startColumn);
}

Token Tokenizer::readIdentifier() {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    DEBUG_LOG(LogLevel::DEBUG, "Entering readIdentifier at position: ", position);

    if (position >= sourceLength) {
        throw TokenizationError("Unexpected end of source in readIdentifier.", line, column, currentLineText);
    }

    size_t start = position;
    int startColumn = column;

    while (position < sourceLength && (isLetter(source[position]) || isDigit(source[position]) || source[position] == '_')) {
        position++;
        column++;
    }

    String value = source.substr(start, position - start);
    TokenType type = TokenType::Variable;  // Default to Variable
    DEBUG_LOG(LogLevel::DEBUG, "Found identifier: ", value);
    const Token& prev = lastToken();

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

    else if (value == "||" || value == "&&") {
        type = TokenType::Operator;
    }
    
    else if (value == "function" || value == "def") {
        type = insideClass ? TokenType::ClassMethodDef : TokenType::FunctionDef; 
        insideParams = true;
    }

    else if (!tokens.empty() && prev.type == TokenType::ClassDef) {
        type = TokenType::ClassRef;
        classes.insert(value);
    }
    
    else if (!tokens.empty() && (prev.type == TokenType::FunctionDef || prev.type == TokenType::ClassMethodDef)){
        type = insideClass ? TokenType::ClassMethodRef : TokenType::FunctionRef;
        functions.insert(value);
    }

    else if (value == "true" || value == "false") {
        type = TokenType::Bool;
    }

    else if (isClass(value)) {
        type = TokenType::ClassCall;
        insideArgs = true;
    }

    else if (value == "var" || value == "const") {
        type = TokenType::VarDeclaration;
    }

    else if (keywords.count(value)) {
        type = TokenType::Keyword;
    }

    // Function Name (Immediately after `FunctionDef`
    else if (isFunction(position)) {
        if (previousToken().value == ".") {
            type = TokenType::ClassMethodCall;
        } else {
            type = TokenType::FunctionCall;
        }
        insideArgs = true;
    }

    // Function Argument (Immediately after `call` and inside function call lists)
    else if (insideArgs && prev.type == TokenType::Punctuation){
        type = TokenType::Argument;
    }

    // Function Parameters (Only inside function parameter lists)
    else if (insideParams) {
        type = TokenType::Parameter;
    }

    else if (functions.count(value)) {
        type = TokenType::FunctionRef; 
    }

    else if (classes.count(value)) {
        type = TokenType::ClassRef;
    }

    if (type == TokenType::FunctionRef || type == TokenType::ClassMethodRef) {
        if (peek() == '.' || prev.type == TokenType::VarDeclaration || (prev.type == TokenType::VarAssignment && peek() == '.')) {
            type = TokenType::Variable;
        }
    }
    
    // Boolean Literals
    DEBUG_LOG(LogLevel::DEBUG, "Exiting readIdentifier with value: ", value, " (Type: ", tokenTypeToString(type), ")");
    DEBUG_FLOW_EXIT();
    auto token = Token(type, value, line, startColumn);
    return token;
}

bool Tokenizer::isCompoundOperator(char c) const {
    // Look ahead to check for `=`
    if (c == '+' && peek() == '+') {return true;}
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
    return c == '=' || c == '<' || c == '>' || c == '!' || c == '&' || c == '|' || c == '?';
}

// Peeks at the next character without advancing position
char Tokenizer::peek(size_t offset) const {
    char result = (position + offset < sourceLength) ? source[position + offset] : '\0';
    return result;
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

bool Tokenizer::handleOptionalType() {
    if (position < sourceLength && source[position] == ':' && peek() != '=') {
        tokens.emplace_back(Token(TokenType::Punctuation, ":", line, column));
        position++; column++;          // consume ':'
        skipWhitespace();              // now at type name

        // parse single identifier as type-name
        if (!isLetter(source[position])) return false;

        size_t start = position;
        int typeCol = column;
        while (position < sourceLength &&
               (isLetter(source[position]) || isDigit(source[position]) || source[position]=='_')) {
            position++; column++;
        }
        String typeName = source.substr(start, position - start);

        tokens.emplace_back(Token(TokenType::Type, typeName, line, typeCol));

        // (optional) parse container syntax immediately after, like List[Int]
        skipWhitespace();
        if (position < sourceLength && typeContainers.count(source[position])) {
            handleContainers();
        }

        return true;
    }
    return false;
}


// bool Tokenizer::handleOptionalType() {
//     if (position < sourceLength && source[position] == ':') {
//         tokens.emplace_back(Token(TokenType::Punctuation, ":", line, column));
//         if (!isFunction(previousToken().value) && !isClass(previousToken().value)) {
//             skipWhitespace();  // Allow optional space
//             position++;
//             column++;

//             if (typeContainers.count(source[position])) {  //&& source[position] != '(' && source[position] != ')'
//                 handleContainers();
//             }

//             if (isCapitalizedType(position)) {
//                 size_t typeStart = position;
//                 int typeCol = column;
//                 while (position < sourceLength &&
//                     (isLetter(source[position]) || isDigit(source[position]))) {
//                     position++;
//                     column++;
//                 }

//                 String typeStr = source.substr(typeStart, position - typeStart);
                
//                 tokens.emplace_back(Token(TokenType::Type, typeStr, line, typeCol));

//                 skipWhitespace();
//                 if (typeContainers.count(source[position])) {  //&& source[position] != '(' && source[position] != ')'
//                     handleContainers();
//                 }
//             }
//             return true;
//         }
//     }
//     return false;
// }
