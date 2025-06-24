#include "core/types.h"
#include "core/errors.h"
#include <string>


TokenizationError::TokenizationError(const String& message, int line, int column, const String& context)
    : MerkError(message), context(context), line(line), column(column) {}

String TokenizationError::errorString() const {
    std::ostringstream oss;
    oss << highlight("Tokenization Error: ", Colors::orange) << message;

    if (line >= 0 && column >= 0) {
        oss << "\n  " << highlight("Line: ", Colors::yellow) << line
            << ", " << highlight("Column: ", Colors::yellow) << column;
    }

    if (!context.empty()) {
        oss << "\n" << context;
        if (column >= 0 && column < static_cast<int>(context.size())) {
            // Highlight the specific character in the context
            oss << "\n" << highlightText(context, column, Colors::red);
        }
    }
    errorStringCache = oss.str();
    return errorStringCache;
}


UnknownTokenError::UnknownTokenError(const String& token, int line, int column, const String& context)
        : TokenizationError("Unknown token encountered: " + token, line, column, context), token(token) {}

String UnknownTokenError::errorString() const {
    std::ostringstream oss;
    oss << TokenizationError::errorString()
        << "\n" << highlight("Unknown Token: ", Colors::blue) << highlightToken(context, token, Colors::red);
    return oss.str();
}


IndentationError::IndentationError(int line, int column, const String& context)
    : TokenizationError("Unexpected indentation level.", line, column, context) {}

String IndentationError::errorString() const {
    std::ostringstream oss;
    oss << TokenizationError::errorString()
        << "\n" << highlight("Hint: ", Colors::green) << "Check the indentation alignment.";
    return oss.str();
}


OutOfBoundsError::OutOfBoundsError(int line, int column, const String& context)
    : TokenizationError("Position out of bounds in the source code.", line, column, context) {}

String OutOfBoundsError::errorString() const {
    std::ostringstream oss;
    oss << highlight("Out of Bounds Error: ", Colors::purple) << message;

    if (line >= 0 && column >= 0) {
        oss << "\n  " << highlight("Line: ", Colors::yellow) << line
            << ", " << highlight("Column: ", Colors::yellow) << column;
    }

    if (!context.empty()) {
        oss << "\n" << context;
        if (column >= 0 && column < static_cast<int>(context.size())) {
            // Highlight the out-of-bounds character or surrounding area
            oss << "\n" << highlightText(context, column, Colors::red);
        }
    }
    return oss.str();
}


UnmatchedQuoteError::UnmatchedQuoteError(int line, int column, const String& context)
    : TokenizationError("Unmatched quote detected in the source code.", line, column, context) {}

String UnmatchedQuoteError::errorString() const {
    std::ostringstream oss;
    oss << highlight("Unmatched Quote Error: ", Colors::pink) << message;

    if (line >= 0 && column >= 0) {
        oss << "\n  " << highlight("Line: ", Colors::yellow) << line
            << ", " << highlight("Column: ", Colors::yellow) << column;
    }

    if (!context.empty()) {
        oss << "\n" << context;
        if (column >= 0 && column < static_cast<int>(context.size())) {
            // Highlight the unmatched quote
            oss << "\n" << highlightText(context, column, Colors::red);
        }
    }
    return oss.str();
}
