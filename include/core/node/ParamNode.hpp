#ifndef PARAM_NODE_H
#define PARAM_NODE_H


#include "core/node/Node.hpp"
#include "core/types.h"

class ArgumentList;



// ParamNode inherits from VarNode
class ParamNode {
private:    
    VariantType defaultValue = UninitializedType();      // Default value (if any)
    bool isVarArgs = false;        // Track if this is a varargs parameter
    bool hasDefaultValue = false;  // True if the parameter has a default value
    

public:
    DataTypeFlags flags;
    ~ParamNode();
    ParamNode();

    ParamNode(DataTypeFlags flags, VariantType defaultVal = UninitializedType());
    ParamNode(String name, NodeValueType type, bool isVarArgs = false);

    // // Explicit Copy Constructor
    ParamNode(const ParamNode& other);

    // // Explicit Move Constructor (optional, for efficiency)
    ParamNode(ParamNode&& other) noexcept;

    // Explicit Copy Assignment Operator
    ParamNode& operator=(const ParamNode& other);


    // Check if this is a varargs parameter
    bool isVarArgsParameter() const;
    void setIsVarArgsParam(bool);

    // Check if this is a positional argument
    bool isPositional() const;

    // Override setValue for VariantType with proper type checks
    void setValue(VariantType newValue);

    // Override setValue for Node with proper type checks
    void setValue(Node newValue);

    void setType(NodeValueType);

    // Retrieve default value safely
    VariantType getDefaultValue() const;

    // Get parameter name
    String getName() const;

    // Check if parameter has a default value
    bool hasDefault() const;

    // Debugging
    String toString() const;
    String toShortString() const; 
    ParamNode copy() const;
    size_t hash() const;




private:
    void validateExpectedType(NodeValueType expectedType);
};


class ParamList {
private:
    Vector<ParamNode> parameters; // Stores function parameters

public:
    // Default Constructor
    ParamList() = default;
    ~ParamList();
    // Add a parameter to the list
    void addParameter(const ParamNode& param);

    // Verify arguments passed to function
    void verifyArguments(Vector<Node> args);
    void verifyArguments(ArgumentList& args);

    // Get parameter count
    size_t size() const { return parameters.size(); }
    ParamNode& back();
    ParamNode back() const;

    // Access parameters by index
    const ParamNode& operator[](size_t index) const;

    // Retrieve parameters by name
    const ParamNode* getByName(const String& name) const;

    Vector<String> getNames() const;

    String toString() const;
    String toShortString() const; 
    void erase(size_t index);

    // Optional: Remove parameter by name
    bool eraseByName(const String& name);

    Vector<NodeValueType> getParameterTypes();
    void printAST(std::ostream& os, int indent = 0) const;
    void resetToDefaults();
    ParamList clone();
    ParamList clone() const;
    bool empty() const;


    auto begin() { return parameters.begin(); }
    auto end() { return parameters.end(); }
    auto begin() const { return parameters.begin(); }
    auto end() const { return parameters.end(); }

    // Add cbegin() and cend() for const iteration, c++ can be a pain
    auto cbegin() const { return parameters.cbegin(); }
    auto cend() const { return parameters.cend(); }
};


#endif // PARAM_NODE_H
