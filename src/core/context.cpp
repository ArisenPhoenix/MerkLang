
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <cassert>

#include "core/types.h"
#include "core/errors.h"
#include "core/node.h"
#include "core/context.h"

#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"
#include "core/evaluator.h"
#include "core/registry/registry.h"


void Context::setVariable(const String& name, UniquePtr<VarNode> value) {
    DEBUG_FLOW(FlowLevel::LOW);
    DEBUG_LOG(LogLevel::TRACE, "[Context] Setting variable: ", name, 
             " | isConst: ", value->isConst, 
             " | isMutable: ", value->isMutable, 
             " | isStatic: ", value->isStatic);
             
    auto it = variables.find(name);
    if (it != variables.end()) {
        // Enforce reassignment rules
        if (!it->second->isMutable) {
            throw ImmutableVariableError(name);
        }
    }

    // Store or update the variable
    DEBUG_LOG(LogLevel::TRACE, "Setting variable: ", name);
    variables[name] = std::move(value);
    DEBUG_FLOW_EXIT();

}

void Context::updateVariable(const String& name, const Node& newValue) {
    DEBUG_FLOW(FlowLevel::MED);
    DEBUG_LOG(LogLevel::TRACE, "[Context] Updating variable: ", name, 
             " | isConst: ", newValue.isConst, 
             " | isMutable: ", newValue.isMutable, 
             " | isStatic: ", newValue.isStatic);

    auto it = variables.find(name);
    if (it == variables.end()) {
        throw VariableNotFoundError(name);
    }
    VarNode& currentVar = *(it->second);

    // Move the new value into the existing variable, Nodes are meant to handle their own data, imuutability etc.
    currentVar.setValue(newValue);

    DEBUG_LOG(LogLevel::INFO, "[Context] Updating variable: ", name, 
        " | Old Value: ", *(it->second), 
        " | New Value: ", newValue);
    
    DEBUG_FLOW_EXIT();

}

std::optional<std::reference_wrapper<VarNode>> Context::getVariable(const String& name) {
    DEBUG_FLOW(FlowLevel::LOW);
    auto it = variables.find(name);

    if (it != variables.end()) {
        assert(it->second != nullptr && "Context::getVariable found a null pointer!");
        DEBUG_FLOW_EXIT();

        return *(it->second); // Wrap in optional and return reference
    }
    DEBUG_FLOW_EXIT();

    return std::nullopt; // Indicate variable not found
}

const std::unordered_map<String, UniquePtr<VarNode>>& Context::getVariables() const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    DEBUG_FLOW_EXIT();
    return variables;
}

// Check if a variable exists in the current context or parent contexts
bool Context::hasVariable(const String& name) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    auto hasVariable = variables.find(name) != variables.end();
    DEBUG_FLOW_EXIT();
    return hasVariable;
    
}

// Print all variables in the current context for debugging
void Context::debugPrint() const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    debugLog(true, "Context Variables:");
    for (const auto& [name, value] : variables) {
        if (value) {
            debugLog(true, "      ", name, " = ", *value); // Dereference pointer
        } else {
            debugLog(true, "      ", name, " = ", "[null]");
        }
    }
    DEBUG_FLOW_EXIT();
}





