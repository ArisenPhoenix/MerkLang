#ifndef NODE_H
#define NODE_H

#include <string>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <variant>
#include "core/types.h"
#include "utilities/debugging_functions.h"

const bool debugNode = false;

// class VarNode; // Forward Declare VarNode


// Struct for NodeData using a union for efficient storage
struct NodeData {
    NodeValueType type = NodeValueType::Null; // Type of the value
    VariantType value = Null; // A std::variant to store any of the allowed types
    NodeData() = default; // Default constructor
    NodeData(const NodeData&) = default; // Copy constructor
    NodeData(NodeData&&) noexcept = default; // Move constructor
    NodeData& operator=(const NodeData&) = default; // Copy assignment operator
    NodeData& operator=(NodeData&&) noexcept = default; // Move assignment operator
    explicit NodeData(NodeValueType t) : type(t), value(Null) {}
    ~NodeData() = default; // Destructor
};

class Node {
protected:
    NodeData data;
    void setInitialValue(const String& value, const String& typeStr);
    void setInitialValue(const VariantType& value);
    void validateTypeAlignment() const;

public:
    bool isConst = false;
    bool isMutable = true;
    bool isStatic = false;
    void setValue(const Node& other);
    void setValue(const VariantType& newValue);
    NodeValueType determineResultType(const Node& left, const Node& right) const;
    String nodeType = "DataNode";

    // Clone Method for Proper Copying
    virtual Node* clone() const;

    // Default Constructor
    Node();

    // Copy Constructor (Handles VarNode without slicing)
    Node(const Node& other);

    // Move Constructor
    Node(Node&& other) noexcept;

    // Copy Assignment Operator
    Node& operator=(const Node& other);

    // Move Assignment Operator
    Node& operator=(Node&& other) noexcept;

    // Constructor accepting VariantType
    explicit Node(const VariantType& value);

    // Constructor accepting String value and type
    Node(const String& value, const String& typeStr);

    // Virtual Destructor
    virtual ~Node();

    NodeValueType getType() const;
    template <typename T>
    T getValue() const;

    template <typename T>
    T convertNumber() const;

    int toInt() const;
    float toFloat() const;
    double toDouble() const;
    long toLong() const;
    String getTypeAsString() const;
    virtual String toString() const;
    bool toBool() const;
    
    NodeValueType getNodeValueType(const String& typeStr, const String& valueStr);
    NodeValueType getNodeValueType(const VariantType& value);

    // Type checks
    bool isType(const NodeValueType type) const;
    bool isInt() const;
    bool isFloat() const;
    bool isChar() const;
    bool isBool() const;
    bool isString() const;
    bool isNumeric() const;
    bool isDouble() const;
    bool isValid() const;
    bool isLong() const;


    VariantType getValue() const;

    Node operator+(const Node& other) const;
    Node operator-(const Node& other) const;
    Node operator*(const Node& other) const;
    Node operator/(const Node& other) const;
    Node operator%(const Node& other) const;

    Node& operator+=(const Node& other);
    Node& operator-=(const Node& other);
    Node& operator/=(const Node& other);
    Node& operator*=(const Node& other);
    
    Node plusEquals(const Node& other);
    Node& minusEquals(const Node& other);
    Node& multEquals(const Node& other);
    Node& divEquals(const Node& other);
    
    bool operator==(const Node& other) const;
    bool operator!=(const Node& other) const;
    bool operator<(const Node& other) const;
    bool operator<=(const Node& other) const;
    bool operator>(const Node& other) const;
    bool operator>=(const Node& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Node& node);
    friend String operator+(const String& lhs, const Node& node);
};

// Literal Node for literals - mostly for clarity in the parser
class LitNode : public Node {
public:
    // Default constructor
    LitNode() : Node() {
        debugLog(debugNode, "===== LitNode was created without initialization.");
    }

    // Constructor accepting a VariantType
    explicit LitNode(const VariantType& value) : Node(value) {
        debugLog(debugNode, "===== LitNode was initialized with VariantType.");
    }

    // Constructor accepting a string value and type
    explicit LitNode(const String& value, const String& typeStr) : Node(value, typeStr) {
        debugLog(debugNode, "===== LitNode was initialized with String and typeStr.");
    }

    // Constructor accepting another Node
    explicit LitNode(const Node& parentNode) : Node(parentNode) {
        debugLog(debugNode, "===== LitNode was initialized from another Node.");
    }

