#include "core/types.h"
#include "core/callables/param_node.h"
#include "core/errors.h"
#include "utilities/helper_functions.h"



// ================================= Functions ==========================================

ArgumentError::ArgumentError(const String& functionName, size_t expected, size_t received)
    : EvaluationError("Function '" + functionName + "' expects " + std::to_string(expected) +
                        " arguments, but " + std::to_string(received) + " were provided."),
        functionName(functionName), expectedSize(expected), receivedSize(received) {}

ArgumentError::ArgumentError(const String& functionName, ParamList parameters, Vector<Node> arguments): EvaluationError("Function '" + functionName + "' expects " + parameters.toShortString() + " got: " + joinVectorNodeStrings(arguments)){

}

String ArgumentError::errorPrefix() const {
    return highlight("Argument Error: ", Colors::red);
}

String ArgumentError::errorHint() const {
    return "Check the number of arguments passed to the function.";
}

String ArgumentError::errorString() const {
    std::ostringstream oss;
    oss << errorPrefix()
        << "Function '" << functionName << "' expects " << expectedSize
        << " arguments, but " << receivedSize << " were provided.";
    if (!errorHint().empty()) {
        oss << "\n" << highlight("Hint: ", Colors::green) << errorHint();
    }
    return oss.str();
}




// Raised when calling a function that does not exist in the scope.

FunctionNotFoundError::FunctionNotFoundError(const String& functionName)
    : EvaluationError("Function '" + functionName + "' is not defined in this scope.") {}

String FunctionNotFoundError::errorPrefix() const {
    return highlight("FunctionNotFoundError: ", Colors::red);
}

String FunctionNotFoundError::errorHint() const {
    return "Ensure the function is declared before calling it.";
}

    
    // Raised when a native function encounters an issue (e.g., incorrect parameters).
NativeFunctionError::NativeFunctionError(const String& functionName, const String& reason)
    : EvaluationError("Native function '" + functionName + "' encountered an error: " + reason) {}

String NativeFunctionError::errorPrefix() const {
    return highlight("NativeFunctionError: ", Colors::orange);
}

String NativeFunctionError::errorHint() const {
    return "Check if the function is being used correctly with the expected arguments.";
}

    
    // Raised when a user-defined function execution fails.
UserFunctionError::UserFunctionError(const String& functionName, const String& reason)
    : EvaluationError("User function '" + functionName + "' encountered an error: " + reason) {}

String UserFunctionError::errorPrefix() const {
    return highlight("UserFunctionError: ", Colors::yellow);
}

String UserFunctionError::errorHint() const {
    return "Check the function implementation for logical errors.";
}

    
    // Raised when a function returns an invalid type.
ReturnTypeError::ReturnTypeError(const String& functionName, const String& expected, const String& actual)
    : EvaluationError("Function '" + functionName + "' expected return type '" + expected +
                        "', but returned '" + actual + "'.") {}

String ReturnTypeError::errorPrefix() const {
    return highlight("ReturnTypeError: ", Colors::purple);
}

String ReturnTypeError::errorHint() const {
    return "Ensure the function returns the correct data type.";
}

    // Raised when a function is redefined within the same scope.
FunctionRedefinitionError::FunctionRedefinitionError(const String& functionName)
    : EvaluationError("Function '" + functionName + "' is already defined in this scope.") {}

String FunctionRedefinitionError::errorPrefix() const {
    return highlight("FunctionRedefinitionError: ", Colors::blue);
}

String FunctionRedefinitionError::errorHint() const {
    return "Use a different function name or check for unintended redefinitions.";
}
    
    // Raised when a function is improperly called (e.g., missing parentheses).

InvalidFunctionCallError::InvalidFunctionCallError(const String& functionName)
    : EvaluationError("Invalid function call syntax for '" + functionName + "'.") {}

String InvalidFunctionCallError::errorPrefix() const {
    return highlight("InvalidFunctionCallError: ", Colors::pink);
}

String InvalidFunctionCallError::errorHint() const {
    return "Ensure the function is called with the correct syntax.";
}