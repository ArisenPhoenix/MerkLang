#include "core/types.h"
#include "core/errors.h"



EvaluationError::EvaluationError(const String& message, const String& context)
    : MerkError(message), context(context) {}

String EvaluationError::errorPrefix() const {
    return highlight("Evaluation Error: ", Colors::blue);
}

String EvaluationError::errorHint() const {
    return ""; // No additional hints by default
}

String EvaluationError::errorString() const {
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

    
    

UndefinedVariableError::UndefinedVariableError(const String& variableName, const String& context)
    : EvaluationError("Undefined variable: '" + variableName + "' was not declared.", context) {}

String UndefinedVariableError::errorPrefix() const {
    return highlight("Undefined Variable Error: ", Colors::red);
}

String UndefinedVariableError::errorHint() const {
    return "Declare the variable before using it.";
}

    
    

TypeMismatchError::TypeMismatchError(const String& expected, const String& actual, const String& context)
        : EvaluationError("Type mismatch: expected '" + expected + "', but found '" + actual + "'.", context) {}

String TypeMismatchError::errorPrefix() const {
    return highlight("Type Mismatch Error: ", Colors::purple);
}

String TypeMismatchError::errorHint() const {
    return "Ensure that the operation involves compatible types.";
}




InvalidOperationError::InvalidOperationError(const String& operation, const String& context)
    : EvaluationError("Invalid operation: '" + operation + "' cannot be performed.", context) {}

String InvalidOperationError::errorPrefix() const {
    return highlight("Invalid Operation Error: ", Colors::orange);
}

String InvalidOperationError::errorHint() const {
    return "Check the operator and operands for correctness.";
}




BlockEvaluationError::BlockEvaluationError(const String& message, const String& context)
    : EvaluationError("Error while evaluating a block: " + message, context) {}

String BlockEvaluationError::errorPrefix() const {
    return highlight("Block Evaluation Error: ", Colors::green);
}

String BlockEvaluationError::errorHint() const {
    return "Ensure that all nodes in the block are valid and executable.";
}




BreakOutsideLoopError::BreakOutsideLoopError(const String& context)
    : EvaluationError("Break statement encountered outside of a loop.", context) {}

String BreakOutsideLoopError::errorPrefix() const {
    return highlight("Break Outside Loop Error: ", Colors::red);
}

String BreakOutsideLoopError::errorHint() const {
    return "Break statements can only be used within loops.";
}



UnreachableCodeError::UnreachableCodeError(const String& context)
    : EvaluationError("Unreachable code was executed. This may indicate a bug.", context) {}

String UnreachableCodeError::errorPrefix() const {
    return highlight("Unreachable Code Error: ", Colors::red);
}

String UnreachableCodeError::errorHint() const {
    return "Review the AST structure and evaluation logic.";
}
