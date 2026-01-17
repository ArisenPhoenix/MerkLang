// #ifndef PARAM_NODE_H
// #define PARAM_NODE_H


// #include "core/node/Node.hpp"
// // #include "core/types.h"
// #include "core/TypesFWD.hpp"


// class ArgumentList;



// // ParamNode inherits from VarNode
// class ParamNode {
// private:    
//     VariantType defaultValue = UninitializedType();      // Default value (if any)
//     bool isVarArgs = false;        // Track if this is a varargs parameter
//     bool hasDefaultValue = false;  // True if the parameter has a default value
    

// public:
//     DataTypeFlags flags;
//     ~ParamNode();
//     ParamNode();

//     ParamNode(DataTypeFlags flags, VariantType defaultVal = UninitializedType());
//     ParamNode(String name, NodeValueType type, bool isVarArgs = false);

//     // // Explicit Copy Constructor
//     ParamNode(const ParamNode& other);

//     // // Explicit Move Constructor (optional, for efficiency)
//     ParamNode(ParamNode&& other) noexcept;

//     // Explicit Copy Assignment Operator
//     ParamNode& operator=(const ParamNode& other);


//     // Check if this is a varargs parameter
//     bool isVarArgsParameter() const;
//     void setIsVarArgsParam(bool);

//     // Check if this is a positional argument
//     bool isPositional() const;

//     // Override setValue for VariantType with proper type checks
//     void setValue(VariantType newValue);

//     // Override setValue for Node with proper type checks
//     void setValue(Node newValue);

//     void setType(NodeValueType);

//     // Retrieve default value safely
//     VariantType getDefaultValue() const;

//     // Get parameter name
//     String getName() const;

//     // Check if parameter has a default value
//     bool hasDefault() const;

//     // Debugging
//     String toString() const;
//     String toShortString() const; 
//     ParamNode copy() const;
//     size_t hash() const;




// private:
//     void validateExpectedType(NodeValueType expectedType);
// };


// class ParamList {
// private:
//     Vector<ParamNode> parameters; // Stores function parameters

// public:
//     // Default Constructor
//     ParamList() = default;
//     ~ParamList();
//     // Add a parameter to the list
//     void addParameter(const ParamNode& param);

//     // Verify arguments passed to function
//     void verifyArguments(Vector<Node> args);
//     void verifyArguments(ArgumentList& args);

//     // Get parameter count
//     size_t size() const { return parameters.size(); }
//     ParamNode& back();
//     ParamNode back() const;

//     // Access parameters by index
//     const ParamNode& operator[](size_t index) const;

//     // Retrieve parameters by name
//     const ParamNode* getByName(const String& name) const;

//     Vector<String> getNames() const;

//     String toString() const;
//     String toShortString() const; 
//     void erase(size_t index);

//     // Optional: Remove parameter by name
//     bool eraseByName(const String& name);

//     Vector<NodeValueType> getParameterTypes();
//     void printAST(std::ostream& os, int indent = 0) const;
//     void resetToDefaults();
//     ParamList clone();
//     ParamList clone() const;
//     bool empty() const;


//     auto begin() { return parameters.begin(); }
//     auto end() { return parameters.end(); }
//     auto begin() const { return parameters.begin(); }
//     auto end() const { return parameters.end(); }

//     // Add cbegin() and cend() for const iteration, c++ can be a pain
//     auto cbegin() const { return parameters.cbegin(); }
//     auto cend() const { return parameters.cend(); }
// };


// #endif // PARAM_NODE_H





#pragma once

#include <optional>
#include "core/types.h"
#include "core/types/Type.hpp"   // TypeSignatureId
#include "core/node/Node.hpp"

class ArgumentList;
class Scope;
class TypeRegistry;

class ParamNode {
public:
    DataTypeFlags flags;                 // keep existing
    VariantType defaultValue = UninitializedType();

    bool isVarArgs = false;
    bool hasDefaultValue = false;

    // NEW: typed annotation state (Option C)
    bool hasAnnotation = false;          // “was :Type written?”
    TypeSignatureId typeSig = 0;         // bound later, 0 = unbound

public:
    ParamNode() = default;
    ~ParamNode() = default;

    ParamNode(DataTypeFlags flags, VariantType defaultVal = UninitializedType());

    // legacy ctor still OK; treat type as legacy
    ParamNode(String name, NodeValueType type, bool isVarArgs = false);

    // Core accessors
    const String& getNameRef() const { return flags.name; }
    String getName() const { return flags.name; }
    bool isVarArgsParameter() const { return isVarArgs; }
    void setIsVarArgsParam(bool v) { isVarArgs = v; }

    bool hasDefault() const { return hasDefaultValue; }
    VariantType getDefaultValue() const;

    // Annotation API (new)
    void setAnnotation(const ResolvedType& rt);          // called by parser
    bool isTyped() const { return hasAnnotation; }
    TypeSignatureId getTypeSig() const { return typeSig; }

    // Bind phase: resolve annotation -> typeSig
    void bindType(TypeRegistry& reg, Scope& scope);

    // Debug
    String toString() const;
    String toShortString() const;
    size_t hash() const;
    ParamNode copy() const;

private:
    void updateDefaultFlags();
};

// ------------------------
// ParamList
// ------------------------
class ParamList {
private:
    Vector<ParamNode> parameters;

public:
    ParamList() = default;
    ~ParamList() = default;

    void addParameter(const ParamNode& p) { parameters.push_back(p); }

    size_t size() const { return parameters.size(); }
    bool empty() const { return parameters.empty(); }
    ParamNode& back() { return parameters.back(); }
    ParamNode back() const { return parameters.back(); }

    const ParamNode& operator[](size_t index) const;

    const ParamNode* getByName(const String& name) const;
    Vector<String> getNames() const;

    // NEW: bind all param annotations
    void bindTypes(TypeRegistry& reg, Scope& scope);

    // NEW: call-time binding (does not mutate ParamList)
    Vector<Node> bindArguments(const Vector<Node>& args) const;
    Vector<Node> bindArguments(ArgumentList& args) const;

    // Keep old API but re-route it (optional)
    void verifyArguments(Vector<Node> args);        // wrapper
    void verifyArguments(ArgumentList& args);       // wrapper

    // Helpers
    Vector<NodeValueType> getParameterTypes();
    String toString() const;
    String toShortString() const;

    ParamList clone();
    ParamList clone() const;

    auto begin() { return parameters.begin(); }
    auto end() { return parameters.end(); }
    auto begin() const { return parameters.begin(); }
    auto end() const { return parameters.end(); }
    auto cbegin() const { return parameters.cbegin(); }
    auto cend() const { return parameters.cend(); }

    void erase(size_t index);
    bool eraseByName(const String& name);

    void printAST(std::ostream& os, int indent = 0) const;
};

