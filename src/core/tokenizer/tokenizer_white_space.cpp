#include "core/types.h"
#include "core/Tokenizer.hpp"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"
#include "core/errors.h"


Token Tokenizer::readCompoundOperatorOrPunctuation() {
    int startColumn = column;
    String first = String(1, source[position]);
    String twoCharOp = first + peek(); // Look ahead
    if (twoCharOp == ":=") {
        
        return Token(TokenType::VarAssignment, ":=", line, column);
    }
    
    if (twoCharOp == "==" || twoCharOp == "!=" ||
        twoCharOp == "<=" || twoCharOp == ">=" || twoCharOp == "&&" ||
        twoCharOp == "||" || twoCharOp == "+=" || twoCharOp == "-=" ||
        twoCharOp == "*=" || twoCharOp == "/=" || twoCharOp == "%=" || twoCharOp == "++" || twoCharOp == "->" ) { // last one signifies return value
        return Token(TokenType::Operator, twoCharOp, line, startColumn);
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

    while (lineStart < sourceLength && (source[lineStart] == ' ' || source[lineStart] == '\t')) { lineStart++; }

    if (lineStart == sourceLength || source[lineStart] == '\n') { return; }

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


