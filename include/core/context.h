#ifndef CONTEXT_H
#define CONTEXT_H

#include <unordered_map>
#include <memory>
#include <vector>
#include <optional>
#include "core/types.h"
class VarNode;


// Forward declare FunctionDeclarationNode
class FunctionDeclarationNode; 

class Context {
public:
    // Constructors
    Context() = default;

    // Disable copy constructor and copy assignment
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    // Enable move constructor and move assignment (optional)
    Context(Context&&) = default;
    Context& operator=(Context&&) = default;


    // Variable management
    void setVariable(const String& name, UniquePtr<VarNode> value);
    void updateVariable(const String& name, const Node& newValue);

    std::optional<std::reference_wrapper<VarNode>> getVariable(const String& name);

    // VarNode& getVariable(const String& name);
    bool hasVariable(const String& name) const;
    void debugPrint() const;

    UniquePtr<Context> clone() const {
        auto newContext = std::make_unique<Context>();
        // Deep copy logic for all members of Context
        newContext->variables = deepCopyVariables();
        newContext->arguments = arguments;  // Assuming this is copyable
        return newContext;
}

    void clear();

    // Get a const reference to all variables in the current context
    const std::unordered_map<String, UniquePtr<VarNode>>& getVariables() const;


private:
    std::unordered_map<String, UniquePtr<VarNode>> deepCopyVariables() const {
        std::unordered_map<String, UniquePtr<VarNode>> copy;
        for (const auto& [key, value] : variables) {
            if (value) {
                copy[key] = std::make_unique<VarNode>(*value);  // Deep copy each VarNode
            }
        }
        return copy;
    }
    std::unordered_map<String, UniquePtr<VarNode>> variables; // Variables for the current context
    Vector<VarNode> arguments;                                // Function arguments
};




#endif // CONTEXT_H



