#include "core/node.h"
#include "core/types.h"
#include "core/callables/callable.h"
#include "core/callables/classes/class_base.h"
#include "core/callables/classes/node_structures.h"
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


// const bool debugNodeC = false;
// getIsCallable

// 1. Infer from String
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

// 2. Coerce string to a given type
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

// 3. Validate and Copy
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


// void Node::copyFlagsFrom(const Node& other) {
//     isConst = other.isConst;
//     isMutable = other.isMutable;
//     isStatic = getFullType().getBaseType().size() || isStatic;
//     isCallable = other.isCallable;
//     name = other.name;
//     nodeType = other.nodeType;
//     data = other.data;
//     setFullType(other.getFullType());
//     updateClassInstance(*this);
// }

// void Node::transferOwnershipFrom(Node&& other) {
//     isConst = other.isConst;
//     isMutable = other.isMutable;
//     isStatic = other.isStatic;
//     isCallable = other.isCallable;
//     name = std::move(other.name);
//     nodeType = std::move(other.nodeType);
//     data = other.data;

//     updateClassInstance(*this);
// }


// void Node::updateClassInstance(const Node& me) {
//     if (isInstance()) {
//         auto instance = std::get<SharedPtr<ClassInstance>>(me.data.value);
//         auto instanceNode = instance->getInstanceNode();
//         instanceNode->isMutable = isMutable;
//         instanceNode->isStatic = isStatic;
//         instanceNode->isConst = isConst;
//     }
    
// }

// void Node::applyTypeInfo(std::optional<NodeValueType> typeTag, const ResolvedType& fullType) {
//     if (typeTag.has_value()) {
//         data.type = typeTag.value();
//         if (getFullType().getBaseType().size() < 1) {data.fullType = fullType;}
//     } 
    
//     else if (!getFullType().getBaseType().empty()) {
//         data.fullType = fullType;
//         data.type = stringToNodeType(getFullType().getBaseType());
//     }

//     // Fallback handling
//     if (data.type == NodeValueType::UNKNOWN) {
//         DEBUG_LOG(LogLevel::PERMISSIVE, "");
//         DEBUG_LOG(LogLevel::PERMISSIVE, "UNKNOWN TYPE -> OUTPUT: ", getType());
//         throw MerkError("data.type is Unknown");
//         data.type = NodeValueType::Any;  // fallback
//     }

//     DEBUG_LOG(LogLevel::PERMISSIVE, "data.type is ", getType());
// }



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

// Overload operator<< for Node to display detailed information
std::ostream& operator<<(std::ostream& os, const Node& node) {
    os << node.nodeType << "(";
    os << "Value: ";

    // Display the value
    try {
        os << node.toString();
    } catch (const std::exception& e) {
        os << "<Error retrieving value>";
    }

    SharedPtr<Scope> scope = nullptr;

    // Display the type
    os << ", Type: " << (nodeTypeToString(node.getType()) + "(" + node.name + ")" + " FullType: " + node.getFullType().toString());
    if (node.getType() == NodeValueType::ClassInstance){
        auto instance = std::get<SharedPtr<ClassInstance>>(node.getValue());
        scope = instance->getInstanceScope();        
    }

    
    // Display metadata
    os << ", isConst: " << (node.isConst ? "true" : "false");
    os << ", isMutable: " << (node.isMutable ? "true" : "false");
    os << ", isStatic: " << (node.isStatic ? "true" : "false");

    os << ", isCallable: " << (node.isCallable ? "true" : "false");
    

    os << ")";

    if (scope){
        os << "\n INSTANCE: " + node.name + " DATA START In Scope : " + "Scope(" + std::to_string(scope->getScopeLevel()) + ", " + scope->owner + ")\n";
        // scope->debugPrint();
        for (auto& [varName, var] : scope->getContext().getVariables()) {
            os << varName << " = " << var->toString() << "\n";
        }
        for (auto& [funcName, funcVec] : scope->localFunctions) {
            for (auto& func : funcVec) {
                os << func->getCallable()->toString() << "\n";
            }
        }

        for (auto& [className, cls] : scope->localClasses) {
            os << cls->getCallable()->toString() << "\n";
        }
        
        os << "\n INSTANCE DATA END\n\n";
    }
    return os;
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

    // if (isConst) {
    //     throw MerkError("Cannot reassign a constant Node.");
    // }
    // if (other.getType() == NodeValueType::Null) {
    //     throw MerkError("Cannot assign a Null Node to another Node.");
    // }
    // if (isStatic && getType() != other.getType()) {
    //     throw MerkError("Cannot reassign a statically-typed Node with a different type.");
    // }
    copyFlagsFrom(other);
    

    // Update both value and metadata (since Node allows this)

    // data.value = other.getValue();
    // data.type = other.getType();
    // isConst = other.isConst;
    // isMutable = other.isMutable;
    // isStatic = other.isStatic;

    DEBUG_LOG(LogLevel::DEBUG, "Node::setValue: Value and metadata updated.");
}

