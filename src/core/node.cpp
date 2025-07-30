#include "core/node.h"
#include "core/types.h"
#include "core/callables/callable.h"
#include "core/callables/classes/class_base.h"
#include "core/callables/classes/node_structures.h"
// #include "core/callables/argument_node.h"
#include "utilities/debugger.h"
#include "utilities/debugging_functions.h"
#include "core/errors.h"
#include "core/scope.h"

#include <limits>
#include <sstream>
#include <iostream>
#include <variant>
#include <tuple>
#include <functional>
#include <utility>
#include <cmath>

// Infer from String
std::pair<VariantType, NodeValueType> inferFromString(const String& value) {
    if (value == "true" || value == "false") {
        return { value == "true", NodeValueType::Bool };
    }

    try {
        if (value.find('.') != String::npos) {
            return { std::stod(value), NodeValueType::Double };
        } else {
            return { std::stoi(value), NodeValueType::Int };
        }
    } catch (...) {}

    if (value.size() == 3 && value.front() == '\'' && value.back() == '\'') {
        return { value[1], NodeValueType::Char };
    }

    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return { value.substr(1, value.size() - 2), NodeValueType::String };
    }

    throw MerkError("Could not infer type from string: " + value);
}

// Coerce string to a given type
std::pair<VariantType, NodeValueType> coerceStringToType(const String& value, NodeValueType type) {
    try {
        switch (type) {
            case NodeValueType::Int: return { std::stoi(value), type };
            case NodeValueType::Float: return { std::stof(value), type };
            case NodeValueType::Double: return { std::stod(value), type };
            case NodeValueType::Long: return { std::stol(value), type };
            case NodeValueType::Bool:
                if (value == "true") return { true, type };
                if (value == "false") return { false, type };
                throw MerkError("Invalid boolean: " + value);
            case NodeValueType::Char:
                if (value.size() == 3 && value.front() == '\'' && value.back() == '\'')
                    return { value[1], type };
                throw MerkError("Invalid char: " + value);
            case NodeValueType::String:
                return { value, type };
            default:
                throw MerkError("Unsupported coercion target type: " + nodeTypeToString(type));
        }
    } catch (const std::exception& e) {
        throw MerkError("Failed to coerce value '" + value + "' to type " + nodeTypeToString(type));
    }
}

// Validate and Copy
std::pair<VariantType, NodeValueType> validateAndCopy(const VariantType& value, NodeValueType type) {
    auto visitor = [&](auto&& arg) -> std::pair<VariantType, NodeValueType> {
        using T = std::decay_t<decltype(arg)>;
        if (getNodeTypeFromType<T>() != type) {
            throw MerkError("Value does not match declared NodeValueType: expected " + nodeTypeToString(type));
        }
        return { arg, type };
    };
    return std::visit(visitor, value);
}

NodeData::~NodeData() {
    value = 0;
}

bool validateSingleNode(Node node, String methodName, bool debug){
    if (debug){
        MARK_UNUSED_MULTI(node, methodName);

        DEBUG_LOG(LogLevel::TRACE, methodName, ": Evaluated Node - ", node);
    }
    return true;
}

bool validateConditionIsBool(Node node, String methodName, bool debug){
    if (debug){
        if (!node.isBool()) {
            // throw TypeMismatchError("Boolean", node.getTypeAsString(), "IfStatementNode::evaluate");
            throw RunTimeError(methodName + " condition must evaluate to a boolean value.");
        } else {
            DEBUG_LOG(LogLevel::TRACE, "Condition for ", methodName, " is a bool value.");
        }
    }
    return true;
}

