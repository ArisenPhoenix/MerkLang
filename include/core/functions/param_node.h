#ifndef PARAM_NODE_H
#define PARAM_NODE_H

#include "core/types.h"
#include "core/node.h"
#include <string>


const bool debugParam = true;


// ParamNode inherits from VarNode
class ParamNode : public VarNode {
private:
    String paramName;              // The name of the parameter
    VariantType defaultValue;      // Default value (if any)
    bool isVarArgs = false;        // Track if this is a varargs parameter
    bool hasDefaultValue = false;  // True if the parameter has a default value

public:
    ~ParamNode() = default;

    // Constructor for a parameter without a default value
    ParamNode(const String& paramName, NodeValueType expectedType, bool isConst = false, bool isStatic = false);

    // Constructor for a parameter with a default value
    ParamNode(const String& paramName, const VariantType& defaultValue, bool isConst = false, bool isStatic = false);

    // All Constructor
    ParamNode(const String& paramName, const VariantType& defaultValue, NodeValueType expectedType, bool isConst = false, bool isStatic = false, bool isMutable = true);

    // Constructor for a parameter without a name (for positional args)
    ParamNode(NodeValueType expectedType, bool isConst = false, bool isStatic = false);

    // Explicit Copy Constructor
    ParamNode(const ParamNode& other)
        : VarNode(other),  // Call VarNode's copy constructor
          paramName(other.paramName),
          defaultValue(other.defaultValue),
          isVarArgs(other.isVarArgs),
          hasDefaultValue(other.hasDefaultValue) {}

    // Explicit Move Constructor (optional, for efficiency)
    ParamNode(ParamNode&& other) noexcept
        : VarNode(std::move(other)),
          paramName(std::move(other.paramName)),
          defaultValue(std::move(other.defaultValue)),
          isVarArgs(other.isVarArgs),
          hasDefaultValue(other.hasDefaultValue) {}

    // Explicit Copy Assignment Operator
    ParamNode& operator=(const ParamNode& other) {
        if (this != &other) {
            VarNode::operator=(other);  // Copy base class
            paramName = other.paramName;
            defaultValue = other.defaultValue;
            isVarArgs = other.isVarArgs;
            hasDefaultValue = other.hasDefaultValue;
        }
        return *this;
    }

    // Explicit Move Assignment Operator (optional)
    ParamNode& operator=(ParamNode&& other) noexcept {
        if (this != &other) {
            VarNode::operator=(std::move(other));  // Move base class
            paramName = std::move(other.paramName);
            defaultValue = std::move(other.defaultValue);
            isVarArgs = other.isVarArgs;
            hasDefaultValue = other.hasDefaultValue;
        }
        return *this;
    }

    // Check if this is a varargs parameter
    bool isVarArgsParameter() const { return isVarArgs; }  

    // Check if this is a positional argument
    bool isPositional() const { return paramName.empty(); }

    // Override setValue for VariantType with proper type checks
    void setValue(VariantType newValue);

    // Override setValue for Node with proper type checks
    void setValue(Node newValue);

    // Retrieve default value safely
    VariantType getDefaultValue() const;

    // Get parameter name
    String getName() const { return paramName; }

    // Check if parameter has a default value
    bool hasDefault() const { return hasDefaultValue; }

    // Debugging
    String toString() const;

    ParamNode copy() const;



private:
    void validateExpectedType(NodeValueType expectedType);
};


class ParamList {
private:
    Vector<ParamNode> parameters; // Stores function parameters

public:
    // Default Constructor
    ParamList() = default;

    // Add a parameter to the list
    void addParameter(const ParamNode& param);

    // Verify arguments passed to function
    void verifyArguments(Vector<Node> args);

    // Get parameter count
    size_t size() const { return parameters.size(); }

    // Access parameters by index
    const ParamNode& operator[](size_t index) const;

    // Retrieve parameters by name
    const ParamNode* getByName(const String& name) const;

    Vector<String> getNames() const;

    String toString() const;

    Vector<NodeValueType> getParameterTypes();
    void printAST(std::ostream& os, int indent = 0) const;
    void resetToDefaults();
    ParamList clone();
    ParamList clone() const;

    auto begin() { return parameters.begin(); }
    auto end() { return parameters.end(); }
    auto begin() const { return parameters.begin(); }
    auto end() const { return parameters.end(); }

    // Add cbegin() and cend() for const iteration, c++ can be a pain
    auto cbegin() const { return parameters.cbegin(); }
    auto cend() const { return parameters.cend(); }
};


#endif // PARAM_NODE_H