// bool Node::isValid() const {
//     return data.type != NodeValueType::Null && !std::holds_alternative<NullType>(data.value);
// }

// bool Node::isType(const NodeValueType type) const {
//     return isValid() && data.type == type;
// }

// // Type checks
// bool Node::isInt() const { return isType(NodeValueType::Int);}
// bool Node::isFloat() const { return isType(NodeValueType::Float);}
// bool Node::isDouble() const { return isType(NodeValueType::Double);}
// bool Node::isLong() const { return isType(NodeValueType::Long); }

// bool Node::isBool() const { return isType(NodeValueType::Bool);}

// bool Node::isChar() const { return isType(NodeValueType::Char);}
// bool Node::isString() const { return isType(NodeValueType::String);}

// // bool Node::isArray() const {return isType(NodeValueType::Array) || (isInstance() && )}
// bool Node::getIsCallable() const { 
//     if (!isCallable){
//         switch (data.type)
//         {
//         case NodeValueType::Function:
//         case NodeValueType::ClassInstance:
//         case NodeValueType::Method:
//         case NodeValueType::Callable:
//         case NodeValueType::Class:
//         return true;
//         default:
//             break;
//         }
//     }
//     return isCallable; 
// }

// bool Node::isSet() const {
//     return data.type  == NodeValueType::Set;
// }
// bool Node::isDict() const {
//     return data.type == NodeValueType::Dict;
// }

// bool Node::isInstance() const {return data.type == NodeValueType::ClassInstance;}

// bool Node::isInstance() {return data.type == NodeValueType::ClassInstance;}


// bool Node::isNumeric() const { return isInt() || isFloat() || isDouble() || isLong(); }

// template <typename T>
// T Node::convertNumber() const {
//     try {
//         if (!isNumeric()) {throw MerkError("Cannot convert a non-numeric type to another.");}

//         switch (data.type) {
//             case NodeValueType::Int:
//                 return static_cast<T>(std::get<int>(data.value));
//             case NodeValueType::Float:
//                 return static_cast<T>(std::get<float>(data.value));
//             case NodeValueType::Double:
//                 return static_cast<T>(std::get<double>(data.value));
//             case NodeValueType::Long:
//                 return static_cast<T>(std::get<long>(data.value));
//             default:
//                 throw MerkError("Cannot convert non-numeric Node to the requested type.");
//         }
//     } catch (std::exception& e) {
//         String out = e.what();
//         throw MerkError("Node::convertNumber failed: " + out);
//     }
    
// }

// int Node::toInt() const {
//     DEBUG_FLOW(FlowLevel::PERMISSIVE);
//     auto res = convertNumber<int>();
//     DEBUG_FLOW_EXIT();
//     return res;
// }

// float Node::toFloat() const {
//     DEBUG_FLOW(FlowLevel::PERMISSIVE);
//     auto res = convertNumber<float>();
//     DEBUG_FLOW_EXIT();
//     return res;
// }

// double Node::toDouble() const {
//     DEBUG_FLOW(FlowLevel::PERMISSIVE);
//     auto res = convertNumber<double>();
//     DEBUG_FLOW_EXIT();
//     return res;
// }

// long Node::toLong() const {
//     DEBUG_FLOW(FlowLevel::PERMISSIVE);
//     auto res = convertNumber<long>();
//     DEBUG_FLOW_EXIT();
//     return res;
// }

// SharedPtr<ListNode> Node::toList() {
//     if (data.type == NodeValueType::List) {
//         if (isList()) {
//             return static_cast<SharedPtr<ListNode>>(std::get<SharedPtr<ListNode>>(data.value));
//         }
//         return std::static_pointer_cast<ListNode>(toInstance()->getNativeData()); 
//     }


//     throw MerkError("Not A List");
// }

