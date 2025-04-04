#include "core/node.h"
#include "core/types.h"
#include "utilities/debugger.h"
#include "utilities/debugging_functions.h"
#include "core/errors.h"

#include <limits>
#include <sstream>
#include <iostream>
#include <variant>
#include <tuple>
#include <functional>
#include <utility>
#include <cmath>


// const bool debugNodeC = false;

bool areFloatsEqual(float a, float b) {
    float diff = std::fabs(a - b);
    float tolerance = std::numeric_limits<float>::epsilon() * std::max(std::fabs(a), std::fabs(b));
    return diff <= tolerance;
}

bool areDoublesEqual(double a, double b) {
    double diff = std::fabs(a - b);
    double tolerance = std::numeric_limits<double>::epsilon() * std::max(std::fabs(a), std::fabs(b));
    return diff <= tolerance;
}

bool compareNumericNodes(const Node& left, const Node& right, const std::function<bool(double, double)>& comparator) {
    if (!(left.isNumeric() && right.isNumeric())) {
        throw MerkError("Attempting to compare non-numeric Nodes.");
    }

    double leftValue = left.toDouble();
    double rightValue = right.toDouble();

    return comparator(leftValue, rightValue);
}

// Overload operator<< for Node to display detailed information
std::ostream& operator<<(std::ostream& os, const Node& node) {
    os << "Node(";
    os << "Value: ";

    // Display the value
    try {
        os << node.toString();
    } catch (const std::exception& e) {
        os << "<Error retrieving value>";
    }

    // Display the type
    os << ", Type: ";
    switch (node.getType()) {
        case NodeValueType::Int: os << "Int"; break;
        case NodeValueType::Float: os << "Float"; break;
        case NodeValueType::Double: os << "Double"; break;
        case NodeValueType::String: os << "String"; break;
        case NodeValueType::Bool: os << "Bool"; break;
        case NodeValueType::Long: os << "Long"; break;
        case NodeValueType::Null: os << "Null"; break;
        default: os << "Unknown"; break;
    }

    // Display metadata
    os << ", isConst: " << (node.isConst ? "true" : "false");
    os << ", isMutable: " << (node.isMutable ? "true" : "false");
    os << ", isStatic: " << (node.isStatic ? "true" : "false");

    os << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const VarNode& node) {
    os << "VarNode(";
    os << "Value: ";

    // Display the value
    try {
        os << node.toString();
    } catch (const std::exception& e) {
        os << "<Error retrieving value>";
    }

    // Display the type
    os << ", Type: ";
    switch (node.getType()) {
        case NodeValueType::Int: os << "Int"; break;
        case NodeValueType::Float: os << "Float"; break;
        case NodeValueType::Double: os << "Double"; break;
        case NodeValueType::String: os << "String"; break;
        case NodeValueType::Bool: os << "Bool"; break;
        case NodeValueType::Long: os << "Long"; break;
        case NodeValueType::Null: os << "Null"; break;
        default: os << "Unknown"; break;
    }

    // Display metadata
    os << ", isConst: " << (node.isConst ? "true" : "false");
    os << ", isMutable: " << (node.isMutable ? "true" : "false");
    os << ", isStatic: " << (node.isStatic ? "true" : "false");

    os << ")";
    return os;
}

// Overload to accept value and type as strings
void Node::setInitialValue(const String& value, const String& typeStr) {
    // DEBUG_LOG(LogLevel::DEBUGC, "Entering Node::setInitialValue 2 ARGS");
    NodeValueType nodeType = getNodeValueType(typeStr, value);

    switch (nodeType) {
        case NodeValueType::Int:
            data.type = NodeValueType::Int;
            data.value = std::stoi(value);
            break;
        case NodeValueType::Float:
            data.type = NodeValueType::Float;
            data.value = std::stof(value);
            break;
        case NodeValueType::String:
            data.type = NodeValueType::String;
            data.value = value;
            break;
        case NodeValueType::Bool:
            if (value == "true") {
                data.type = NodeValueType::Bool;
                data.value = true;
            } else if (value == "false") {
                data.type = NodeValueType::Bool;
                data.value = false;
            } else {
                throw MerkError("Invalid boolean string: " + value);
            }
            break;
        default:
            throw MerkError("Unsupported NodeValueType in setInitialValue.");
    }

    // DEBUG_LOG(LogLevel::DEBUGC, "Set initial value with String. Type: ", static_cast<int>(data.type), ", Value: ", data.value);
}