    // Copy constructor
    LitNode(const LitNode& other) : Node(other) {
        debugLog(debugNode, "===== LitNode was copy-constructed.");
    }

    // Move constructor
    LitNode(LitNode&& other) noexcept : Node(std::move(other)) {
        debugLog(debugNode, "===== LitNode was move-constructed.");
    }

    // Copy assignment operator
    LitNode& operator=(const LitNode& other) {
        if (this != &other) {
            Node::operator=(other);
            debugLog(debugNode, "===== LitNode was copy-assigned.");
        }
        return *this;
    }

    // Move assignment operator
    LitNode& operator=(LitNode&& other) noexcept {
        if (this != &other) {
            Node::operator=(std::move(other));
            debugLog(debugNode, "===== LitNode was move-assigned.");
        }
        return *this;
    } 

    String toString() const override {
        try {
            switch (data.type) {
                case NodeValueType::Int:
                    return std::to_string(std::get<int>(data.value));
                case NodeValueType::Float:
                    return std::to_string(std::get<float>(data.value));
                case NodeValueType::Double:
                    return std::to_string(std::get<double>(data.value));
                case NodeValueType::Long:
                    return std::to_string(std::get<long>(data.value));
                case NodeValueType::Bool:
                    return std::get<bool>(data.value) ? "true" : "false";
                case NodeValueType::Char:
                    return std::string(1, std::get<char>(data.value));
                case NodeValueType::String:
                    return std::get<String>(data.value);
                case NodeValueType::Null:
                    return "null"; 
                case NodeValueType::Uninitialized:
                    return "[Uninitialized]";
                case NodeValueType::Any:
                    return "[Any Type]";
                default:
                    throw RunTimeError("Unsupported type for Node toString.");
            }
        } catch (const std::exception& e) {
            debugLog(true, highlight("[Error] Exception in Node::toString():", Colors::red), e.what());
            return "[Error in toString]";
        }
    }

};



// Variable Node for variables
class VarNode : public Node {
public:
    String nodeType = "VarNode";
    
    VarNode();
    explicit VarNode(const VariantType& value, bool isConst = false, bool isMutable = true, bool isStatic = false);
    explicit VarNode(const String& value, const String& typeStr, bool isConst = false, bool isMutable = true, bool isStatic = false);
    explicit VarNode(const Node& parentNode, bool isConst = false, bool isMutable = true, bool isStatic = false);
    
    // Copy Constructor
    VarNode(const VarNode& other);

    // Move Constructor
    VarNode(VarNode&& other) noexcept;

    // Copy Assignment Operator
    VarNode& operator=(const VarNode& other);

    // Move Assignment Operator
    VarNode& operator=(VarNode&& other) noexcept;

    void setValueFromVariant(VariantType newValue) {
        setValue(newValue); // Calls the original setValue method in Node
    }

    // void setValue(const Node& other) override;
    void setValue(const Node& other);
    void setValue(VariantType newValue){Node::setValue(newValue);}
    virtual VarNode* clone() const;
};



















inline bool validateSingleNode(Node node, String methodName, bool debug = true){
    if (debug){
        debugLog(debug, methodName, ": Evaluated Node - ", node);
    }
    return true;
}

inline bool validateConditionIsBool(Node node, String methodName, bool debug = true){
    if (debug){
        if (!node.isBool()) {
            // throw TypeMismatchError("Boolean", node.getTypeAsString(), "IfStatementNode::evaluate");
            throw RunTimeError(methodName + " condition must evaluate to a boolean value.");
        } else {
            debugLog(debug, "Condition for ", methodName, " is a bool value.");
        }
    }
    return true;
}



inline bool validateLeftAndRightNodes(Node leftNode, Node rightNode, String methodName, String op = "", bool debug = true){
    (void)op;
    if (debugNode){
        debugLog(debugNode, "Evaluating leftNode: ", leftNode);
        if (!leftNode.isValid()) {
            throw RunTimeError(methodName + "left Node is invalid in " + methodName);
        } else {
            debugLog(debug, "left Node is valid in " + methodName);
        }

        debugLog(debugNode, "Evaluating rightNode: ", rightNode);
        if (!rightNode.isValid()) {
            throw RunTimeError(methodName + "right Node is invalid in " + methodName);
        } else {
            debugLog(debug, "right Node is valid in " + methodName);
        }
    }
    return true;
}


#endif // NODE_H



