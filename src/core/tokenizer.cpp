#include "core/types.h"
#include "core/tokenizer.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"
#include "core/errors.h"


Vector<Token> Tokenizer::tokenize() {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    while (position < source.size()) {
        DEBUG_LOG(LogLevel::DEBUG, "Processing char: ", source[position], " at position: ", position);

        // Skip whitespace
        while (position < source.size() && isWhitespace(source[position])) {
            position++;
            column++;
        }

        if (source[position] == '\n') {
            // Secondary Backslash for viewing properly in the output
            tokens.emplace_back(TokenType::Newline, "\\n", line, column);
            position++;
            line++;
            column = 1;
            handleIndentation(tokens);
            continue;
        }

        // Handle comments
        if (position < source.size() && source[position] == '#') {
            while (position < source.size() && source[position] != '\n') {
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
            tokens.push_back(readIdentifier());
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

    while (position < source.size() && isDigit(source[position])) {
        position++;
        column++;
    }

    return Token(TokenType::Number, source.substr(start, position - start), line, startColumn);
}

Token Tokenizer::readString() {
    int startColumn = column;

    // Move past the opening double-quote
    position++;
    column++;

    String result;
    while (position < source.size() && source[position] != '"') {
        if (source[position] == '\\') { // Handle escape sequences
            if (position + 1 >= source.size()) {
                // throw RunTimeError("Unfinished escape sequence in string literal.");
                TokenizationError("Unfinished escape sequence in string literal.", line, column);
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

    // Ensure the string is properly terminated
    if (position >= source.size() || source[position] != '"') {
        // throw RunTimeError("Unmatched double-quote in string literal.");
        throw UnmatchedQuoteError(line, column, currentLineText);
    }

    // Move past the closing double-quote
    position++;
    column++;

    return Token(TokenType::String, result, line, startColumn);
}

Token Tokenizer::readIdentifier() {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    DEBUG_LOG(LogLevel::DEBUG, "Entering readIdentifier at position: ", position);

    if (position >= source.size()) {
        throw TokenizationError("Unexpected end of source in readIdentifier.", line, column, currentLineText);
    }

    size_t start = position;
    int startColumn = column;

    // Read identifier (letters, digits, underscores)
    while (position < source.size() && (isLetter(source[position]) || isDigit(source[position]) || source[position] == '_')) {
        position++;
        column++;
    }

    String value = source.substr(start, position - start);
    TokenType type = TokenType::Variable;  // Default to Variable

    DEBUG_LOG(LogLevel::DEBUG, "Found identifier: ", value);

    // Function Definition Keyword?
    if (value == "function" || value == "def") {
        type = TokenType::FunctionDef;
        insideParams = true; // Keep this active until after ')'
    }
    
    // Function Name (Immediately after `def`)
    // else if (tokens.size() > 0 && tokens.back().type == TokenType::FunctionDef || tokens.back().type == TokenType::Punctuation) {
    else if (tokens.size() > 0 && tokens.back().type == TokenType::FunctionDef){
        type = TokenType::FunctionRef;
        functions.insert(value);
    }

    // Function Name (Immediately after `FunctionDef`
    else if (isFunction(position)) {
        type = TokenType::FunctionCall;
        insideArgs = true;
    }

    else if (isFunction(value)){
        type = TokenType::FunctionRef;
    }

    // Function Argument (Immediately after `call` and inside function call lists)
    else if (insideArgs && tokens.back().type == TokenType::Punctuation){
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

    // Loop Keywords
    else if (value == "for" || value == "while" || value == "break" || value == "return") {
        type = TokenType::Keyword;
    }

    // Boolean Literals
    else if (value == "true" || value == "false") {
        type = TokenType::Bool;
    }

    // Function Call?
    else if (isFunction(position)) {
        type = TokenType::FunctionCall;
        DEBUG_LOG(LogLevel::DEBUG, "Classified as function call: ", value);
    }

    DEBUG_LOG(LogLevel::DEBUG, "Exiting readIdentifier with value: ", value, " (Type: ", tokenTypeToString(type), ")");
    DEBUG_FLOW_EXIT();
    return Token(type, value, line, startColumn);
}

bool Tokenizer::isWhitespace(char c) const {
    return c == ' ' || c == '\t';
}

bool Tokenizer::isLetter(char c) const {
    if (position >= source.size()) {
        throw OutOfBoundsError(line, column, currentLineText);
    }
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool Tokenizer::isDigit(char c) const {
    if (position >= source.size()) {
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

bool Tokenizer::isFunction(size_t startPos) {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    bool insideFunction = false;
    DEBUG_LOG(LogLevel::DEBUG, "Entering isFunction() at position: ", startPos);
    
    // If the last token was 'def' or 'function', this is a function definition
    if (!tokens.empty() && tokens.back().type == TokenType::Function) {
        DEBUG_LOG(LogLevel::DEBUG, "[isFunction] Identified as function definition at position: ", startPos);
        insideFunction = true;
    }

    size_t lookahead = startPos;
    
    // Skip whitespace after the identifier
    while (lookahead < source.size() && isWhitespace(source[lookahead])) {
        lookahead++;
    }

    // **Handle Function Definitions**: Check if the previous token was a function definition
    if (!tokens.empty() && (tokens.back().type == TokenType::FunctionDef)) {
        DEBUG_LOG(LogLevel::DEBUG, "[isFunction] Identified as function definition at position: ", startPos);
        insideFunction = true;
    }

    // **Handle Function Calls**: If the next non-whitespace character is '(', it is a function call
    if (lookahead < source.size() && source[lookahead] == '(') {
        DEBUG_LOG(LogLevel::DEBUG, "[isFunction] Identified as function call at position: ", startPos);
        insideFunction = true;
    }

    DEBUG_LOG(LogLevel::DEBUG, "[isFunction] Identified as regular identifier at position: ", startPos);
    DEBUG_FLOW_EXIT();
    return insideFunction;
}

bool Tokenizer::isFunction(String value){
    for (auto& func : functions){
        if (value == func){
            return true;
        }
    }
    return false;
}

void Tokenizer::skipWhitespace() {
    while (position < source.size() && isWhitespace(source[position])) {
        position++;
        column++;
    }

    if (position >= source.size()) {
        OutOfBoundsError(line, column, currentLineText);
    }
}

void Tokenizer::handleIndentation(Vector<Token>& tokens) {
    size_t lineStart = position;

    // Skip leading spaces/tabs to find the first non-whitespace character
    while (lineStart < source.size() && (source[lineStart] == ' ' || source[lineStart] == '\t')) {
        lineStart++;
    }

    // Skip blank lines (no tokens needed)
    if (lineStart == source.size() || source[lineStart] == '\n') {
        return;
    }

    // Count leading spaces/tabs
    int newIndentLevel = countLeadingSpaces(source.substr(position, lineStart - position));

    if (newIndentLevel > currentIndent) {
        tokens.emplace_back(TokenType::Indent, "", line, column);
        indentStack.push_back(newIndentLevel);
    } else if (newIndentLevel < currentIndent) {
        while (!indentStack.empty() && newIndentLevel < indentStack.back()) {
            tokens.emplace_back(TokenType::Dedent, "", line, column);
            indentStack.pop_back();
        }

        if (indentStack.empty() || newIndentLevel != indentStack.back()) {
            throw IndentationError(line, column, currentLineText);


        }
    }

    currentIndent = newIndentLevel;
    position = lineStart;
}

void Tokenizer::updateCurrentLineText() {
        // Update currentLineText on newline or at the start
        if (column == 1) { // Start of a new line
            size_t lineEnd = source.find('\n', position);
            if (lineEnd == String::npos) {
                lineEnd = source.size(); // Handle the last line without a newline
            }
            currentLineText = source.substr(position, lineEnd - position);
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
    char result = (position + offset < source.size()) ? source[position + offset] : '\0';
    return result;
}

void Tokenizer::printTokens(bool colored) const {
    for (const auto& token : tokens) {
        String tok = colored ? highlight("Token", Colors::green) : "Token";
        std::cout << tok + "(Type: " << tokenTypeToString(token.type, colored)
                  << ", Value: " << token.value
                  << ", Line: " << token.line
                  << ", Column: " << token.column << ")" << std::endl;
    }
}

