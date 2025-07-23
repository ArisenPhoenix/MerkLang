#include "core/node.h"
#include "core/types.h"
#include "utilities/debugger.h"
#include "core/errors.h"
#include "core/scope.h"





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
    return data.type == type;
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
    auto res = convertNumber<int>();
    return res;
}

float Node::toFloat() const {
    auto res = convertNumber<float>();
    return res;
}

double Node::toDouble() const {
    auto res = convertNumber<double>();
    return res;
}

long Node::toLong() const {
    auto res = convertNumber<long>();
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
    try {
        switch (data.type) {
            case NodeValueType::Char: return std::string(1, std::get<char>(data.value));
            case NodeValueType::Int: return std::to_string(std::get<int>(data.value));
            case NodeValueType::Number: return std::to_string(std::get<int>(data.value));
            case NodeValueType::Float: return std::to_string(toFloat());
            case NodeValueType::Double: return std::to_string(toDouble());
            case NodeValueType::Long: return std::to_string(toLong());
            case NodeValueType::Bool: return std::get<bool>(data.value) == true ? "true" : "false";
            
            case NodeValueType::String: return std::get<String>(data.value);
            case NodeValueType::Null: return "null"; 
            case NodeValueType::Uninitialized: return "<Uninitialized>";
            case NodeValueType::Any: return nodeTypeToString(data.type, false);
            case NodeValueType::Class: return nodeTypeToString(data.type, false);
            case NodeValueType::Method: return nodeTypeToString(data.type, false);
            
            case NodeValueType::ClassInstance: { return toInstance()->toString(); }
            case NodeValueType::List: { return toList()->toString(); };
            case NodeValueType::Array: return nodeTypeToString(data.type, false);              
            case NodeValueType::Callable: return nodeTypeToString(data.type, false) + name;
            case NodeValueType::UNKNOWN: return "UNKNOWN";
            case NodeValueType::Function: return nodeTypeToString(data.type, false) + name;
            case NodeValueType::None: return nodeTypeToString(data.type, false);
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
            case NodeValueType::Char: return std::string(1, std::get<char>(data.value));
            case NodeValueType::Int: return std::to_string(std::get<int>(data.value));
            case NodeValueType::Number: return std::to_string(std::get<int>(data.value));
            case NodeValueType::Float: return std::to_string(toFloat());
            case NodeValueType::Double: return std::to_string(toDouble());
            case NodeValueType::Long: return std::to_string(toLong());
            case NodeValueType::Bool: return std::get<bool>(data.value) == true ? "true" : "false";
            
            case NodeValueType::String: return std::get<String>(data.value);
            case NodeValueType::Null: return "null"; 
            case NodeValueType::Uninitialized: return "<Uninitialized>";
            case NodeValueType::Any: return nodeTypeToString(data.type, false);
            case NodeValueType::Class: return nodeTypeToString(data.type, false);
            case NodeValueType::Method: return nodeTypeToString(data.type, false);
            
            case NodeValueType::ClassInstance: { return toInstance()->toString(); }
            case NodeValueType::List: { return toList()->toString(); };
            case NodeValueType::Array: return nodeTypeToString(data.type, false);              
            case NodeValueType::Callable: return nodeTypeToString(data.type, false) + name;
            case NodeValueType::UNKNOWN: return "UNKNOWN";
            case NodeValueType::Function: return nodeTypeToString(data.type, false) + name;
            case NodeValueType::None: return nodeTypeToString(data.type, false);
            default: throw RunTimeError("No Accepted Variables Found");
        }
    } catch (const std::exception& e) {
        debugLog(true, highlight("[Error] Exception in LitNode::toString():", Colors::red), e.what());
        return "[Error in LitNode::toString]";
    }

    // return "";
}