bool validateLeftAndRightNodes(Node leftNode, Node rightNode, String methodName, String op, bool debug){
    (void)op;
    if (debug){
        DEBUG_LOG(LogLevel::TRACE, "Evaluating leftNode: ", leftNode);
        if (!leftNode.isValid()) {
            throw RunTimeError(methodName + "left Node is invalid in " + methodName);
        } else {
            DEBUG_LOG(LogLevel::TRACE, "left Node is valid in " + methodName);
        }

        DEBUG_LOG(LogLevel::TRACE, "Evaluating rightNode: ", rightNode);
        if (!rightNode.isValid()) {
            throw RunTimeError(methodName + "right Node is invalid in " + methodName);
        } else {
            DEBUG_LOG(LogLevel::TRACE, "right Node is valid in " + methodName);
        }
    }
    return true;
}

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
    if (!(left.isNumeric() && right.isNumeric())) {throw MerkError("Attempting to compare non-numeric Nodes.");}

    double leftValue = left.toDouble();
    double rightValue = right.toDouble();

    return comparator(leftValue, rightValue);
}




// Overload to accept value and type as strings
void Node::setInitialValue(const String& value, const String& typeStr) {
    // DEBUG_LOG(LogLevel::DEBUGC, "Entering Node::setInitialValue 2 ARGS");
    NodeValueType nodeType = getNodeValueType(typeStr, value);
    
    data.type = nodeType;
    switch (nodeType) {
        case NodeValueType::Int:
            data.value = std::stoi(value);
            break;
        case NodeValueType::Float:
            data.value = std::stof(value);
            break;
        case NodeValueType::String:
            data.value = value;
            break;
        case NodeValueType::Bool:
            if (value == "true") {
                data.value = true;
            } else if (value == "false") {
                data.value = false;
            } else {
                throw MerkError("Invalid boolean string: " + value);
            }
            break;
        case NodeValueType::Long:
            data.value = std::stol(value);
            break;
        case NodeValueType::Char:
            if (!value.empty()){
                data.value = char(value[0]);
            }
            else {data.value = char(0);}
            break;
        case NodeValueType::None:
        case NodeValueType::Null:
            data.value = NullType();
            break;

        default:
            throw MerkError("Unsupported NodeValueType in setInitialValue.");
    }

    // DEBUG_LOG(LogLevel::DEBUGC, "Set initial value with String. Type: ", static_cast<int>(data.type), ", Value: ", data.value);
}


void Node::setValue(const VariantType& newValue) {
    DEBUG_LOG(LogLevel::DEBUG, "[Debug] Node::setValue() - Setting new value with type:", nodeTypeToString(getNodeValueType(newValue)));

    if (isConst) {throw MerkError("Cannot reassign a constant Node.");}

    if (isStatic && data.type != getNodeValueType(newValue)) {throw MerkError("Cannot reassign a statically typed Node with a different type.");}

    NodeValueType newType = getNodeValueType(newValue);

    if (newType == NodeValueType::List) {throw MerkError("Is A List in setValue");}
    // else {throw MerkError("Is Not A List, but: " + nodeTypeToString(newType) + " in setValue");}

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
        
        data.value = newValue;
        data.type = newType;
        // data.type = ;
        validateTypeAlignment();
    }

    
}


bool Node::toBool() const {
        switch (data.type) {
            case NodeValueType::Bool: return std::get<bool>(data.value);
            case NodeValueType::Int: return toInt() != 0;
            case NodeValueType::Long: return toLong() != 0;
            case NodeValueType::Float: return toFloat() != 0.0f;
            case NodeValueType::Double: return toDouble() != 0.0;
            case NodeValueType::String: 
            case NodeValueType::Char: return toString().empty();
            case NodeValueType::None:
            case NodeValueType::Null:
                return false;
            
            case NodeValueType::List:
                return toList()->getElements().size() > 0;
            case NodeValueType::Array:
                return toArray()->getElements().size() > 0;
            

            case NodeValueType::ClassInstance:
                {
                    auto inst = std::get<SharedPtr<ClassInstance>>(data.value);
                    auto data = inst->getNativeData();
                    // throw MerkError("Got Data");
                    if (data) {
                        // throw MerkError("Got HAS DATA REALLY");
                        return data->holdsValue();
                    }
                    return false;
                }
                
            default:
                throw MerkError("No Suitable Conversion From: " + nodeTypeToString(data.type) + " to bool has yet been made");
        }
    
    
}

