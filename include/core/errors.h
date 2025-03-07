#ifndef ERRORS_H
#define ERRORS_H

#include <stdexcept>
#include <string>
#include <sstream>
#include "types.h"



inline bool isWhitespaceOnly(const String& str) {
    return str.find_first_not_of(" \t\n\r") == String::npos;
}

// Base Error Class: MerkError
class MerkError : public RunTimeError {
protected:
    String message;
    mutable String errorStringCache; // Cache for detailed message

public:
    MerkError(const String& message)
        : RunTimeError(message), message(message) {}

    virtual String errorString() const {
        return highlight("MerkError", Colors::red) + ": " + highlight(message, Colors::yellow);
    }

    const char* what() const noexcept override {
        errorStringCache = errorString(); // Cache the result
        return errorStringCache.c_str(); // Return the cached string
    }

    virtual ~MerkError() = default;
};


// =============================================== Tokenizer =================================================
// TokenizationError Class
class TokenizationError : public MerkError {
protected:
    String context;
    int line;
    int column;

public:
    TokenizationError(const String& message, int line = -1, int column = -1, const String& context = "")
        : MerkError(message), context(context), line(line), column(column) {}

    String errorString() const override {
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

};

class UnknownTokenError : public TokenizationError {
    String token;

public:
    UnknownTokenError(const String& token, int line, int column, const String& context = "")
        : TokenizationError("Unknown token encountered: " + token, line, column, context), token(token) {}

    String errorString() const override {
        std::ostringstream oss;
        oss << TokenizationError::errorString()
            << "\n" << highlight("Unknown Token: ", Colors::blue) << highlightToken(context, token, Colors::red);
        return oss.str();
    }
};

class IndentationError : public TokenizationError {
public:
    IndentationError(int line, int column, const String& context = "")
        : TokenizationError("Unexpected indentation level.", line, column, context) {}

    String errorString() const override {
        std::ostringstream oss;
        oss << TokenizationError::errorString()
            << "\n" << highlight("Hint: ", Colors::green) << "Check the indentation alignment.";
        return oss.str();
    }
};

class OutOfBoundsError : public TokenizationError {
    String context;

public:
    OutOfBoundsError(int line, int column, const String& context = "")
        : TokenizationError("Position out of bounds in the source code.", line, column), context(context) {}

    String errorString() const override {
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
};

class UnmatchedQuoteError : public TokenizationError {
    String context;

public:
    UnmatchedQuoteError(int line, int column, const String& context = "")
        : TokenizationError("Unmatched quote detected in the source code.", line, column), context(context) {}

    String errorString() const override {
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
};



// =============================================== Parser =================================================

// Base ParserError Class
class ParserError : public MerkError {
protected:
    Token token; // The token causing the error
    String context;

public:
    ParserError(const String& message, const Token& token, const String& context = "")
        : MerkError(message), token(token), context(context) {}

    virtual String errorPrefix() const {
        return highlight("Parser Error: ", Colors::green);
    }

    virtual String errorHint() const {
        return ""; // Default: No additional hints
    }

    String errorString() const override {
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
};

// Specialized Parser Errors

class SyntaxError : public ParserError {
public:
    SyntaxError(const String& message, const Token& token, const String& context = "")
        : ParserError("Syntax Error: " + message, token, context) {}

    String errorPrefix() const override {
        return highlight("Syntax Error: ", Colors::blue);
    }
};

class MissingTokenError : public ParserError {
public:
    MissingTokenError(const Token& token, const String& context = "")
        : ParserError("Expected token is missing.", token, context) {}

    String errorPrefix() const override {
        return highlight("Missing Token Error: ", Colors::orange);
    }

    String errorHint() const override {
        return "A required token was expected but not found.";
    }
};

class UnexpectedTokenError : public ParserError {
    String expected;

public:
    UnexpectedTokenError(const Token& token, const String& expected, const String& context = "")
        : ParserError(token.value, token, context), expected(expected) {}

    String errorPrefix() const override {
        return highlight("Unexpected Token Error: ", Colors::red);
    }

