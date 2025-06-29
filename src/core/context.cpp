
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



void Context::setVariable(const String& name, UniquePtr<VarNode> value) {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
             
    auto it = variables.find(name);
    if (it != variables.end()) {
        if (!it->second->isMutable) {
            throw ImmutableVariableError(name);
        }
    }

    variables[name] = std::move(value);
    DEBUG_FLOW_EXIT();

}

void Context::updateVariable(const String& name, const Node& newValue) {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    auto it = variables.find(name);
    if (it == variables.end()) {
        DEBUG_LOG(LogLevel::ERROR, "From Context");
        throw VariableNotFoundError(name);
    }
    VarNode& currentVar = *(it->second);

    currentVar.setValue(newValue);

    DEBUG_FLOW_EXIT();

}

std::optional<std::reference_wrapper<VarNode>> Context::getVariable(const String& name) {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    auto it = variables.find(name);

    if (it != variables.end()) {
        assert(it->second != nullptr && "Context::getVariable found a null pointer!");
        DEBUG_FLOW_EXIT();

        return *(it->second);
    }
    DEBUG_FLOW_EXIT();

    return std::nullopt; 
}

UniquePtr<Context> Context::clone() const {
    auto newContext = std::make_unique<Context>();
    newContext->variables = deepCopyVariables();
    newContext->arguments = arguments; 
    return newContext;
}

std::unordered_map<String, UniquePtr<VarNode>> Context::deepCopyVariables() const {
    std::unordered_map<String, UniquePtr<VarNode>> copy;
    for (const auto& [key, value] : variables) {
        if (value) {
            copy[key] = std::make_unique<VarNode>(*value);  // Deep copy each VarNode
        }
    }
    return copy;
}

const std::unordered_map<String, UniquePtr<VarNode>>& Context::getVariables() const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    DEBUG_FLOW_EXIT();
    return variables;
}

bool Context::hasVariable(const String& name) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    auto hasVariable = variables.find(name) != variables.end();
    DEBUG_FLOW_EXIT();
    return hasVariable;
    
}

// Print all variables in the current context for debugging
void Context::debugPrint() const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    for (const auto& [name, value] : variables) {
        if (value) {
            debugLog(true, "      ", name, " = ", *value); // Dereference pointer
        } else {
            debugLog(true, "      ", name, " = ", "[null]");
        }
    }
    DEBUG_FLOW_EXIT();
}

void Context::clear() {
    for (auto& [varName, var] : variables) {
        var.reset();
    }
    variables.clear();
}