// SharedPtr<ListNode> Node::toList() const {
//     if (data.type == NodeValueType::List) {
//         if (isList()) {
//             return static_cast<SharedPtr<ListNode>>(std::get<SharedPtr<ListNode>>(data.value));
//         }
//         return std::static_pointer_cast<ListNode>(toInstance()->getNativeData()); 
//     }


//     throw MerkError("Not A List");
// }

// SharedPtr<ArrayNode> Node::toArray() {
//     if (data.type == NodeValueType::List) {
//         if (isArray()) {
//             return static_cast<SharedPtr<ArrayNode>>(std::get<SharedPtr<ArrayNode>>(data.value));
//         }
//         return std::static_pointer_cast<ArrayNode>(toInstance()->getNativeData());
//     }

//     throw MerkError("Not An Array");
// }

// SharedPtr<ArrayNode> Node::toArray() const {
//     if (data.type == NodeValueType::List) {
//         if (isArray()) {
//             return static_cast<SharedPtr<ArrayNode>>(std::get<SharedPtr<ArrayNode>>(data.value));
//         }
//         return std::static_pointer_cast<ArrayNode>(toInstance()->getNativeData());
//     }

//     throw MerkError("Not An Array");
// }

// SharedPtr<DictNode> Node::toDict() {
//     if (data.type == NodeValueType::List) {
//         if (isDict()) {
//             return static_cast<SharedPtr<DictNode>>(std::get<SharedPtr<DictNode>>(data.value));
//         }
//         return std::static_pointer_cast<DictNode>(toInstance()->getNativeData());

//     }
//     throw MerkError("Not A Dict");
// }

// SharedPtr<DictNode> Node::toDict() const {
//     if (data.type == NodeValueType::List) {
//         if (isDict()) {
//             return static_cast<SharedPtr<DictNode>>(std::get<SharedPtr<DictNode>>(data.value));
//         }
//         return std::static_pointer_cast<DictNode>(toInstance()->getNativeData());

//     }
//     throw MerkError("Not A Dict");
// }

// SharedPtr<SetNode> Node::toSet() {
//     if (data.type == NodeValueType::List) {
//         if (isSet()) {
//             return static_cast<SharedPtr<SetNode>>(std::get<SharedPtr<SetNode>>(data.value));
//         }
//         return std::static_pointer_cast<SetNode>(toInstance()->getNativeData());
//     }

//     throw MerkError("Not A Set");
// }

// SharedPtr<SetNode> Node::toSet() const {
//     if (data.type == NodeValueType::List) {
//         if (isSet()) {
//             return static_cast<SharedPtr<SetNode>>(std::get<SharedPtr<SetNode>>(data.value));
//         }
//         return std::static_pointer_cast<SetNode>(toInstance()->getNativeData());
//     }

//     throw MerkError("Not A Set");
// }


// char Node::toChar() const {
//     return std::get<char>(data.value);
// }

// SharedPtr<ClassInstance> Node::toInstance() const {
//     return std::get<SharedPtr<ClassInstance>>(data.value);
// }

// SharedPtr<ClassInstance> Node::toInstance() {
//     return std::get<SharedPtr<ClassInstance>>(data.value);
// }
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
                    return inst->getNativeData()->holdsValue();
                }
                
            default:
                throw MerkError("No Suitable Conversion From: " + nodeTypeToString(data.type) + " to bool has yet been made");
        }
    
    
}
// bool Node::toBool() const {
//         switch (data.type) {
//             case NodeValueType::Bool: return toBool();
//             case NodeValueType::Int: return toInt() != 0;
//             case NodeValueType::Long: return toLong() != 0;
//             case NodeValueType::Float: return toFloat() != 0.0f;
//             case NodeValueType::Double: return toDouble() != 0.0;
//             case NodeValueType::String: 
//             case NodeValueType::Char: return !std::get<String>(data.value).empty();
//             case NodeValueType::None:
//             case NodeValueType::Null:
//                 return false;
            
//             case NodeValueType::List:
//                 return toList()->getElements().size() > 0;
//             case NodeValueType::Array:
//                 return toArray()->getElements().size() > 0;
            

//             case NodeValueType::ClassInstance:
//                 {
//                     auto inst = std::get<SharedPtr<ClassInstance>>(data.value);
//                     return inst->getNativeData()->holdsValue();
//                 }
                