void Node::setValue(const VariantType& newValue) {
    DEBUG_LOG(LogLevel::DEBUG, "[Debug] Node::setValue() - Setting new value with type:", nodeTypeToString(getNodeValueType(newValue)));

    if (isConst) {
        throw MerkError("Cannot reassign a constant Node.");
    }

    if (isStatic && data.type != getNodeValueType(newValue)) {
        throw MerkError("Cannot reassign a statically typed Node with a different type.");
    }

    NodeValueType newType = getNodeValueType(newValue);

    if (data.type == NodeValueType::Any) {
        // Allow assignment of any type if type is 'Any'
        data.value = newValue;
        data.type = newType;
    } else if (isStatic && data.type != newType) {
        throw MerkError("Cannot reassign a statically typed Node with a different type.");
    } else if (newType == NodeValueType::Null) {
        // Allow assignment of Null *only* if the existing type is `Any` or `Null`
        if (data.type != NodeValueType::Null && data.type != NodeValueType::Any) {
            throw MerkError("Cannot assign NullType to a non-nullable Node.");
        }
        data.value = newValue;
        data.type = NodeValueType::Null;
    } else {
        validateTypeAlignment();
        data.value = newValue;
    }

    
}

void Node::setValue(const Node& other) {

    if (isConst) {
        throw MerkError("Cannot reassign a constant Node.");
    }
    if (other.getType() == NodeValueType::Null) {
        throw MerkError("Cannot assign a Null Node to another Node.");
    }
    if (isStatic && getType() != other.getType()) {
        throw MerkError("Cannot reassign a statically-typed Node with a different type.");
    }

    // Update both value and metadata (since Node allows this)
    data.value = other.getValue();
    data.type = other.getType();
    isConst = other.isConst;
    isMutable = other.isMutable;
    isStatic = other.isStatic;

    DEBUG_LOG(LogLevel::DEBUG, "Node::setValue: Value and metadata updated.");
}

bool Node::isValid() const {
    return data.type != NodeValueType::Null && !std::holds_alternative<NullType>(data.value);
}

bool Node::isType(const NodeValueType type) const {
    return isValid() && data.type == type;
}

// Type checks
bool Node::isInt() const { return isType(NodeValueType::Int);}
bool Node::isFloat() const { return isType(NodeValueType::Float);}
bool Node::isDouble() const { return isType(NodeValueType::Double);}
bool Node::isLong() const { return isType(NodeValueType::Long); }

bool Node::isBool() const { return isType(NodeValueType::Bool);}

bool Node::isChar() const { return isType(NodeValueType::Char);}
bool Node::isString() const { return isType(NodeValueType::String);}


bool Node::isNumeric() const { return isInt() || isFloat() || isDouble() || isLong(); }

template <typename T>
T Node::convertNumber() const {
    if (!isNumeric()) {throw MerkError("Cannot convert a non-numeric type to another.");}

    switch (data.type) {
        case NodeValueType::Int:
            return static_cast<T>(std::get<int>(data.value));
        case NodeValueType::Float:
            return static_cast<T>(std::get<float>(data.value));
        case NodeValueType::Double:
            return static_cast<T>(std::get<double>(data.value));
        case NodeValueType::Long:
            return static_cast<T>(std::get<long>(data.value));
        default:
            throw MerkError("Cannot convert non-numeric Node to the requested type.");
    }
}

int Node::toInt() const {
    return convertNumber<int>();
}

float Node::toFloat() const {
    return convertNumber<float>();
}

double Node::toDouble() const {
    return convertNumber<double>();
}

long Node::toLong() const {
    return convertNumber<long>();
}

bool Node::toBool() const {
    if (data.type != NodeValueType::Bool) {
        throw MerkError("Cannot convert non-boolean Node to bool.");
    }
    return std::get<bool>(data.value);
}