    String errorHint() const override {
        return "Expected: " + String("'") + highlight(expected, Colors::green) + "'";
    }
};

class UnclosedBlockError : public ParserError {
public:
    UnclosedBlockError(const Token& token)
        : ParserError("Unclosed block detected.", token) {}

    String errorPrefix() const override {
        return highlight("Unclosed Block Error: ", Colors::pink);
    }

    String errorHint() const override {
        return "Ensure that all blocks are properly closed (e.g., with dedent or matching braces).";
    }
};

class InvalidIndentationError : public ParserError {
public:
    InvalidIndentationError(const Token& token)
        : ParserError("Invalid indentation.", token) {}

    String errorPrefix() const override {
        return highlight("Invalid Indentation Error: ", Colors::green);
    }

    String errorHint() const override {
        return "Check the alignment of indentation. It may be inconsistent with the previous lines.";
    }
};













// =============================================== Evaluator =================================================

class EvaluationError : public MerkError {
protected:
    String context; // Optional context for evaluation errors

public:
    EvaluationError(const String& message, const String& context = "")
        : MerkError(message), context(context) {}

    virtual String errorPrefix() const {
        return highlight("Evaluation Error: ", Colors::blue);
    }

    virtual String errorHint() const {
        return ""; // No additional hints by default
    }

    String errorString() const override {
        std::ostringstream oss;
        oss << errorPrefix() << message;

        if (!context.empty()) {
            oss << "\n" << highlight("Context: ", Colors::yellow) << context;
        }

        if (!errorHint().empty()) {
            oss << "\n" << highlight("Hint: ", Colors::green) << errorHint();
        }

        return oss.str();
    }
};


class UndefinedVariableError : public EvaluationError {
public:
    UndefinedVariableError(const String& variableName, const String& context = "")
        : EvaluationError("Undefined variable: '" + variableName + "' was not declared.", context) {}

    String errorPrefix() const override {
        return highlight("Undefined Variable Error: ", Colors::red);
    }

    String errorHint() const override {
        return "Declare the variable before using it.";
    }
};


class TypeMismatchError : public EvaluationError {
public:
    TypeMismatchError(const String& expected, const String& actual, const String& context = "")
        : EvaluationError("Type mismatch: expected '" + expected + "', but found '" + actual + "'.", context) {}

    String errorPrefix() const override {
        return highlight("Type Mismatch Error: ", Colors::purple);
    }

    String errorHint() const override {
        return "Ensure that the operation involves compatible types.";
    }
};


class InvalidOperationError : public EvaluationError {
public:
    InvalidOperationError(const String& operation, const String& context = "")
        : EvaluationError("Invalid operation: '" + operation + "' cannot be performed.", context) {}

    String errorPrefix() const override {
        return highlight("Invalid Operation Error: ", Colors::orange);
    }

    String errorHint() const override {
        return "Check the operator and operands for correctness.";
    }
};


class BlockEvaluationError : public EvaluationError {
public:
    BlockEvaluationError(const String& message, const String& context = "")
        : EvaluationError("Error while evaluating a block: " + message, context) {}

    String errorPrefix() const override {
        return highlight("Block Evaluation Error: ", Colors::green);
    }

    String errorHint() const override {
        return "Ensure that all nodes in the block are valid and executable.";
    }
};


class BreakOutsideLoopError : public EvaluationError {
public:
    BreakOutsideLoopError(const String& context = "")
        : EvaluationError("Break statement encountered outside of a loop.", context) {}

    String errorPrefix() const override {
        return highlight("Break Outside Loop Error: ", Colors::red);
    }

    String errorHint() const override {
        return "Break statements can only be used within loops.";
    }
};


class UnreachableCodeError : public EvaluationError {
public:
    UnreachableCodeError(const String& context = "")
        : EvaluationError("Unreachable code was executed. This may indicate a bug.", context) {}

    String errorPrefix() const override {
        return highlight("Unreachable Code Error: ", Colors::red);
    }

    String errorHint() const override {
        return "Review the AST structure and evaluation logic.";
    }
};









// =============================================== Context =================================================

class ContextError : public MerkError {
public:
    explicit ContextError(const String& message)
        : MerkError(message) {}