//             default:
//                 throw MerkError("No Suitable Conversion From: " + nodeTypeToString(data.type) + " to bool has yet been made");
//         }
    
    
// }

// String Node::toString() const {
//     // auto type = getType();
//     // DEBUG_LOG(LogLevel::PERMISSIVE, nodeTypeToString(data.type));
//     // DEBUG_LOG(LogLevel::PERMISSIVE, nodeTypeToString(type));

//     try {
//         switch (data.type) {
//             case NodeValueType::Char: return std::string(1, std::get<char>(data.value));
//             case NodeValueType::Int: return std::to_string(std::get<int>(data.value));
//             // case NodeValueType::Number: return std::to_string(std::get<int>(data.value));
//             case NodeValueType::Float: return std::to_string(std::get<float>(data.value));
//             case NodeValueType::Double: return std::to_string(std::get<double>(data.value));
//             case NodeValueType::Long: return std::to_string(std::get<long>(data.value));
//             case NodeValueType::Bool: return std::get<bool>(data.value) ? "true" : "false";
            
//             case NodeValueType::String: return std::get<String>(data.value);
//             case NodeValueType::Null: return "null"; 
//             case NodeValueType::Uninitialized: return "<Uninitialized>";
//             case NodeValueType::Any: return "Any";
//             case NodeValueType::Class: return "Class";
//             case NodeValueType::Method: return "Method";
            
//             case NodeValueType::ClassInstance: {
//                 DEBUG_LOG(LogLevel::PERMISSIVE, "Got A Class Instance");
//                 auto inst = std::get<SharedPtr<ClassInstance>>(data.value);
//                 return inst->toString();
//             }
//             case NodeValueType::List: {
//                 DEBUG_LOG(LogLevel::PERMISSIVE, "Got A Class Instance");
//                 return std::get<SharedPtr<ListNode>>(data.value)->toString();
//             };
//             case NodeValueType::Array: return "Array";                
//             case NodeValueType::Callable: return "<Callable>" + name;
//             case NodeValueType::UNKNOWN: return "UNKNOWN";
//             case NodeValueType::Function: return "<Function>" + name;
//             case NodeValueType::None: return "None";
//             default: {
//                 return highlight("Error: Unsupported Type For Node toString.'                    '" + nodeTypeToString(data.type), Colors::red) + "  " + toString();
//             }
//         }
//     } catch (const std::exception& e) {
//         return "[Error in Node::toString] " + nodeTypeToString(data.type) + " " + e.what();
//     }

//     // return "";
    
// }

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
        if (std::holds_alternative<SharedPtr<ListNode>>(value)) {throw MerkError("Is A List From The Start"); return NodeValueType::List;}
        if (std::holds_alternative<SharedPtr<ClassInstance>>(value)) return NodeValueType::ClassInstance;
        // if (std::holds_alternative<SharedPtr<FunctionNode>>(value)) return NodeValueType::Function;
        return NodeValueType::Any;  // Allow `Any` when the type is unknown
    } catch (std::exception& e) {
        String out = e.what();
        throw MerkError("Node::getNodeValueType failed: " + out);
    }
    
} 

void Node::validateTypeAlignment() const {
    // if (data.type == NodeValueType::Null) {
    //     throw MerkError("Invalid Node type: Null");
    // }
    if (data.type == NodeValueType::Any) {
        return;  // Allow Any type without restriction
    }
}

void Node::setInitialValue(const VariantType& value) {
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
    

    
}


// bool Node::isList() const {
//     if (name == "List" && data.type != NodeValueType::List) {
//         throw MerkError("List is not a List Type");
//     }
//     return data.type == NodeValueType::List;
// }

// bool Node::isArray() const {
//     if (name == "Array" && data.type != NodeValueType::Array) {
//         throw MerkError("List is not a List Type");
//     }
//     return data.type == NodeValueType::List;
// }


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
    if (isStatic && getType() != other.getType()) {
        throw MerkError("Cannot reassign a statically typed VarNode with a different type.");
    }

    // Only update the value, but not the metadata (isConst, isMutable, isStatic)
    data.value = other.getValue();
    data.type = other.getType();
    updateClassInstance(*this);
    DEBUG_LOG(LogLevel::DEBUG, "VarNode::setValue: Value updated, metadata remains unchanged.");
}

// // Default Constructor
// Node::Node() : data() {
//     DEBUG_LOG(LogLevel::DEBUG, "===== Node was created with default initialization.");
// }

