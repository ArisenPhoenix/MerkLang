#include "core/types.h"
#include "core/errors.h"


ScopeError::ScopeError(const String& message)
    : MerkError(message) {}

String ScopeError::errorPrefix() const {
    return highlight("Scope Error: ", Colors::blue);
}

String ScopeError::errorString() const {
    if (errorStringCache.empty()) { // Use the cached message
        std::ostringstream oss;
        oss << errorPrefix() << message;
        errorStringCache = oss.str(); // Cache the detailed message
    }
    return errorStringCache;
}

String ScopeError::errorHint() const {
    return "";
}

ParentScopeNotFoundError::ParentScopeNotFoundError()
    : ScopeError("Parent scope not found. This operation is invalid for the root scope.") {}

String ParentScopeNotFoundError::errorPrefix() const {
    return highlight("Parent Scope Not Found Error: ", Colors::red);
}

String ParentScopeNotFoundError::errorHint() const {
    return "Ensure this operation is only performed within a non-root scope.";
}

    
    
VariableNotFoundError::VariableNotFoundError(const String& variableName)
    : ScopeError("Variable '" + variableName + "' not found in any accessible scope.") {}

String VariableNotFoundError::errorPrefix() const {
    return highlight("Variable Not Found Error: ", Colors::red);
}

String VariableNotFoundError::errorHint() const {
    return "Ensure the variable is declared before accessing or updating it.";
}

    
    
    

ImmutableVariableUpdateError::ImmutableVariableUpdateError(const String& variableName)
    : ScopeError("Variable '" + variableName + "' is immutable and cannot be updated.") {}

String ImmutableVariableUpdateError::errorPrefix() const {
    return highlight("Immutable Variable Update Error: ", Colors::purple);
}

String ImmutableVariableUpdateError::errorHint() const {
    return "Declare the variable as mutable if updates are required.";
}

    

ChildScopeCreationError::ChildScopeCreationError()
    : ScopeError("Failed to create a child scope. Ensure sufficient memory and a valid parent scope.") {}

String ChildScopeCreationError::errorPrefix() const {
    return highlight("Child Scope Creation Error: ", Colors::orange);
}

String ChildScopeCreationError::errorHint() const {
    return "Check for memory constraints or invalid parent scope configurations.";
}