    virtual String errorPrefix() const {
        return highlight("Context Error: ", Colors::blue);
    }

    String errorString() const override {
        if (errorStringCache.empty()) { // Use the cached message
            std::ostringstream oss;
            oss << errorPrefix() << message;
            errorStringCache = oss.str(); // Cache the detailed message
        }
        return errorStringCache;
    }
    virtual String errorHint() const {
        return "";
    }
};

class InvalidVariableUpdateError : public ContextError {
public:
    explicit InvalidVariableUpdateError(const String& variableName)
        : ContextError("Invalid update for variable '" + variableName + "'. The new value is not compatible.") {}

    String errorPrefix() const override {
        return highlight("Invalid Variable Update Error: ", Colors::orange);
    }

    String errorHint() const override {
        return "Check the type or value of the new assignment.";
    }
};


class NullVariableError : public ContextError {
public:
    explicit NullVariableError(const String& variableName)
        : ContextError("Variable '" + variableName + "' is null and cannot be accessed or assigned.") {}

    String errorPrefix() const override {
        return highlight("Null Variable Error: ", Colors::pink);
    }

    String errorHint() const override {
        return "Ensure the variable is initialized with a valid value.";
    }
};


class VariableAlreadyDeclaredError : public ContextError {
public:
    explicit VariableAlreadyDeclaredError(const String& variableName)
        : ContextError("Variable '" + variableName + "' has already been declared in the current context.") {}

    String errorPrefix() const override {
        return highlight("Variable Already Declared Error: ", Colors::yellow);
    }

    String errorHint() const override {
        return "Consider using a different name or updating the existing variable if allowed.";
    }
};



class ImmutableVariableError : public ContextError {
public:
    explicit ImmutableVariableError(const String& variableName)
        : ContextError("Variable '" + variableName + "' is immutable and cannot be reassigned or updated.") {}

    String errorPrefix() const override {
        return highlight("Immutable Variable Error: ", Colors::purple);
    }

    String errorHint() const override {
        return "Ensure the variable is declared as mutable if reassignment is required.";
    }
};








// =============================================== Scope =================================================
class ScopeError : public MerkError {
public:
    explicit ScopeError(const String& message)
        : MerkError(message) {}

    virtual String errorPrefix() const {
        return highlight("Scope Error: ", Colors::blue);
    }

    String errorString() const override {
        if (errorStringCache.empty()) { // Use the cached message
            std::ostringstream oss;
            oss << errorPrefix() << message;
            errorStringCache = oss.str(); // Cache the detailed message
        }
        return errorStringCache;
    }

    virtual String errorHint() const {
        return "";
    }
};

class ParentScopeNotFoundError : public ScopeError {
public:
    explicit ParentScopeNotFoundError()
        : ScopeError("Parent scope not found. This operation is invalid for the root scope.") {}

    String errorPrefix() const override {
        return highlight("Parent Scope Not Found Error: ", Colors::red);
    }

    String errorHint() const override {
        return "Ensure this operation is only performed within a non-root scope.";
    }
};


class VariableNotFoundError : public ScopeError {
public:
    explicit VariableNotFoundError(const String& variableName)
        : ScopeError("Variable '" + variableName + "' not found in any accessible scope.") {}

    String errorPrefix() const override {
        return highlight("Variable Not Found Error: ", Colors::red);
    }

    String errorHint() const override {
        return "Ensure the variable is declared before accessing or updating it.";
    }
};




class ImmutableVariableUpdateError : public ScopeError {
public:
    explicit ImmutableVariableUpdateError(const String& variableName)
        : ScopeError("Variable '" + variableName + "' is immutable and cannot be updated.") {}

    String errorPrefix() const override {
        return highlight("Immutable Variable Update Error: ", Colors::purple);
    }

    String errorHint() const override {
        return "Declare the variable as mutable if updates are required.";
    }
};


class ChildScopeCreationError : public ScopeError {
public:
    explicit ChildScopeCreationError()
        : ScopeError("Failed to create a child scope. Ensure sufficient memory and a valid parent scope.") {}