Node Node::negate() const {
    if (!isNumeric()) throw MerkError("Cannot negate non-numeric Node.");
    DEBUG_LOG(LogLevel::ERROR, "Type: ", nodeTypeToString(getType()));

    switch (getType()) {
        case NodeValueType::Int:
            return Node(-toInt());
        case NodeValueType::Float:
            return Node(-toFloat());
        case NodeValueType::Double:
            return Node(-toDouble());
        case NodeValueType::Long:
            return Node(-toLong());
        default:
            throw MerkError("Unsupported type for negation.");
    }
}

NodeValueType Node::getNodeValueType(const String& typeStr, const String& valueStr) {
    if (typeStr == "Variable" || typeStr == "AccessorVariable" || typeStr == "Argument" || typeStr == "FunctionCall" || typeStr == "ClassMethodCall"){
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
    } else if (typeStr == "Char") {
        return NodeValueType::Char;
    }

    throw MerkError("Unknown type string: " + typeStr);
}

NodeValueType Node::getNodeValueType(const VariantType& value) {
    try {
        if (std::holds_alternative<int>(value)) return NodeValueType::Int;
        if (std::holds_alternative<float>(value)) return NodeValueType::Float;
        if (std::holds_alternative<double>(value)) return NodeValueType::Double;
        if (std::holds_alternative<long>(value)) return NodeValueType::Long;
        if (std::holds_alternative<String>(value)) return NodeValueType::String;
        if (std::holds_alternative<bool>(value)) return NodeValueType::Bool;
        if (std::holds_alternative<char>(value)) return NodeValueType::Char;
        if (std::holds_alternative<NullType>(value)) return NodeValueType::Null;
        if (std::holds_alternative<SharedPtr<ArrayNode>>(value)) {throw MerkError("Is A List From The Start"); return NodeValueType::Array;}
        if (std::holds_alternative<SharedPtr<ListNode>>(value)) {throw MerkError("Is A List From The Start"); return NodeValueType::List;}
        if (std::holds_alternative<SharedPtr<SetNode>>(value)) {throw MerkError("Is A List From The Start"); return NodeValueType::Set;}
        if (std::holds_alternative<SharedPtr<DictNode>>(value)) {throw MerkError("Is A List From The Start"); return NodeValueType::Dict;}
        if (std::holds_alternative<SharedPtr<ClassInstance>>(value)) return NodeValueType::ClassInstance;
        // if (std::holds_alternative<SharedPtr<FunctionNode>>(value)) return NodeValueType::Function;
        return NodeValueType::Any;  // Allow `Any` when the type is unknown
    } catch (std::exception& e) {
        String out = e.what();
        throw MerkError("Node::getNodeValueType failed: " + out);
    }
    
} 

void Node::validateTypeAlignment() const {
    if (data.type == NodeValueType::Any) { return;  /*Allow Any type without restriction*/}
}

void Node::setInitialValue(const VariantType& value) {
    // if (getValue() == value) {return;}
    try {
        std::visit(
            [this](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                data.type = getNodeTypeFromType<T>();
                data.value = arg; // Store the value
            },
            value
        );
    }  catch (std::exception& e) {
        String out = e.what();
        throw MerkError("Node::getNodeValueType failed: " + out);
    }

    // if (data.type != NodeValueType::List && data.type != NodeValueType::Uninitialized && data.type != NodeValueType::String) {throw MerkError("Not Correct type of: " + nodeTypeToString(data.type) + " " + toString());}

    if (data.type == NodeValueType::List && isInstance()) {
        throw MerkError("When Initially Setting List value, it is an Instance");
    }
}

// Getter for the value
VariantType Node::getValue() const {
    return data.value; // Return the value stored in the NodeData
}


// Getter for the type
NodeValueType Node::getType() const {
    // return getNodeTypeFromType();
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
        if (left.isDouble() || right.isDouble()) {return NodeValueType::Double;}
        if (left.isFloat() || right.isFloat()) {return NodeValueType::Float;}
        if (left.isLong() || right.isLong()) {return NodeValueType::Long;}

        // Otherwise Int
        return NodeValueType::Int;
    }

    // If both are static, they must match
    if (left.isStatic && right.isStatic) {
        if (left.getType() != right.getType()) {
            throw MerkError("Cannot perform arithmetic on mismatched static types.");
        }
        return left.getType();
    }

    return left.isStatic ? left.getType() : right.getType();
}


