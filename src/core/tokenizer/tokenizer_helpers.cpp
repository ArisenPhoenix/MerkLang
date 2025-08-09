#include "core/types.h"
#include "core/tokenizer.h"

#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"
#include "core/errors.h"


bool Tokenizer::isFunction(size_t startPos) {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    const Token& prev = lastToken();  // Do once

    bool insideFunction = false;
    DEBUG_LOG(LogLevel::DEBUG, "Entering isFunction() at position: ", startPos);
    
    // If the last token was 'def' or 'function', this is a function definition
    if (!tokens.empty() && prev.type == TokenType::Function) {
        DEBUG_LOG(LogLevel::DEBUG, "[isFunction] Identified as function definition at position: ", startPos);
        insideFunction = true;
    }

    size_t lookahead = startPos;
    
    // Skip whitespace after the identifier
    while (lookahead < sourceLength && isWhitespace(source[lookahead])) {
        lookahead++;
    }

    // **Handle Function Definitions**: Check if the previous token was a function definition
    if (!tokens.empty() && (prev.type == TokenType::FunctionDef)) {
        DEBUG_LOG(LogLevel::DEBUG, "[isFunction] Identified as function definition at position: ", startPos);
        insideFunction = true;
    }

    // **Handle Function Calls**: If the next non-whitespace character is '(', it is a function call
    if (lookahead < sourceLength && source[lookahead] == '(') {
        DEBUG_LOG(LogLevel::DEBUG, "[isFunction] Identified as function call at position: ", startPos);
        insideFunction = true;
    }

    DEBUG_LOG(LogLevel::DEBUG, "[isFunction] Identified as regular identifier at position: ", startPos);
    DEBUG_FLOW_EXIT();
    return insideFunction;
}

bool Tokenizer::isFunction(String value){
    return functions.count(value);
    // for (auto& func : functions){
    //     if (value == func){
    //         return true;
    //     }
    // }
    // return false;
}

bool Tokenizer::isClass(String value){
    for (auto& cls : classes){
        if (value == cls){
            return true;
        }
    }
    return false;
}


bool Tokenizer::isClass(size_t startPos) {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    bool insideClass = false;
    DEBUG_LOG(LogLevel::DEBUG, "Entering isFunction() at position: ", startPos);
    const Token& prev = lastToken();  // Do once

    // If the last token was 'Class' or 'function', this is a function definition
    if (!tokens.empty() && prev.type == TokenType::ClassDef) {
        DEBUG_LOG(LogLevel::DEBUG, "[isFunction] Identified as function definition at position: ", startPos);
        insideClass = true;
    }

    size_t lookahead = startPos;
    
    // Skip whitespace after the identifier
    while (lookahead < sourceLength && isWhitespace(source[lookahead])) {
        lookahead++;
    }

    // **Handle Function Definitions**: Check if the previous token was a function definition
    if (!tokens.empty() && (prev.type == TokenType::FunctionDef)) {
        DEBUG_LOG(LogLevel::DEBUG, "[isFunction] Identified as function definition at position: ", startPos);
        insideClass = true;
    }

    // **Handle Function Calls**: If the next non-whitespace character is '(', it is a function call
    if (lookahead < sourceLength && source[lookahead] == '(') {
        DEBUG_LOG(LogLevel::DEBUG, "[isFunction] Identified as function call at position: ", startPos);
        insideClass = true;
    }

    DEBUG_LOG(LogLevel::DEBUG, "[isFunction] Identified as regular identifier at position: ", startPos);
    DEBUG_FLOW_EXIT();
    return insideClass;
}

void Tokenizer::updateCurrentLineText() {
    if (column == 1) { // Start of a new line
        size_t lineEnd = source.find('\n', position);
        if (lineEnd == String::npos) {
            lineEnd = sourceLength; // Handle the last line without a newline
        }
        currentLineText = source.substr(position, lineEnd - position);
    }
}

void Tokenizer::printTokens(bool colored) const {
    Token prev = Token(TokenType::Newline, "newLine", 0, 0);
    for (const auto& token : tokens) {
        if (prev.type == TokenType::Newline && token.type == TokenType::Newline) {
            continue;
        }
        prev = token;
        String tok = colored ? highlight("Token", Colors::green) : "Token";
        std::cout << tok + "(Type: " << tokenTypeToString(token.type, colored)
                  << ", Value: " << token.value
                  << ", Line: " << token.line
                  << ", Column: " << token.column << ")" << std::endl;
    }
}



const Token& Tokenizer::lastToken() const {
    return tokens.back();
}

const Token& Tokenizer::previousToken(size_t offset) const {
    if (tokens.size() >= offset) {
        return tokens[tokens.size() - offset];
    }
    static Token dummyToken(TokenType::Unknown, "", line, column);
    return dummyToken;
}

bool Tokenizer::lastTokenWas(TokenType type, size_t offset) const {
    return tokens.size() >= offset && tokens[tokens.size() - offset].type == type;
}





bool isUpper(char c) {
    return std::isupper(static_cast<unsigned char>(c));
}


bool Tokenizer::isCapitalizedType(size_t pos) const {
    if (pos >= sourceLength) return false;

    if (!std::isupper(static_cast<unsigned char>(source[pos]))) return false;

    // Continue checking for alphanumeric identifier
    while (pos < sourceLength) {
        char c = source[pos];
        if (!(std::isalpha(static_cast<unsigned char>(c)) || std::isdigit(static_cast<unsigned char>(c)))) {
            break;
        }
        pos++;
    }

    return true;
}

