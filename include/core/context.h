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

    UniquePtr<Context> clone() const;

    void clear();

    // Get a const reference to all variables in the current context
    const std::unordered_map<String, UniquePtr<VarNode>>& getVariables() const;


private:
    std::unordered_map<String, UniquePtr<VarNode>> deepCopyVariables() const;
    std::unordered_map<String, UniquePtr<VarNode>> variables; // Variables for the current context
    Vector<VarNode> arguments;                                // Function arguments
};




#endif // CONTEXT_H



