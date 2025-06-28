#include "core/types.h"
#include "core/errors.h"


ParserError::ParserError(const String& message, const Token& token, const String& context)
        : MerkError(message), token(token), context(context) {}

String ParserError::errorPrefix() const {
        return highlight("Parser Error: ", Colors::green);
    }

String ParserError::errorHint() const {
    return ""; // Default: No additional hints
}

String ParserError::errorString() const {
    std::ostringstream oss;
    oss << errorPrefix();

    if (!message.empty()) {
        oss << message;
    }

    // Include line and column information
    if (token.line >= 0 && token.column >= 0) {
        oss << "\n  " << highlight("Line: ", Colors::yellow) << token.line
            << ", " << highlight("Column: ", Colors::yellow) << token.column;
    }


    // Handle empty token value
    String tokenRepresentation = isWhitespaceOnly(token.value) ? "<empty>" : token.value;

    // Highlight token within the context if provided
    if (!context.empty()) {
        oss << "\n" << highlightToken(context, tokenRepresentation, Colors::red);
    } else {
        oss << "\n  Token: " << highlight(tokenRepresentation, Colors::red);
    }

    // Add any specific hints
    if (!errorHint().empty()) {
        oss << "\n" << highlight("Hint: ", Colors::green) << errorHint();
    }

    return oss.str();
}

// Specialized Parser Errors

SyntaxError::SyntaxError(const String& message, const Token& token, const String& context)
        : ParserError("Syntax Error: " + message, token, context) {}

String SyntaxError::errorPrefix() const {
    return highlight("SyntaxError: ", Colors::blue);
}

MissingTokenError::MissingTokenError(const Token& token, const String& context)
        : ParserError("Expected token is missing.", token, context) {}

String MissingTokenError::errorPrefix() const {
    return highlight("MissingTokenError: ", Colors::orange);
}

String MissingTokenError::errorHint() const {
    return "A required token was expected but not found.";
}


UnexpectedTokenError::UnexpectedTokenError(const Token& token, const String& expected, const String& context)
    : ParserError(token.value, token, context), expected(expected) {}

String UnexpectedTokenError::errorPrefix() const {
    return highlight("UnexpectedTokenError: ", Colors::red);
}

String UnexpectedTokenError::errorHint() const {
    return "Expected: " + String("'") + highlight(expected, Colors::green) + "'" + "\ngot, " + "Data: " + token.toString();
}



UnclosedBlockError::UnclosedBlockError(const Token& token)
    : ParserError("Unclosed block detected.", token) {}

String UnclosedBlockError::errorPrefix() const {
    return highlight("UnclosedBlockError: ", Colors::pink);
}

String UnclosedBlockError::errorHint() const {
    return "Ensure that all blocks are properly closed (e.g., with dedent or matching braces).";
}


InvalidIndentationError::InvalidIndentationError(const Token& token)
    : ParserError("Invalid indentation.", token) {}

String InvalidIndentationError::errorPrefix() const {
    return highlight("InvalidIndentationError: ", Colors::green);
}

String InvalidIndentationError::errorHint() const {
    return "Check the alignment of indentation. It may be inconsistent with the previous lines.";
}