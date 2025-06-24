#include "core/types.h"
#include "core/errors.h"

ContextError::ContextError(const String& message)
    : MerkError(message) {}

String ContextError::errorPrefix() const {
    return highlight("Context Error: ", Colors::blue);
}

String ContextError::errorString() const {
    if (errorStringCache.empty()) { // Use the cached message
        std::ostringstream oss;
        oss << errorPrefix() << message;
        errorStringCache = oss.str(); // Cache the detailed message
    }
    return errorStringCache;
}
String ContextError::errorHint() const {
    return "";
}


InvalidVariableUpdateError::InvalidVariableUpdateError(const String& variableName)
    : ContextError("Invalid update for variable '" + variableName + "'. The new value is not compatible.") {}

String InvalidVariableUpdateError::errorPrefix() const {
    return highlight("Invalid Variable Update Error: ", Colors::orange);
}

String InvalidVariableUpdateError::errorHint() const {
    return "Check the type or value of the new assignment.";
}

    
    

NullVariableError::NullVariableError(const String& variableName)
    : ContextError("Variable '" + variableName + "' is null and cannot be accessed or assigned.") {}

String NullVariableError::errorPrefix() const {
    return highlight("Null Variable Error: ", Colors::pink);
}

String NullVariableError::errorHint() const {
    return "Ensure the variable is initialized with a valid value.";
}



VariableAlreadyDeclaredError::VariableAlreadyDeclaredError(const String& variableName)
    : ContextError("Variable '" + variableName + "' has already been declared in the current context.") {}

String VariableAlreadyDeclaredError::errorPrefix() const {
    return highlight("Variable Already Declared Error: ", Colors::yellow);
}

String VariableAlreadyDeclaredError::errorHint() const {
    return "Consider using a different name or updating the existing variable if allowed.";
}




ImmutableVariableError::ImmutableVariableError(const String& variableName)
    : ContextError("Variable '" + variableName + "' is immutable and cannot be reassigned or updated.") {}

String ImmutableVariableError::errorPrefix() const {
    return highlight("Immutable Variable Error: ", Colors::purple);
}

String ImmutableVariableError::errorHint() const {
    return "Ensure the variable is declared as mutable if reassignment is required.";
}
