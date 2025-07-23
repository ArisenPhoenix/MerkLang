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




bool Node::isList() const {
    if (name == "List" && data.type != NodeValueType::List) {
        throw MerkError("List is not a List Type");
    }
    return data.type == NodeValueType::List;
}

bool Node::isArray() const {
    if (name == "Array" && data.type != NodeValueType::Array) {
        throw MerkError("List is not a List Type");
    }
    return data.type == NodeValueType::List;
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

// bool Node::isArray() const {return isType(NodeValueType::Array) || (isInstance() && )}
bool Node::getIsCallable() const { 
    if (!isCallable){
        switch (data.type)
        {
        case NodeValueType::Function:
        case NodeValueType::ClassInstance:
        case NodeValueType::Method:
        case NodeValueType::Callable:
        case NodeValueType::Class:
        return true;
        default:
            break;
        }
    }
    return isCallable; 
}

bool Node::isSet() const {
    return data.type  == NodeValueType::Set;
}
bool Node::isDict() const {
    return data.type == NodeValueType::Dict;
}

bool Node::isInstance() const {return data.type == NodeValueType::ClassInstance;}

bool Node::isInstance() {return data.type == NodeValueType::ClassInstance;}


bool Node::isNumeric() const { return isInt() || isFloat() || isDouble() || isLong(); }

template <typename T>
T Node::convertNumber() const {
    try {
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
    } catch (std::exception& e) {
        String out = e.what();
        throw MerkError("Node::convertNumber failed: " + out);
    }
    
}

int Node::toInt() const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    auto res = convertNumber<int>();
    DEBUG_FLOW_EXIT();
    return res;
}

float Node::toFloat() const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    auto res = convertNumber<float>();
    DEBUG_FLOW_EXIT();
    return res;
}

double Node::toDouble() const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    auto res = convertNumber<double>();
    DEBUG_FLOW_EXIT();
    return res;
}

long Node::toLong() const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    auto res = convertNumber<long>();
    DEBUG_FLOW_EXIT();
    return res;
}

SharedPtr<ListNode> Node::toList() {
    if (data.type == NodeValueType::List) {
        if (isList()) {
            return static_cast<SharedPtr<ListNode>>(std::get<SharedPtr<ListNode>>(data.value));
        }
        return std::static_pointer_cast<ListNode>(toInstance()->getNativeData()); 
    }


    throw MerkError("Not A List");
}

SharedPtr<ListNode> Node::toList() const {
    if (data.type == NodeValueType::List) {
        if (isList()) {
            return static_cast<SharedPtr<ListNode>>(std::get<SharedPtr<ListNode>>(data.value));
        }
        return std::static_pointer_cast<ListNode>(toInstance()->getNativeData()); 
    }


    throw MerkError("Not A List");
}

SharedPtr<ArrayNode> Node::toArray() {
    if (data.type == NodeValueType::List) {
        if (isArray()) {
            return static_cast<SharedPtr<ArrayNode>>(std::get<SharedPtr<ArrayNode>>(data.value));
        }
        return std::static_pointer_cast<ArrayNode>(toInstance()->getNativeData());
    }

    throw MerkError("Not An Array");
}

SharedPtr<ArrayNode> Node::toArray() const {
    if (data.type == NodeValueType::List) {
        if (isArray()) {
            return static_cast<SharedPtr<ArrayNode>>(std::get<SharedPtr<ArrayNode>>(data.value));
        }
        return std::static_pointer_cast<ArrayNode>(toInstance()->getNativeData());
    }

    throw MerkError("Not An Array");
}




char Node::toChar() const {
    return std::get<char>(data.value);
}

SharedPtr<ClassInstance> Node::toInstance() const {
    return std::get<SharedPtr<ClassInstance>>(data.value);
}

SharedPtr<ClassInstance> Node::toInstance() {
    return std::get<SharedPtr<ClassInstance>>(data.value);
}