// // Copy Constructor (Handles VarNode properly)
// Node::Node(const Node& other) {
//     if (this == &other) {return;}
//     copyFlagsFrom(other);
//     // this->data = other.data;
//     // isConst = other.isConst;
//     // isMutable = other.isMutable;
//     // isStatic = other.isStatic;
//     // isCallable = other.isCallable;
//     // name = other.name;
//     // if (isInstance()) {
//     //     auto instance = std::get<SharedPtr<ClassInstance>>(getValue());
//     //     instance->getInstanceNode()->isMutable = this->isMutable;
//     // }

//     DEBUG_LOG(LogLevel::DEBUG, "===== Node was copy-constructed.");
// }

// // Move Constructor
// Node::Node(Node&& other) noexcept {
//     auto otherData = other.data;
//     // this->data = std::move(other.data);
//     // other.data.type = NodeValueType::Null; // Reset moved-from object
//     // isConst = other.isConst;
//     // isMutable = other.isMutable;
//     // isStatic = other.isStatic;
//     // isCallable = other.isCallable;
//     // name = other.name;
//     copyFlagsFrom(other);
//     data = otherData;
// }

// // Copy Assignment Operator
// Node& Node::operator=(const Node& other) {
//     if (this != &other) {
//         if (const VarNode* varNode = dynamic_cast<const VarNode*>(&other)) {
//             *this = VarNode(*varNode); // Call VarNode's copy assignment operator
//         } else {
            
//             copyFlagsFrom(other);
            
//             DEBUG_LOG(LogLevel::DEBUG, "===== Node was copy-assigned.");
//         }
//     }
//     return *this;
// }

// // Move Assignment Operator
// Node& Node::operator=(Node&& other) noexcept {
//     if (this != &other) {
//         // data = std::move(other.data);
//         // isConst = other.isConst;
//         // isMutable = other.isMutable;
//         // isStatic = other.isStatic;
//         // isCallable = other.isCallable;
//         // name = other.name;
//         copyFlagsFrom(other);
        
//         DEBUG_LOG(LogLevel::DEBUG, "===== Node was move-assigned.");
//     }
//     return *this;
// }

// // Constructor accepting a VariantType
// Node::Node(const VariantType& value) {
//     setInitialValue(value);
//     validateTypeAlignment();
// }

// // Constructor accepting a string value and type
// Node::Node(const String& value, const String& typeStr) {
//     setInitialValue(value, typeStr);
//     validateTypeAlignment();
// }

// // Destructor
// Node::~Node() {
//     DEBUG_LOG(LogLevel::DEBUG, "===== Node was destroyed.");
//     // data.value._M_reset();
// }

// // Clone Method
// Node* Node::clone() const {
//     return new Node(*this);
// }

// void Node::setFullType(ResolvedType fullRType) const {
//     data.fullType.setBaseType(fullRType.getBaseType());
//     data.fullType.setInner(fullRType.getInnerType());
// }

// // Default constructor
// LitNode::LitNode() : Node() {
//     nodeType = "LitNode";
//     DEBUG_LOG(LogLevel::TRACE, "===== LitNode was created without initialization.");
// }

// // Constructor accepting a VariantType
// LitNode::LitNode(const VariantType& value) : Node(value) {
//     nodeType = "LitNode";
//     DEBUG_LOG(LogLevel::TRACE, "===== LitNode was initialized with VariantType.");
// }

// // Constructor accepting a string value and type
// LitNode::LitNode(const String& value, const String& typeStr) : Node(value, typeStr) {
//     nodeType = "LitNode";
//     DEBUG_LOG(LogLevel::TRACE, "===== LitNode was initialized with String and typeStr.");
// }

// // Constructor accepting another Node
// LitNode::LitNode(const Node& parentNode) : Node(parentNode) {
//     nodeType = "LitNode";
//     DEBUG_LOG(LogLevel::TRACE, "===== LitNode was initialized from another Node.");
// }

// // Copy constructor
// LitNode::LitNode(const LitNode& other) : Node(other) {
//     nodeType = "LitNode";
//     DEBUG_LOG(LogLevel::TRACE, "===== LitNode was copy-constructed.");
// }

// // Move constructor
// LitNode::LitNode(LitNode&& other) noexcept : Node(std::move(other)) {
//     nodeType = "LitNode";
//     DEBUG_LOG(LogLevel::TRACE, "===== LitNode was move-constructed.");
// }