String Node::toString() const {
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
                throw MerkError("Unsupported type for Node toString.");
        }
    } catch (const std::exception& e) {
        debugLog(true, highlight("[Error] Exception in Node::toString():", Colors::red), e.what());
        return "[Error in toString]";
    }
    
}

NodeValueType Node::getNodeValueType(const String& typeStr, const String& valueStr) {
    if (typeStr == "Variable" || typeStr == "Argument" || typeStr == "FunctionCall"){
        data.type = NodeValueType::String;
        data.value = valueStr;
        return NodeValueType::String;
    } 
    
    else if (typeStr == "Parameter"){
        data.type = NodeValueType::String;
        data.value = valueStr;
        return NodeValueType::String;
    }

    if (typeStr == "Number") {
        try {
            // Check if it has a decimal point or scientific notation
            if (valueStr.find('.') != String::npos || valueStr.find('e') != String::npos || valueStr.find('E') != String::npos) {
                double doubleValue = std::stod(valueStr); // Try converting to double
                if (doubleValue >= -std::numeric_limits<float>::max() &&
                    doubleValue <= std::numeric_limits<float>::max()) {
                    return NodeValueType::Float; // Fits within float range
                }
                return NodeValueType::Double; // Otherwise, it's a double
            }

            // Otherwise, try integer conversions
            long longValue = std::stol(valueStr); // Try converting to long
            if (longValue >= std::numeric_limits<int>::min() &&
                longValue <= std::numeric_limits<int>::max()) {
                return NodeValueType::Int; // Fits within int range
            }
            return NodeValueType::Long; // Otherwise, it's a long
        } catch (const std::exception&) {
            throw MerkError("Invalid numeric string: " + valueStr);
        }
    } else if (typeStr == "String") {
        return NodeValueType::String;
    } else if (typeStr == "Bool") {
        return NodeValueType::Bool;
    } else if (typeStr == "Null") {
        return NodeValueType::Null;
    }

    throw MerkError("Unknown type string: " + typeStr);
}

NodeValueType Node::getNodeValueType(const VariantType& value) {
    if (std::holds_alternative<int>(value)) return NodeValueType::Int;
    if (std::holds_alternative<float>(value)) return NodeValueType::Float;
    if (std::holds_alternative<double>(value)) return NodeValueType::Double;
    if (std::holds_alternative<long>(value)) return NodeValueType::Long;
    if (std::holds_alternative<String>(value)) return NodeValueType::String;
    if (std::holds_alternative<bool>(value)) return NodeValueType::Bool;
    if (std::holds_alternative<NullType>(value)) return NodeValueType::Null;
    return NodeValueType::Any;  // Allow `Any` when the type is unknown
}

void Node::validateTypeAlignment() const {
    if (data.type == NodeValueType::Null) {
        throw MerkError("Invalid Node type: Null");
    }
    if (data.type == NodeValueType::Any) {
        return;  // Allow Any type without restriction
    }
}

void Node::setInitialValue(const VariantType& value) {
    std::visit(
        [this](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            data.type = getNodeTypeFromType<T>();
            data.value = arg; // Store the value
        },
        value
    );
}


// Getter for the value
VariantType Node::getValue() const {
    if (data.type == NodeValueType::Null) {
        throw MerkError("Attempting to get value from a Node of type Null.");
    }
    return data.value; // Return the value stored in the NodeData
}


// Getter for the type
NodeValueType Node::getType() const {
    return data.type;
}

String Node::getTypeAsString() const {
    return nodeTypeToString(getType());
}

NodeValueType Node::determineResultType(const Node& left, const Node& right) const {
    if (!isValid() || !right.isValid()) {
        throw MerkError("Invalid Node(s) used in arithmetic operation.");
    }

    // Check if either is static and enforce type rules
    if (left.isStatic || right.isStatic) {
        if (left.getType() == right.getType()) {
            return left.getType(); // Both are the same static type
        }
        throw MerkError("Static types cannot be implicitly promoted.");
    }

    // Promote to Float if either is Float
    if (left.getType() == NodeValueType::Float || right.getType() == NodeValueType::Float) {
        return NodeValueType::Float;
    }

    // Promote to Double if either is Double
    if (left.getType() == NodeValueType::Double || right.getType() == NodeValueType::Double) {
        return NodeValueType::Double;
    }

    // Promote to Double if either is Double
    if (left.getType() == NodeValueType::Long || right.getType() == NodeValueType::Long) {
        return NodeValueType::Long;
    }

    // Otherwise, keep as Int
    return NodeValueType::Int;
}

