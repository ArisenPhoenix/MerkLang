#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <string>
#include <set>
#include <vector>
#include <regex>
#include <iostream>
#include <ostream> 
#include "core/types.h"


class Tokenizer {
private:
    String source;
    size_t position;
    size_t sourceLength;
    int line;
    int column;
    int currentIndent;
    int newIndentLevel;
    String currentLineText = "";
    static String defaultHint;
    bool insideParams = false;  // Tracks whether inside function parameters or not
    bool insideArgs = false;    // same but for arguments, KISS

    bool insideClass = false;
    int classIndentLevel = 0;

    std::unordered_set<String> functions;
    std::unordered_set<String> classes;


    Vector<int> indentStack; // Stack to manage indentation levels

    bool isWhitespace(char c) const;
    bool isLetter(char c) const;
    bool isDigit(char c) const;
    bool isOperator(char c) const;
    bool isPunctuation(char c) const;

    bool isFunction(size_t startPos);
    bool isFunction(String value);

    bool isClass(size_t startPos);
    bool isClass(String value);

    const Token& previousToken(size_t offset = 1) const;
    const Token& lastToken() const;
    bool lastTokenWas(TokenType type, size_t offset = 1) const;
    bool isLogicOperator();

    bool isComparisonOperator(char c) const;
    bool isCompoundOperator(char c) const;

    bool isComparisonOrLogicalOperator(char c) const;
    bool isMathOperator(char c) const;

    void skipWhitespace();
    Token readIdentifier();
    Token readNumber();
    Token readString();
    Token readOperatorOrPunctuation();
    Token readCompoundOperatorOrPunctuation();

    void handleIndentation(Vector<Token>& tokens);
    void updateCurrentLineText();
    bool isUpper(char c);

    char peek(size_t offset = 1) const;

    bool isCapitalizedType(size_t pos) const;
    bool handleOptionalType(Vector<Token>& tokens);


public:
    Tokenizer(const String& sourceCode)
    : source(sourceCode), position(0), line(1), column(1), currentIndent(0) {
        indentStack.push_back(0); // Initialize with base indent level
    }

    void finalizeIndentation(Vector<Token>& tokens);

    Vector<Token> tokens;
    Vector<Token> tokenize();

    int countLeadingSpaces(const String& line);

    void printTokens(bool colored = false) const;

    const Vector<Token>& getTokens() const {
        return tokens;
    }

};




inline std::ostream& operator<<(std::ostream& os, TokenType type) {
    os << tokenTypeToString(type);
    return os;
}

#endif // TOKENIZER_H
