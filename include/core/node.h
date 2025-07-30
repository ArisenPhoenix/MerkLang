#ifndef NODE_H
#define NODE_H

#include <string>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <variant>
#include "core/types.h"
#include <optional>

#include <functional>


// Struct for NodeData using a union for efficient storage
struct NodeData {
    NodeValueType type = NodeValueType::Null; // Type of the value
    mutable ResolvedType fullType;
    VariantType value = Null; // A std::variant to store any of the allowed types
    NodeData() = default; // Default constructor
    NodeData(const NodeData&) = default; // Copy constructor
    NodeData(NodeData&&) noexcept = default; // Move constructor
    NodeData& operator=(const NodeData&) = default; // Copy assignment operator
    NodeData& operator=(NodeData&&) noexcept = default; // Move assignment operator
    explicit NodeData(NodeValueType t) : type(t), value(Null) {}
    ~NodeData(); // Destructor
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
    bool isCallable = false;

    void copyFlagsFrom(const Node& other);

    void transferOwnershipFrom(Node&& other);

    void applyTypeInfo(std::optional<NodeValueType> typeTag, const ResolvedType& fullType);

    void updateClassInstance(const Node& me);

    void setValue(const Node& other);
    void setValue(const VariantType& newValue);
    NodeValueType determineResultType(const Node& left, const Node& right) const;
    String nodeType = "DataNode";
    String name = "";
    String key = "";

    ResolvedType getFullType() {return data.fullType;}
    ResolvedType getFullType() const {return data.fullType;}
    virtual std::size_t hash() const;
    

    
    // Clone Method for Proper Copying
    virtual Node* clone() const;

    void setFullType(ResolvedType) const;

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
    char toChar() const;
    SharedPtr<ListNode> toList();
    SharedPtr<ArrayNode> toArray();
    SharedPtr<SetNode> toSet();
    SharedPtr<DictNode> toDict();

    SharedPtr<ListNode> toList() const;
    SharedPtr<ArrayNode> toArray() const;
    SharedPtr<SetNode> toSet() const;
    SharedPtr<DictNode> toDict() const;
    

    SharedPtr<ClassInstance> toInstance() const;
    SharedPtr<ClassInstance> toInstance();
    
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
    bool getIsCallable() const;
    bool isArray() const;
    bool isList() const;
    bool isSet() const;
    bool isDict() const;
    bool isInstance();
    bool isInstance() const;

    Node negate() const;
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

    // Node resolveMember(const String& name, IdentifierType type) const;

};



// Literal Node for literals - mostly for clarity in the parser
class LitNode : public Node {
public:
    // Default constructor
    LitNode();

    // Constructor accepting a VariantType
    explicit LitNode(const VariantType& value);

    // Constructor accepting a string value and type
    explicit LitNode(const String& value, const String& typeStr);

    // Constructor accepting another Node
    explicit LitNode(const Node& parentNode);

    // Copy constructor
    LitNode(const LitNode& other);

    // Move constructor
    LitNode(LitNode&& other) noexcept;

    // Copy assignment operator
    LitNode& operator=(const LitNode& other);

    // Move assignment operator
    LitNode& operator=(LitNode&& other) noexcept; 

    String toString() const override;

};



// Variable Node for variables
class VarNode : public Node {
public:
    
    VarNode();
    explicit VarNode(const VariantType& value, bool isConst = false, bool isMutable = true, bool isStatic = false);
    explicit VarNode(const String& value, const String& typeStr, bool isConst = false, bool isMutable = true, bool isStatic = false);
    explicit VarNode(const Node& parentNode, bool isConst = false, bool isMutable = true, bool isStatic = false);

    // // For Static Typing
    explicit VarNode(const String value, const String& type, bool isConst, bool isMutable, std::optional<NodeValueType> typeTag, bool isStatic = false);
    explicit VarNode(VarNode& parent, bool isConst, bool isMutable, std::optional<NodeValueType> typeTag, bool isStatic = true);

    explicit VarNode(const String value, const String& type, bool isConst, bool isMutable, ResolvedType typeTag, bool isStatic = false);
    explicit VarNode(VarNode& parent, bool isConst, bool isMutable, ResolvedType typeTag, bool isStatic = true);
    
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



inline bool validateSingleNode(Node node, String methodName, bool debug = true);

inline bool validateConditionIsBool(Node node, String methodName, bool debug = true);

bool validateLeftAndRightNodes(Node leftNode, Node rightNode, String methodName, String op = "", bool debug = true);





class ComposableTypeNode : public Node {
    String baseType;         // e.g., "List" or "Map"
    Vector<SharedPtr<Node>> innerTypes; // e.g., [Int], or [String, Int]
};

UniquePtr<VarNode> cloneVarNode(VarNode* original);


namespace std {
    template<>
    struct hash<Node> {
        std::size_t operator()(const Node& n) const noexcept {
            return n.hash();
        }
    };
}

#endif // NODE_H