// Helper function to determine the result type for numeric operations
NodeValueType determineNumericResultType(const Node& left, const Node& right) {
    if (!left.isNumeric() || !right.isNumeric()) {
        throw MerkError("Cannot perform arithmetic on non-numeric types.");
    }

    // Follow type promotion rules for dynamic variables
    if (!left.isStatic && !right.isStatic) {
        if (left.isDouble() || right.isDouble()) return NodeValueType::Double;
        if ((left.isFloat() && right.isFloat()) || (left.isFloat() && right.isInt()) || (left.isInt() && right.isFloat())) {
            return NodeValueType::Float; // Ensure int+float stays float
        }
        if (left.isLong() || right.isLong()) return NodeValueType::Long;
        return NodeValueType::Int;
    }

    // If both are static, they must match
    if (left.isStatic && right.isStatic) {
        if (left.getType() != right.getType()) {
            throw MerkError("Cannot perform arithmetic on mismatched static types.");
        }
        return left.getType();
    }

    // If one is static, enforce the static type
    return left.isStatic ? left.getType() : right.getType();
}


// Helper function to validate if a node can be modified

void validateModifiability(const Node& node) {
    if (node.isConst) {
        std::cout << "🚨 validateModifiability: Attempting to modify a constant Node! 🚨" << std::endl;
        throw MerkError("Cannot modify a constant Node.");
    }

    if (!node.isMutable) {
        std::cout << "🚨 validateModifiability: Attempting to modify an immutable Node! 🚨" << std::endl;
        throw MerkError("Cannot modify an immutable Node.");
    }
}

// Generalized arithmetic operation function
Node performArithmeticOperation(const Node& left, const Node& right, 
                                const std::function<double(double, double)>& operation) {
    NodeValueType resultType = determineNumericResultType(left, right);

    double leftValue = left.toDouble();
    double rightValue = right.toDouble();
    double result = operation(leftValue, rightValue);

    // Return result with appropriate type
    if (resultType == NodeValueType::Int) return Node(static_cast<int>(result));
    if (resultType == NodeValueType::Float) return Node(static_cast<float>(result));
    if (resultType == NodeValueType::Double) return Node(result);
    if (resultType == NodeValueType::Long) return Node(static_cast<long>(result));

    throw MerkError("Unsupported numeric type in arithmetic operation.");
}

// Addition Operator
Node Node::operator+(const Node& other) const {
    if (isString() && other.isString()) {
        return Node(toString() + other.toString());
    }
    return performArithmeticOperation(*this, other, [](double a, double b) { return a + b; });
}

// Subtraction Operator
Node Node::operator-(const Node& other) const {
    return performArithmeticOperation(*this, other, [](double a, double b) { return a - b; });
}

// Multiplication Operator
Node Node::operator*(const Node& other) const {
    return performArithmeticOperation(*this, other, [](double a, double b) { return a * b; });
}

// Division Operator (With Static Type Safety)
Node Node::operator/(const Node& other) const {
    if (other.toDouble() == 0.0) {
        throw MerkError("Division by zero.");
    }
    
    NodeValueType resultType = determineNumericResultType(*this, other);
    if (isStatic && other.isStatic && resultType == NodeValueType::Int) {
        throw MerkError("Cannot perform integer division that results in non-integer.");
    }

    return performArithmeticOperation(*this, other, [](double a, double b) { return a / b; });
}

// Modulo Operator (Integer Only)
Node Node::operator%(const Node& other) const {
    if (!(isInt() && other.isInt())) {
        throw MerkError("Modulo is only supported for integer types.");
    }
    return Node(toInt() % other.toInt());
}

// Assignment Operators (`+=`, `-=`, `*=`, `/=`)
Node& Node::operator+=(const Node& other) {
    validateModifiability(*this);
    auto val = performArithmeticOperation(*this, other, [](double a, double b) { return a + b; });
    setValue(val.getValue());
    // *this = *this + other;
    return *this;
}