// LitNode& LitNode::operator=(const LitNode& other) {
//     nodeType = "LitNode";
//     if (this != &other) {
//         Node::operator=(other);
//     }
//     return *this;
// }

// // Move assignment operator
// LitNode& LitNode::operator=(LitNode&& other) noexcept {
//     nodeType = "LitNode";

//     if (this != &other) {
//         Node::operator=(std::move(other));
//     }
//     return *this;
// }











// // VarNode Default Constructor
// VarNode::VarNode() : Node() {
//     nodeType = "VarNode";
//     DEBUG_LOG(LogLevel::DEBUG, "===== VarNode was created without initialization.");
// }

// // VarNode Constructor accepting VariantType
// VarNode::VarNode(const VariantType& value, bool isConst, bool isMutable, bool isStatic)
//     : Node(value) {
//     nodeType = "VarNode";
//     this->isConst = isConst;
//     this->isMutable = isMutable;
//     this->isStatic = isStatic;
//     updateClassInstance(*this);
// }

// // VarNode Constructor accepting String value and type
// VarNode::VarNode(const String& value, const String& typeStr, bool isConst, bool isMutable, bool isStatic)
//     : Node(value, typeStr) {
//     nodeType = "VarNode";
//     this->isConst = isConst;
//     this->isMutable = isMutable;
//     this->isStatic = isStatic;
//     updateClassInstance(*this);
// }

// // VarNode Constructor accepting another Node
// VarNode::VarNode(const Node& parentNode, bool isConst, bool isMutable, bool isStatic)
//     : Node(parentNode) {
//     nodeType = "VarNode";

//     this->isConst = parentNode.isConst || isConst;
//     this->isMutable = parentNode.isMutable || isMutable;
//     this->isStatic = parentNode.isStatic || isStatic;
//     this->isCallable = parentNode.isCallable;
//     this->name = parentNode.name;
//     this->nodeType = parentNode.nodeType;
//     updateClassInstance(*this);
// }

// // VarNode Copy Constructor
// VarNode::VarNode(const VarNode& other) : Node(other) {
    

//     this->isConst = other.isConst;
//     this->isMutable = other.isMutable;
//     this->isStatic = other.isStatic;
//     this->isCallable = other.isCallable;
//     this->name = other.name;
//     this->nodeType = other.nodeType;

//     copyFlagsFrom(other);
//     nodeType = "VarNode";
//     updateClassInstance(*this);
// }

// // VarNode Move Constructor
// VarNode::VarNode(VarNode&& other) noexcept : Node(std::move(other)) {
//     this->isConst = other.isConst;
//     this->isMutable = other.isMutable;
//     this->isStatic = other.isStatic;
//     this->isCallable = other.isCallable;
//     this->name = other.name;
//     this->nodeType = other.nodeType;
//     updateClassInstance(*this);
// }

// // Copy Assignment Operator
// VarNode& VarNode::operator=(const VarNode& other) {
//     if (this != &other) {
//         Node::operator=(other);
//     }

//     updateClassInstance(*this);
//     return *this;
// }

// // Move Assignment Operator
// VarNode& VarNode::operator=(VarNode&& other) noexcept {
//     if (this != &other) {
//         Node::operator=(std::move(other));
//     }
//     updateClassInstance(*this);

//     return *this;
// }

// VarNode* VarNode::clone() const {
//     return new VarNode(*this);
// }


// // String LitNode::toString() const {
// //     try {
// //         switch (data.type) {
// //             case NodeValueType::Int: return std::to_string(std::get<int>(data.value));
// //             case NodeValueType::Number: return std::to_string(std::get<int>(data.value));
// //             case NodeValueType::Float: return std::to_string(std::get<float>(data.value));
// //             case NodeValueType::Double: return std::to_string(std::get<double>(data.value));
// //             case NodeValueType::Long: return std::to_string(std::get<long>(data.value));
// //             case NodeValueType::Bool: return std::get<bool>(data.value) ? "true" : "false";
// //             case NodeValueType::Char: return std::string(1, std::get<char>(data.value));
// //             case NodeValueType::String: return std::get<String>(data.value);
// //             case NodeValueType::Null: return "null"; 
// //             case NodeValueType::Uninitialized: return "<Uninitialized>";
// //             case NodeValueType::Any: return "Any";
// //             case NodeValueType::Class: return "Class";
// //             case NodeValueType::Method: return "Method";
            