    String errorPrefix() const override {
        return highlight("Child Scope Creation Error: ", Colors::orange);
    }

    String errorHint() const override {
        return "Check for memory constraints or invalid parent scope configurations.";
    }
};




// ================================= Functions ==========================================
class ArgumentError : public EvaluationError {
private:
    String functionName;
    size_t expectedArgs;
    size_t receivedArgs;

public:
    ArgumentError(const String& functionName, size_t expected, size_t received)
        : EvaluationError("Function '" + functionName + "' expects " + std::to_string(expected) +
                          " arguments, but " + std::to_string(received) + " were provided."),
          functionName(functionName), expectedArgs(expected), receivedArgs(received) {}

    String errorPrefix() const override {
        return highlight("Argument Error: ", Colors::red);
    }

    String errorHint() const override {
        return "Check the number of arguments passed to the function.";
    }

    String errorString() const override {
        std::ostringstream oss;
        oss << errorPrefix()
            << "Function '" << functionName << "' expects " << expectedArgs
            << " arguments, but " << receivedArgs << " were provided.";
        if (!errorHint().empty()) {
            oss << "\n" << highlight("Hint: ", Colors::green) << errorHint();
        }
        return oss.str();
    }
};

















// ================================= Functions ==========================================

// Raised when calling a function that does not exist in the scope.
class FunctionNotFoundError : public EvaluationError {
public:
    explicit FunctionNotFoundError(const String& functionName)
        : EvaluationError("Function '" + functionName + "' is not defined in this scope.") {}

    String errorPrefix() const override {
        return highlight("Function Not Found Error: ", Colors::red);
    }

    String errorHint() const override {
        return "Ensure the function is declared before calling it.";
    }
};

// Raised when a native function encounters an issue (e.g., incorrect parameters).
class NativeFunctionError : public EvaluationError {
public:
    explicit NativeFunctionError(const String& functionName, const String& reason)
        : EvaluationError("Native function '" + functionName + "' encountered an error: " + reason) {}

    String errorPrefix() const override {
        return highlight("Native Function Error: ", Colors::orange);
    }

    String errorHint() const override {
        return "Check if the function is being used correctly with the expected arguments.";
    }
};

// Raised when a user-defined function execution fails.
class UserFunctionError : public EvaluationError {
public:
    explicit UserFunctionError(const String& functionName, const String& reason)
        : EvaluationError("User function '" + functionName + "' encountered an error: " + reason) {}

    String errorPrefix() const override {
        return highlight("User Function Error: ", Colors::yellow);
    }

    String errorHint() const override {
        return "Check the function implementation for logical errors.";
    }
};

// Raised when a function returns an invalid type.
class ReturnTypeError : public EvaluationError {
public:
    explicit ReturnTypeError(const String& functionName, const String& expected, const String& actual)
        : EvaluationError("Function '" + functionName + "' expected return type '" + expected +
                          "', but returned '" + actual + "'.") {}

    String errorPrefix() const override {
        return highlight("Return Type Error: ", Colors::purple);
    }

    String errorHint() const override {
        return "Ensure the function returns the correct data type.";
    }
};

// Raised when a function is redefined within the same scope.
class FunctionRedefinitionError : public EvaluationError {
public:
    explicit FunctionRedefinitionError(const String& functionName)
        : EvaluationError("Function '" + functionName + "' is already defined in this scope.") {}

    String errorPrefix() const override {
        return highlight("Function Redefinition Error: ", Colors::blue);
    }

    String errorHint() const override {
        return "Use a different function name or check for unintended redefinitions.";
    }
};

// Raised when a function is improperly called (e.g., missing parentheses).
class InvalidFunctionCallError : public EvaluationError {
public:
    explicit InvalidFunctionCallError(const String& functionName)
        : EvaluationError("Invalid function call syntax for '" + functionName + "'.") {}

    String errorPrefix() const override {
        return highlight("Invalid Function Call Error: ", Colors::pink);
    }

    String errorHint() const override {
        return "Ensure the function is called with the correct syntax.";
    }
};





#endif // ERRORS_H