Node& Node::operator-=(const Node& other) {
    validateModifiability(*this);
    // this->setValue(this->data.value + other.getValue())
    *this = *this - other;
    return *this;
}

Node& Node::operator*=(const Node& other) {
    validateModifiability(*this);
    *this = *this * other;
    return *this;
}

Node& Node::operator/=(const Node& other) {
    validateModifiability(*this);
    *this = *this / other;
    return *this;
}


Node Node::plusEquals(const Node& other) {

    return *this += other;
};

Node& Node::minusEquals(const Node& other) {
    return *this -= other;
};
Node& Node::multEquals(const Node& other) {
    return *this *= other;
};
Node& Node::divEquals(const Node& other) {
    return *this /= other;
};

// Comparison Operators
bool Node::operator==(const Node& other) const {
if (std::holds_alternative<UninitializedType>(data.value) ||
    std::holds_alternative<UninitializedType>(other.data.value)) {
    return std::holds_alternative<UninitializedType>(data.value) &&
           std::holds_alternative<UninitializedType>(other.data.value);
}
    if (data.type == NodeValueType::Null || other.data.type == NodeValueType::Null) {
        return data.type == NodeValueType::Null && other.data.type == NodeValueType::Null;
    }

    if (isNumeric() && other.isNumeric()) {
        if (isFloat() || other.isFloat()) {
            return areFloatsEqual(toFloat(), other.toFloat());
        }
        if (isDouble() || other.isDouble()) {
            return areDoublesEqual(toDouble(), other.toDouble());
        }
        return toLong() == other.toLong();  // Use `toLong` for integer-based comparison
    }

    if (isStatic && (data.type != other.data.type)) {
        return false;
    }

    switch (data.type) {
        case NodeValueType::Int:
            return toInt() == other.toInt();
        case NodeValueType::Float:
            return areFloatsEqual(toFloat(), other.toFloat());
        case NodeValueType::Double:
            return areDoublesEqual(toDouble(), other.toDouble());
        case NodeValueType::String:
            return toString() == other.toString();
        case NodeValueType::Bool:
            return toBool() == other.toBool();
        case NodeValueType::Long:
            return toLong() == other.toLong();
        default:
            return false;
    }


    return getValue() == other.getValue();
}

bool Node::operator!=(const Node& other) const { return !(*this == other); }

bool Node::operator<(const Node& other) const {
    if (!isNumeric() || !other.isNumeric()) throw MerkError("Comparison '<' is only supported for numbers.");
    return toDouble() < other.toDouble();
}

bool Node::operator<=(const Node& other) const {
    if (!isNumeric() || !other.isNumeric()) throw MerkError("Comparison '<=' is only supported for numbers.");
    return toDouble() <= other.toDouble();
}

bool Node::operator>(const Node& other) const {
    if (!isNumeric() || !other.isNumeric()) throw MerkError("Comparison '>' is only supported for numbers.");
    return toDouble() > other.toDouble();
}

bool Node::operator>=(const Node& other) const {
    if (!isNumeric() || !other.isNumeric()) throw MerkError("Comparison '>=' is only supported for numbers.");
    return toDouble() >= other.toDouble();
}
// Print Overloads
String operator+(const String& lhs, const Node& node) {
    std::ostringstream oss;
    oss << lhs << node.toString();
    return oss.str();
}

String operator+(const Node& node, const String& rhs) {
    std::ostringstream oss;
    oss << node.toString() << rhs;
    return oss.str();
}


void VarNode::setValue(const Node& other) {
    if (isConst) {
        throw MerkError("Cannot reassign a constant VarNode.");
    }
    if (other.getType() == NodeValueType::Null) {
        throw MerkError("Cannot assign a Null Node to a VarNode.");
    }
    if (isStatic && getType() != other.getType()) {
        throw MerkError("Cannot reassign a statically typed VarNode with a different type.");
    }

    // Only update the value, but not the metadata (isConst, isMutable, isStatic)
    data.value = other.getValue();
    data.type = other.getType();

    DEBUG_LOG(LogLevel::DEBUG, "VarNode::setValue: Value updated, metadata remains unchanged.");
}

// Default Constructor
Node::Node() : data() {
    DEBUG_LOG(LogLevel::DEBUG, "===== Node was created with default initialization.");
}

