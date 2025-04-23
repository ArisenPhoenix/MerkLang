#ifndef ERRORS_H
#define ERRORS_H

#include <stdexcept>
#include <string>
#include <sstream>
#include "types.h"
#include "core/functions/param_node.h"
// class ParamList;


// bool isWhitespaceOnly(const String& str);

// Base Error Class: MerkError
class MerkError : public RunTimeError {
protected:
    String message;
    mutable String errorStringCache; // Cache for detailed message

public:
    MerkError(const String& message);

    virtual String errorString() const;

    const char* what() const noexcept override;

    virtual ~MerkError() = default;
    bool isWhitespaceOnly(const String& str) const;
};


// =============================================== Tokenizer =================================================
// TokenizationError Class
class TokenizationError : public MerkError {
protected:
    String context;
    int line;
    int column;

public:
    TokenizationError(const String& message, int line = -1, int column = -1, const String& context = "");

    String errorString() const override;

};

class UnknownTokenError : public TokenizationError {
    String token;

public:
    UnknownTokenError(const String& token, int line, int column, const String& context = "");

    String errorString() const override;
};

class IndentationError : public TokenizationError {
public:
    IndentationError(int line, int column, const String& context = "");

    String errorString() const override;
};

class OutOfBoundsError : public TokenizationError {
    String context;

public:
    OutOfBoundsError(int line, int column, const String& context = "");

    String errorString() const override;
};

class UnmatchedQuoteError : public TokenizationError {
    String context;

public:
    UnmatchedQuoteError(int line, int column, const String& context = "");

    String errorString() const override;
};



// =============================================== Parser =================================================

// Base ParserError Class
class ParserError : public MerkError {
protected:
    Token token; // The token causing the error
    String context;

public:
    ParserError(const String& message, const Token& token, const String& context = "");

    virtual String errorPrefix() const;

    virtual String errorHint() const;

    String errorString() const override;
};

// Specialized Parser Errors

class SyntaxError : public ParserError {
public:
    SyntaxError(const String& message, const Token& token, const String& context = "");

    String errorPrefix() const override;
};

class MissingTokenError : public ParserError {
public:
    MissingTokenError(const Token& token, const String& context = "");

    String errorPrefix() const override;

    String errorHint() const override;
};

class UnexpectedTokenError : public ParserError {
    String expected;

public:
    UnexpectedTokenError(const Token& token, const String& expected, const String& context = "");

    String errorPrefix() const override;

    String errorHint() const override;
};

class UnclosedBlockError : public ParserError {
public:
    UnclosedBlockError(const Token& token);

    String errorPrefix() const override;

    String errorHint() const override;
};

class InvalidIndentationError : public ParserError {
public:
    InvalidIndentationError(const Token& token);

    String errorPrefix() const override;

    String errorHint() const override;
};













// =============================================== Evaluator =================================================

class EvaluationError : public MerkError {
protected:
    String context; // Optional context for evaluation errors

public:
    EvaluationError(const String& message, const String& context = "");

    virtual String errorPrefix() const;

    virtual String errorHint() const;

    String errorString() const override;
};


class UndefinedVariableError : public EvaluationError {
public:
    UndefinedVariableError(const String& variableName, const String& context = "");

    String errorPrefix() const override;

    String errorHint() const override;
};


class TypeMismatchError : public EvaluationError {
public:
    TypeMismatchError(const String& expected, const String& actual, const String& context = "");

    String errorPrefix() const override;

    String errorHint() const override;
};


class InvalidOperationError : public EvaluationError {
public:
    InvalidOperationError(const String& operation, const String& context = "");

    String errorPrefix() const override;

    String errorHint() const override;
};


class BlockEvaluationError : public EvaluationError {
public:
    BlockEvaluationError(const String& message, const String& context = "");

    String errorPrefix() const override;

    String errorHint() const override;
};


class BreakOutsideLoopError : public EvaluationError {
public:
    BreakOutsideLoopError(const String& context = "");

    String errorPrefix() const override;

    String errorHint() const override;
};