// Helper function to validate if a node can be modified
void validateModifiability(const Node& node) {
    if (node.isConst) {
        std::cout << "validateModifiability: Attempting to modify a constant Node!" << std::endl;
        throw MerkError("Cannot modify a constant Node.");
    }

    if (!node.isMutable) {
        std::cout << "validateModifiability: Attempting to modify an immutable Node!" << std::endl;
        throw MerkError("Cannot modify an immutable Node.");
    }
}

// Generalized arithmetic operation function
Node performArithmeticOperation(const Node& left, const Node& right, 
                                const std::function<double(double, double)>& operation) {
    NodeValueType resultType = determineNumericResultType(left, right);
    if (left.isStatic && left.getType() != resultType) {
        throw MerkError("Types Don't Match For Node " + nodeTypeToString(left.getType()) + " -> " + nodeTypeToString(resultType));
    }
    DEBUG_LOG(LogLevel::TRACE, "ResultType: ", nodeTypeToString(resultType));

    double leftValue = left.toDouble();
    double rightValue = right.toDouble();
    double result = operation(leftValue, rightValue);

    // Return result with appropriate type
    if (resultType == NodeValueType::Int) {
        DEBUG_LOG(LogLevel::TRACE, "Returning INT");
        return Node(static_cast<int>(result));
    }
    if (resultType == NodeValueType::Float) {
        DEBUG_LOG(LogLevel::TRACE, "Returning FLOAT");

        return Node(static_cast<float>(result));
    }
    if (resultType == NodeValueType::Double) {
        DEBUG_LOG(LogLevel::TRACE, "Returning DOUBLE");
        return Node(result);
    }
    if (resultType == NodeValueType::Long) {
        DEBUG_LOG(LogLevel::TRACE, "Returning LONG");

        return Node(static_cast<long>(result));
    }

    throw MerkError("Unsupported numeric type in arithmetic operation.");
}

// Addition Operator
Node Node::operator+(const Node& other) const {
    if (isString() && other.isString()) {
        return Node(toString() + other.toString());
    }
    if (std::holds_alternative<char>(data.value) && std::holds_alternative<char>(other.data.value)) { 
        return Node(std::string() + std::get<char>(data.value) + std::get<char>(other.data.value));  // construct string from both chars
    }
    else if ((isString() && !other.isString()) || (!isString() && other.isString())){
        throw MerkError("Cannot concatenate string with another type");
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
    if (other.toDouble() == 0.0) {throw MerkError("Division by zero.");}
    
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
    // return getValue() == other.getValue();
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
    try {
        switch (data.type) {
            case NodeValueType::Int:
                return toInt() == other.toInt();
            case NodeValueType::Float:
                return areFloatsEqual(toFloat(), other.toFloat());
            case NodeValueType::Double:
                return areDoublesEqual(toDouble(), other.toDouble());
            case NodeValueType::String:
            case NodeValueType::Char:
                return toString() == other.toString();
            case NodeValueType::Bool:
                return toBool() == other.toBool();
            case NodeValueType::Long:
                return toLong() == other.toLong();
            
            default:
                return false;
        }
    } catch (std::exception& e) {
        String out = e.what();
        throw MerkError("Node::operator== failed: " + out);
    }
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




UniquePtr<VarNode> cloneVarNode(VarNode* original) {
    return UniquePtr<VarNode>(original);
}


std::size_t Node::hash() const {
    std::size_t h1 = std::hash<int>()(static_cast<int>(data.type));
    std::size_t h2 = 0;

    std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, NodeList>) {
            // Hash the vector contents
            std::size_t combined = 0;
            for (const auto& n : arg) {
                combined ^= n.hash() + 0x9e3779b9 + (combined << 6) + (combined >> 2);
            }
            h2 = combined;
        } else {
            h2 = std::hash<T>()(arg);
        }
    }, data.value);

    std::size_t h3 = std::hash<std::string>()(name);
    std::size_t h4 = (isConst << 1) ^ (isMutable << 2) ^ (isStatic << 3);

    return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
}