// //             case NodeValueType::ClassInstance: {
// //                 DEBUG_LOG(LogLevel::PERMISSIVE, "Got A Class Instance");
// //                 auto inst = std::get<SharedPtr<ClassInstance>>(data.value);
// //                 return inst->toString();
// //             }
// //             case NodeValueType::List: {
// //                 DEBUG_LOG(LogLevel::PERMISSIVE, "Got A Class Instance");
// //                 return std::get<SharedPtr<ListNode>>(data.value)->toString();
// //             };
// //             case NodeValueType::Array: return "Array";                
// //             case NodeValueType::Callable: return "<Callable>" + name;
// //             case NodeValueType::UNKNOWN: return "UNKNOWN";
// //             case NodeValueType::Function: return "<Function>" + name;
// //             case NodeValueType::None: return "None";
// //             default: throw RunTimeError("");
// //         }
// //     } catch (const std::exception& e) {
// //         debugLog(true, highlight("[Error] Exception in LitNode::toString():", Colors::red), e.what());
// //         return "[Error in LitNode::toString]";
// //     }

// //     // return "";
// // }



// // For Variable Name Part Construction
// VarNode::VarNode(const String value, const String& typeStr, bool isConst, bool isMutable, std::optional<NodeValueType> typeTag, bool isStatic)
//     : Node(value, typeStr) {
//     nodeType = "VarNode";
//     this->isConst = isConst;
//     this->isMutable = isMutable;
//     applyTypeInfo(typeTag, getFullType());

//     this->isStatic = isStatic && typeTag.has_value();
//     if (typeTag.has_value()){
//         this->data.type = typeTag.value_or(NodeValueType::Any);
//     }

//     validateTypeAlignment();
// }

// // For Variable Name Part and ResolvedVariable Construction
// VarNode::VarNode(VarNode& parent, bool isConst, bool isMutable, std::optional<NodeValueType> typeTag, bool isStatic)
//     :Node(parent) {
//     nodeType = "VarNode";
//     copyFlagsFrom(parent);
//     this->isStatic = typeTag.has_value() || isStatic;
//     this->isConst = isConst;
//     this->isMutable = isMutable;

//     validateTypeAlignment();
// }

// VarNode::VarNode(const String value, const String& typeStr, bool isConst, bool isMutable, ResolvedType fullType, bool isStatic)
//     : Node(value, typeStr) {
//         nodeType = "VarNode";
//         this->isConst = isConst;
//         this->isMutable = isMutable;
//         this->isStatic = fullType.getBaseType().size() || isStatic; 
//         this->data.fullType = fullType;

        
//         DEBUG_LOG(LogLevel::PERMISSIVE, "VarNode FullType: ", fullType.getBaseType(), "Type: ", typeStr);
//         data.type = stringToNodeType(fullType.getBaseType());
//         if (data.type == NodeValueType::UNKNOWN) {throw MerkError("data.type is Unknown");  data.type = NodeValueType::Any;}   // Temporary solution for user defined types
        
//         DEBUG_LOG(LogLevel::PERMISSIVE, "data.type is ", data.type);
//         validateTypeAlignment();
// }

// VarNode::VarNode(VarNode& parent, bool isConst, bool isMutable, ResolvedType fullType, bool isStatic)
//     :Node (parent) {
//         nodeType = "VarNode";
//         this->isConst = isConst;
//         this->isMutable = isMutable;
//         this->isStatic = fullType.getBaseType().size() || isStatic; // && parent.data.type != NodeValueType::Uninitialized;
//         applyTypeInfo(std::nullopt, fullType);

//         this->data.type = parent.getType();
        
//         if (data.type == NodeValueType::UNKNOWN) {DEBUG_LOG(LogLevel::PERMISSIVE, "UNKNOWN TYPE OUTPUT: ", data.type); throw MerkError("data.type is Unknown"); data.type = NodeValueType::Any;}   // Temporary solution for user defined types
        
//         DEBUG_LOG(LogLevel::PERMISSIVE, "VarNode FullType: ", fullType.getBaseType(), "Type: ", parent.getTypeAsString());
//         DEBUG_LOG(LogLevel::PERMISSIVE, "data.type is ", data.type);
//         updateClassInstance(*this);

//         validateTypeAlignment();
//     }


UniquePtr<VarNode> cloneVarNode(VarNode* original) {
    return UniquePtr<VarNode>(original);
}