class UnreachableCodeError : public EvaluationError {
public:
    UnreachableCodeError(const String& context = "");

    String errorPrefix() const override;

    String errorHint() const override;
};









// =============================================== Context =================================================

class ContextError : public MerkError {
public:
    explicit ContextError(const String& message);

    virtual String errorPrefix() const;

    String errorString() const override;
    virtual String errorHint() const;
};

class InvalidVariableUpdateError : public ContextError {
public:
    explicit InvalidVariableUpdateError(const String& variableName);

    String errorPrefix() const override;

    String errorHint() const override;
};


class NullVariableError : public ContextError {
public:
    explicit NullVariableError(const String& variableName);

    String errorPrefix() const override;

    String errorHint() const override;
};


class VariableAlreadyDeclaredError : public ContextError {
public:
    explicit VariableAlreadyDeclaredError(const String& variableName);

    String errorPrefix() const override;

    String errorHint() const override;
};



class ImmutableVariableError : public ContextError {
public:
    explicit ImmutableVariableError(const String& variableName);

    String errorPrefix() const override;

    String errorHint() const override;
};








// =============================================== Scope =================================================
class ScopeError : public MerkError {
public:
    explicit ScopeError(const String& message);

    virtual String errorPrefix() const;

    String errorString() const override;

    virtual String errorHint() const;
};

class ParentScopeNotFoundError : public ScopeError {
public:
    explicit ParentScopeNotFoundError();

    String errorPrefix() const override;

    String errorHint() const override;
};


class VariableNotFoundError : public ScopeError {
public:
    explicit VariableNotFoundError(const String& variableName);

    String errorPrefix() const override;

    String errorHint() const override;
};




class ImmutableVariableUpdateError : public ScopeError {
public:
    explicit ImmutableVariableUpdateError(const String& variableName);

    String errorPrefix() const override;

    String errorHint() const override;
};


class ChildScopeCreationError : public ScopeError {
public:
    explicit ChildScopeCreationError();

    String errorPrefix() const override;

    String errorHint() const override;
};




// ================================= Functions ==========================================
class ArgumentError : public EvaluationError {
private:
    String functionName;
    size_t expectedSize;
    size_t receivedSize;
    ParamList expectedParams;
    Vector<Node> receivedArguments;

public:
    ArgumentError(const String& functionName, size_t expected, size_t received);
    ArgumentError(const String& functionName, ParamList parameters, Vector<Node> arguments);

    String errorPrefix() const override;

    String errorHint() const override;

    String errorString() const override;
};

















// ================================= Functions ==========================================

// Raised when calling a function that does not exist in the scope.
class FunctionNotFoundError : public EvaluationError {
public:
    explicit FunctionNotFoundError(const String& functionName);

    String errorPrefix() const override;

    String errorHint() const override;
};

// Raised when a native function encounters an issue (e.g., incorrect parameters).
class NativeFunctionError : public EvaluationError {
public:
    explicit NativeFunctionError(const String& functionName, const String& reason);

    String errorPrefix() const override;

    String errorHint() const override;
};

// Raised when a user-defined function execution fails.
class UserFunctionError : public EvaluationError {
public:
    explicit UserFunctionError(const String& functionName, const String& reason);

    String errorPrefix() const override;

    String errorHint() const override;
};

// Raised when a function returns an invalid type.
class ReturnTypeError : public EvaluationError {
public:
    explicit ReturnTypeError(const String& functionName, const String& expected, const String& actual);

    String errorPrefix() const override;

    String errorHint() const override;
};

// Raised when a function is redefined within the same scope.
class FunctionRedefinitionError : public EvaluationError {
public:
    explicit FunctionRedefinitionError(const String& functionName);

    String errorPrefix() const override;

    String errorHint() const override;
};

// Raised when a function is improperly called (e.g., missing parentheses).
class InvalidFunctionCallError : public EvaluationError {
public:
    explicit InvalidFunctionCallError(const String& functionName);

    String errorPrefix() const override;

    String errorHint() const override;
};





#endif // ERRORS_H
