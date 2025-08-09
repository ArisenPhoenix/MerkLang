#include "core/types.h"
#include "core/tokenizer.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"
#include "core/errors.h"





Token Tokenizer::readCompoundOperatorOrPunctuation() {
    int startColumn = column;
    String first = String(1, source[position]);
    String twoCharOp = first + peek(); // Look ahead
    if (twoCharOp == ":=") {
        tokens.push_back(Token(TokenType::VarAssignment, ":=", line, column));
        return lastToken();
    }
    
    else if (twoCharOp == ":;") {
        // Push colon
        tokens.push_back(Token(TokenType::Punctuation, ":", line, column));

        // Simulate Newline + Indent
        tokens.push_back(Token(TokenType::Newline, ";", line, column + 1));
        tokens.push_back(Token(TokenType::Indent, "", line + 1, 1));

        position += 2;
        column = 1;

        // Optionally skip whitespace after ; before statement
        skipWhitespace();
        
        return lastToken();
    }
    
    if (twoCharOp == "==" || twoCharOp == "!=" ||
        twoCharOp == "<=" || twoCharOp == ">=" || twoCharOp == "&&" ||
        twoCharOp == "||" || twoCharOp == "+=" || twoCharOp == "-=" ||
        twoCharOp == "*=" || twoCharOp == "/=" || twoCharOp == "%=" || twoCharOp == "++" || twoCharOp == "->" ) { // last one signifies return value
        tokens.push_back(Token(TokenType::Operator, twoCharOp, line, startColumn));
        return lastToken();
    }

    throw UnknownTokenError("Unexpected compound operator: " + twoCharOp, line, column, currentLineText);
}

void Tokenizer::finalizeIndentation() {                
    while (!indentStack.empty() && indentStack.back() > 0) {
        tokens.emplace_back(TokenType::Dedent, "", line, column);
        indentStack.pop_back();
    }
}

bool Tokenizer::isWhitespace(char c) const {
    return c == ' ' || c == '\t';
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




void Tokenizer::handleIndentation() {
    size_t lineStart = position;

    // Skip leading spaces/tabs to find the first non-whitespace character
    while (lineStart < sourceLength && (source[lineStart] == ' ' || source[lineStart] == '\t')) {
        lineStart++;
    }

    // Skip blank lines (no tokens needed)
    if (lineStart == sourceLength || source[lineStart] == '\n') {
        return;
    }

    // Count leading spaces/tabs
    int newIndentLevel = countLeadingSpaces(source.substr(position, lineStart - position));

    if (newIndentLevel > currentIndent) {
        tokens.emplace_back(TokenType::Indent, "->", line, column);
        indentStack.push_back(newIndentLevel);
    } else if (newIndentLevel < currentIndent) {
        while (!indentStack.empty() && newIndentLevel < indentStack.back()) {
            tokens.emplace_back(TokenType::Dedent, "<-", line, column);
            indentStack.pop_back();
        }
        
        if (insideClass && newIndentLevel <= classIndentLevel) {
            insideClass = false;
        }

        if (indentStack.empty() || newIndentLevel != indentStack.back()) {
            throw IndentationError(line, column, currentLineText);


        }
    }

    currentIndent = newIndentLevel;
    position = lineStart;
}