// Copy Constructor (Handles VarNode properly)
Node::Node(const Node& other) {
    if (this == &other) return;

    this->data = other.data;
    isConst = other.isConst;
    isMutable = other.isMutable;
    isStatic = other.isStatic;

    DEBUG_LOG(LogLevel::DEBUG, "===== Node was copy-constructed.");
}

// Move Constructor
Node::Node(Node&& other) noexcept {
    this->data = std::move(other.data);
    other.data.type = NodeValueType::Null; // Reset moved-from object
    isConst = other.isConst;
    isMutable = other.isMutable;
    isStatic = other.isStatic;
    DEBUG_LOG(LogLevel::DEBUG, "===== Node was move-constructed.");
}

// Copy Assignment Operator
Node& Node::operator=(const Node& other) {
    if (this != &other) {
        if (const VarNode* varNode = dynamic_cast<const VarNode*>(&other)) {
            *this = VarNode(*varNode); // Call VarNode's copy assignment operator
        } else {
            data = other.data;
            isConst = other.isConst;
            isMutable = other.isMutable;
            isStatic = other.isStatic;
            DEBUG_LOG(LogLevel::DEBUG, "===== Node was copy-assigned.");
        }
    }
    return *this;
}

// Move Assignment Operator
Node& Node::operator=(Node&& other) noexcept {
    if (this != &other) {
        data = std::move(other.data);
        isConst = other.isConst;
        isMutable = other.isMutable;
        isStatic = other.isStatic;
        DEBUG_LOG(LogLevel::DEBUG, "===== Node was move-assigned.");
    }
    return *this;
}

// Constructor accepting a VariantType
Node::Node(const VariantType& value) {
    setInitialValue(value);
    validateTypeAlignment();
}

// Constructor accepting a string value and type
Node::Node(const String& value, const String& typeStr) {
    setInitialValue(value, typeStr);
    validateTypeAlignment();
}

// Destructor
Node::~Node() {
    DEBUG_LOG(LogLevel::DEBUG, "===== Node was destroyed.");
}

// Clone Method
Node* Node::clone() const {
    return new Node(*this);
}

// VarNode Default Constructor
VarNode::VarNode() : Node() {
    DEBUG_LOG(LogLevel::DEBUG, "===== VarNode was created without initialization.");
}

// VarNode Constructor accepting VariantType
VarNode::VarNode(const VariantType& value, bool isConst, bool isMutable, bool isStatic)
    : Node(value) {
    this->isConst = isConst;
    this->isMutable = isMutable;
    this->isStatic = isStatic;
}

// VarNode Constructor accepting String value and type
VarNode::VarNode(const String& value, const String& typeStr, bool isConst, bool isMutable, bool isStatic)
    : Node(value, typeStr) {
    this->isConst = isConst;
    this->isMutable = isMutable;
    this->isStatic = isStatic;
}

// VarNode Constructor accepting another Node
VarNode::VarNode(const Node& parentNode, bool isConst, bool isMutable, bool isStatic)
    : Node(parentNode) {
    if (parentNode.getType() == NodeValueType::Null) {
        throw MerkError("Cannot create a VarNode from an untyped (Null) parent Node.");
    }
    this->isConst = isConst;
    this->isMutable = isMutable;
    this->isStatic = isStatic;
}

// VarNode Copy Constructor
VarNode::VarNode(const VarNode& other) : Node(other) {
    this->isConst = other.isConst;
    this->isMutable = other.isMutable;
    this->isStatic = other.isStatic;
}

// VarNode Move Constructor
VarNode::VarNode(VarNode&& other) noexcept : Node(std::move(other)) {
    this->isConst = other.isConst;
    this->isMutable = other.isMutable;
    this->isStatic = other.isStatic;
}

// Copy Assignment Operator
VarNode& VarNode::operator=(const VarNode& other) {
    if (this != &other) {
        Node::operator=(other);
    }
    return *this;
}

// Move Assignment Operator
VarNode& VarNode::operator=(VarNode&& other) noexcept {
    if (this != &other) {
        Node::operator=(std::move(other));
    }
    return *this;
}

VarNode* VarNode::clone() const {
    return new VarNode(*this);
}