String Node::toString() const {
    // auto type = getType();
    // DEBUG_LOG(LogLevel::PERMISSIVE, nodeTypeToString(data.type));
    // DEBUG_LOG(LogLevel::PERMISSIVE, nodeTypeToString(type));

    try {
        switch (data.type) {
            case NodeValueType::Char: return std::string(1, std::get<char>(data.value));
            case NodeValueType::Int: return std::to_string(std::get<int>(data.value));
            // case NodeValueType::Number: return std::to_string(std::get<int>(data.value));
            case NodeValueType::Float: return std::to_string(std::get<float>(data.value));
            case NodeValueType::Double: return std::to_string(std::get<double>(data.value));
            case NodeValueType::Long: return std::to_string(std::get<long>(data.value));
            case NodeValueType::Bool: return std::get<bool>(data.value) ? "true" : "false";
            
            case NodeValueType::String: return std::get<String>(data.value);
            case NodeValueType::Null: return "null"; 
            case NodeValueType::Uninitialized: return "<Uninitialized>";
            case NodeValueType::Any: return "Any";
            case NodeValueType::Class: return "Class";
            case NodeValueType::Method: return "Method";
            
            case NodeValueType::ClassInstance: {
                DEBUG_LOG(LogLevel::PERMISSIVE, "Got A Class Instance");
                auto inst = std::get<SharedPtr<ClassInstance>>(data.value);
                return inst->toString();
            }
            case NodeValueType::List: {
                DEBUG_LOG(LogLevel::PERMISSIVE, "Got A Class Instance");
                return std::get<SharedPtr<ListNode>>(data.value)->toString();
            };
            case NodeValueType::Array: return "Array";                
            case NodeValueType::Callable: return "<Callable>" + name;
            case NodeValueType::UNKNOWN: return "UNKNOWN";
            case NodeValueType::Function: return "<Function>" + name;
            case NodeValueType::None: return "None";
            default: {
                return highlight("Error: Unsupported Type For Node toString.'                    '" + nodeTypeToString(data.type), Colors::red) + "  " + toString();
            }
        }
    } catch (const std::exception& e) {
        return "[Error in Node::toString] " + nodeTypeToString(data.type) + " " + e.what();
    }
    
}






String LitNode::toString() const {
    try {
        switch (data.type) {
            case NodeValueType::Int: return std::to_string(std::get<int>(data.value));
            case NodeValueType::Number: return std::to_string(std::get<int>(data.value));
            case NodeValueType::Float: return std::to_string(std::get<float>(data.value));
            case NodeValueType::Double: return std::to_string(std::get<double>(data.value));
            case NodeValueType::Long: return std::to_string(std::get<long>(data.value));
            case NodeValueType::Bool: return std::get<bool>(data.value) ? "true" : "false";
            case NodeValueType::Char: return std::string(1, std::get<char>(data.value));
            case NodeValueType::String: return std::get<String>(data.value);
            case NodeValueType::Null: return "null"; 
            case NodeValueType::Uninitialized: return "<Uninitialized>";
            case NodeValueType::Any: return "Any";
            case NodeValueType::Class: return "Class";
            case NodeValueType::Method: return "Method";
            
            case NodeValueType::ClassInstance: {
                DEBUG_LOG(LogLevel::PERMISSIVE, "Got A Class Instance");
                auto inst = std::get<SharedPtr<ClassInstance>>(data.value);
                return inst->toString();
            }
            case NodeValueType::List: {
                DEBUG_LOG(LogLevel::PERMISSIVE, "Got A Class Instance");
                return std::get<SharedPtr<ListNode>>(data.value)->toString();
            };
            case NodeValueType::Array: return "Array";                
            case NodeValueType::Callable: return "<Callable>" + name;
            case NodeValueType::UNKNOWN: return "UNKNOWN";
            case NodeValueType::Function: return "<Function>" + name;
            case NodeValueType::None: return "None";
            default: throw RunTimeError("");
        }
    } catch (const std::exception& e) {
        debugLog(true, highlight("[Error] Exception in LitNode::toString():", Colors::red), e.what());
        return "[Error in LitNode::toString]";
    }

    // return "";
}