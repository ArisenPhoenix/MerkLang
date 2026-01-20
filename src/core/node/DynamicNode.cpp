#include "core/node/Node.hpp"
#include "core/callables/classes/ClassBase.hpp"
#include "core/callables/functions/Function.hpp"
#include "core/callables/Callable.hpp"
#include "core/node/Node.hpp"
#include "core/node/NodeStructures.hpp"
#include "core/types.h"
#include "core/Scope.hpp"
#include <charconv>



template <typename T>
T get(VariantType v) { return std::get<T>(v); }


template <typename T>
T DynamicNode::parseExact(std::string_view s, const char* what) {
    if constexpr (std::is_integral_v<T>) {
        T out{};
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
        if (ec != std::errc{} || ptr != s.data() + s.size()) {
            throw MerkError(String("Invalid ") + what + " literal: '" + String(s) + "'");
        }
        return out;
    }
    else if constexpr (std::is_floating_point_v<T>) {
        // strtoX needs a null-terminated buffer
        String tmp(s);

        char* end = nullptr;
        errno = 0;

        long double v = std::strtold(tmp.c_str(), &end);

        if (errno != 0 || end != tmp.c_str() + tmp.size()) {
            throw MerkError(String("Invalid ") + what + " literal: '" + String(s) + "'");
        }

        // ange-check before cast if you care
        return static_cast<T>(v);
    }
    else {
        static_assert(!sizeof(T), "parseExact<T>: unsupported T");
    }
}



static std::optional<std::string_view> tryStringView(const VariantType& v) {
    if (std::holds_alternative<String>(v)) {
        const auto& s = std::get<String>(v);
        return std::string_view(s.data(), s.size());
    }
    // If you have TextVariant nested, handle it here too.
    // else if constexpr (...) { ... }
    return std::nullopt;
}



// static T forceTo(const VariantType& v, CoerceMode mode = CoerceMode::Permissive);

[[noreturn]] static void fail(NodeValueType t, const String& msg = "") {
    throw MerkError("Cannot force " + nodeTypeToString(t) + " to Float" + (msg.empty()? "" : (": "+msg)));
}

template <>
int DynamicNode::forceTo<int>(const VariantType& v, CoerceMode mode) {
    if (std::holds_alternative<int>(v)) {return std::get<int>(v);}

    NodeValueType t = getTypeFromValue(v);

    switch (t) {
        case NodeValueType::Bool:   {return std::get<bool>(v) ? 1 : 0;}
        case NodeValueType::Char:   {return (int)std::get<char>(v);}

        case NodeValueType::Float:
            if (mode == CoerceMode::Strict) fail(t, "float->int not allowed");
            return (int)std::get<float>(v);

        case NodeValueType::Double:
            if (mode == CoerceMode::Strict) fail(t, "double->int not allowed");
            return (int)std::get<double>(v);

        case NodeValueType::Long:
            if (mode == CoerceMode::Strict) fail(t, "long->int not allowed");
            return (int)std::get<long>(v);

        case NodeValueType::String:
        case NodeValueType::Text: {
            // normalize extraction in ONE place
            if (std::holds_alternative<String>(v)) {
                const auto& s = std::get<String>(v);
                return parseExact<int>(s, "int");
            }
            fail(t, "text storage not String");
        }

        default:
            fail(t);
    }
}

template <>
String DynamicNode::forceTo<String>(const VariantType& v, CoerceMode mode) {
    (void)mode;
    NodeValueType t = getTypeFromValue(v);

    switch (t) {
        case NodeValueType::String:
        case NodeValueType::Text:
            return std::get<String>(v);

        case NodeValueType::Int:    return std::to_string(std::get<int>(v));
        case NodeValueType::Long:   return std::to_string(std::get<long>(v));
        case NodeValueType::Float:  return std::to_string(std::get<float>(v));
        case NodeValueType::Double: return std::to_string(std::get<double>(v));
        case NodeValueType::Bool:   return std::get<bool>(v) ? "true" : "false";
        case NodeValueType::Char:   return String(1, std::get<char>(v));
        case NodeValueType::Null:   return "null";

        case NodeValueType::Callable:
        case NodeValueType::ClassInstance:
            return std::get<SharedPtr<Callable>>(v)->toString();

        default:
            throw MerkError("Cannot force " + nodeTypeToString(t) + " to String");
    }
}


template<>
long DynamicNode::forceTo<long>(const VariantType& v, CoerceMode mode) {
    NodeValueType t = getTypeFromValue(v);
    if (std::holds_alternative<long>(v)) return std::get<long>(v);
    if (std::holds_alternative<int>(v))  return (long)std::get<int>(v);
    if (std::holds_alternative<float>(v))  return (long)std::get<float>(v);

    switch (t) {
        case NodeValueType::Bool: return std::get<bool>(v) ? 1L : 0L;
        case NodeValueType::Char: return (long)std::get<char>(v);

        case NodeValueType::Double:
        case NodeValueType::Float:
            if (mode == CoerceMode::Strict) fail(t, "float/double->long not allowed");
            // You may also want range checks here.
            if (t == NodeValueType::Double) return (long)std::get<double>(v);
            return (long)std::get<float>(v);

        case NodeValueType::String:
        case NodeValueType::Text: {
            auto sv = tryStringView(v);
            if (!sv) fail(t, "text storage not accessible");
            return parseExact<long>(*sv, "long");
        }

        default:
            fail(t);
    }
}

template<>
double DynamicNode::forceTo<double>(const VariantType& v, CoerceMode mode) {
    (void)mode; // usually numeric->double is always OK; you can still use mode if you want to ban string parsing in Strict

    NodeValueType t = getTypeFromValue(v);

    if (std::holds_alternative<double>(v)) return std::get<double>(v);

    switch (t) {
        case NodeValueType::Float: return (double)std::get<float>(v);
        case NodeValueType::Int:   return (double)std::get<int>(v);
        case NodeValueType::Long:  return (double)std::get<long>(v);
        case NodeValueType::Char:  return (double)std::get<char>(v);
        case NodeValueType::Bool:  return std::get<bool>(v) ? 1.0 : 0.0;

        case NodeValueType::String:
        case NodeValueType::Text: {
            auto sv = tryStringView(v);
            if (!sv) fail(t, "text storage not accessible");
            // optional: in Strict mode you could refuse string parsing:
            // if (mode == CoerceMode::Strict) fail("string->double not allowed");
            return parseExact<double>(*sv, "double");
        }

        default:
            fail(t);
    }
}

template<>
float DynamicNode::forceTo<float>(const VariantType& v, CoerceMode mode) {
    NodeValueType t = getTypeFromValue(v);
    

    if (std::holds_alternative<float>(v)) return std::get<float>(v);

    switch (t) {
        case NodeValueType::Double: {
            double d = std::get<double>(v);
            if (mode == CoerceMode::Strict) fail(t, "double->float not allowed");
            // optional range check
            if (d < -std::numeric_limits<float>::max() || d > std::numeric_limits<float>::max())
                fail(t, "out of float range");
            return (float)d;
        }

        case NodeValueType::Int:  return (float)std::get<int>(v);
        case NodeValueType::Long: return (float)std::get<long>(v);
        case NodeValueType::Char: return (float)std::get<char>(v);
        case NodeValueType::Bool: return std::get<bool>(v) ? 1.0f : 0.0f;

        case NodeValueType::String:
        case NodeValueType::Text: {
            auto sv = tryStringView(v);
            if (!sv) fail(t, "text storage not accessible");
            return parseExact<float>(*sv, "float");
        }

        default:
            fail(t);
    }
}

template<>
char DynamicNode::forceTo<char>(const VariantType& v, CoerceMode mode) {
    NodeValueType t = getTypeFromValue(v);


    if (std::holds_alternative<char>(v)) return std::get<char>(v);

    switch (t) {
        case NodeValueType::Int:
            if (mode == CoerceMode::Strict) fail(t, "int->char not allowed");
            return (char)std::get<int>(v);

        case NodeValueType::String:
        case NodeValueType::Text: {
            auto sv = tryStringView(v);
            if (!sv) fail(t, "text storage not accessible");
            if (sv->size() == 1) {return (*sv)[0];}
            // optionally support quoted char literals:  '\''x'\''
            if (sv->size() == 3 && (*sv)[0]=='\'' && (*sv)[2]=='\'') return (*sv)[1];
            fail(t, "string is not a single char");
        }

        default:
            fail(t);
    }
}


String DynamicNode::forceToString(VariantType v) { return forceTo<String>(v); }
const char* DynamicNode::forceToCString(const VariantType& v, String& scratch) {
    scratch = forceTo<String>(v);
    return scratch.c_str();
}



NodeValueType DynamicNode::inferTypeFromString (String& valueStr, String& typeStr) {
    if (typeStr == "Variable" || typeStr == "Argument" || typeStr == "FunctionCall" || typeStr == "ClassMethodCall" || typeStr == "Parameter") {
        return NodeValueType::String;
    } 

    if (typeStr == "Number") {
        const bool looksFloat =
        valueStr.find('.') != String::npos ||
        valueStr.find('e') != String::npos ||
        valueStr.find('E') != String::npos;

        if (looksFloat) {
            // exact parse as double first
            double d = parseExact<double>(std::string_view(valueStr), "double");

            // decide Float vs Double
            if (d >= -std::numeric_limits<float>::max() &&
                d <=  std::numeric_limits<float>::max()) {
                return NodeValueType::Float;
            }
            return NodeValueType::Double;
        }

        // integer syntax
        long l = parseExact<long>(std::string_view(valueStr), "long");

        if (l >= std::numeric_limits<int>::min() &&
            l <= std::numeric_limits<int>::max()) {
            return NodeValueType::Int;
        }
        return NodeValueType::Long;
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

        // DEBUG_LOG(LogLevel::DEBUGC, "Set initial value with String. Type: ", static_cast<int>(data.type), ", Value: ", data.value);
}

NodeValueType DynamicNode::getNodeTypeFromString(String& type) {
    DEBUG_FLOW(FlowLevel::MED);

    auto nodeType = stringToNodeType(type);
    if (nodeType == NodeValueType::UNKNOWN) {
        throw MerkError("Cannot Have An UNKNOWN TYPE Passed Back");
    }
    DEBUG_FLOW_EXIT();
    return nodeType;
}

std::pair<VariantType, NodeValueType> DynamicNode::getCoercedStringAndType(const String& value, String& typeStr) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (value == "Variable" || value == "Argument" || value == "FunctionCall" || value == "ClassMethodCall" || value == "Parameter") {
        DEBUG_FLOW_EXIT();
        return {value, NodeValueType::String};
    }

    if (value == "null" && typeStr == "String") {
        return {value, NodeValueType::Null};
    }

    

    NodeValueType type = getNodeTypeFromString(typeStr);

    if (typeStr == "Number") { if (type != NodeValueType::Number) {throw MerkError("Not A Number");} }
    
    try {
        switch (type) {
            case NodeValueType::Number: {
                auto sv = std::string_view(value);
                const bool looksFloat =
                    value.find('.') != String::npos ||
                    value.find('e') != String::npos ||
                    value.find('E') != String::npos;
                
                if (looksFloat) {
                    double d = parseExact<double>(sv, "double");
                    if (d >= -std::numeric_limits<float>::max() &&
                        d <=  std::numeric_limits<float>::max()) {
                        return { static_cast<float>(d), NodeValueType::Float };
                    }
                    return { d, NodeValueType::Double };
                }

                long l = parseExact<long>(sv, "long");
                if (l >= std::numeric_limits<int>::min() &&
                    l <= std::numeric_limits<int>::max()) {
                    return { static_cast<int>(l), NodeValueType::Int };
                }
                return { l, NodeValueType::Long };
            }
            
            case NodeValueType::String: return { value, type };
            case NodeValueType::Text: return { value, type };
            case NodeValueType::Int: return { parseExact<int>(value, "int"), type};
            case NodeValueType::Float: return { parseExact<float>(value, "float"), type };
            case NodeValueType::Double: return { parseExact<double>(value, "double"), type };
            case NodeValueType::Long: return { parseExact<long>(value, "long"), type };
            case NodeValueType::Bool:
                if (value == "true" || value == "1") return {true, type};
                if (value == "false" || value == "0") return { false, type };
                throw MerkError("Invalid boolean: " + value);
            case NodeValueType::Char: return { parseExact<char>(value, "char"), type };
            
            default:
                throw MerkError("Unsupported coercion target type: " + nodeTypeToString(type));
        }
    } catch (const std::exception& e) {
        throw MerkError("Failed to coerce value '" + value + "' to type " + nodeTypeToString(type));
    }
}

std::pair<VariantType, NodeValueType> DynamicNode::validateAndCopy(const VariantType& value, NodeValueType type) {
    auto visitor = [&](auto&& arg) -> std::pair<VariantType, NodeValueType> {
        using T = std::decay_t<decltype(arg)>;
        if (getNodeTypeFromType<T>() != type) {
            throw MerkError("Value does not match declared NodeValueType: expected " + nodeTypeToString(type));
        }
        return { arg, type };
    };
    return std::visit(visitor, value);
}

NodeValueType DynamicNode::getTypeFromValue(const VariantType& value) {
    DEBUG_FLOW(FlowLevel::NONE);
    auto val = std::visit([](auto&& arg) -> NodeValueType {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, NumberVariant>) {
            return std::visit([](auto&& num) {
                using N = std::decay_t<decltype(num)>;
                return getNodeTypeFromType<N>();
            }, arg);
        }
        else if constexpr (std::is_same_v<T, TextVariant>) {
            return std::visit([](auto&& txt) {
                using S = std::decay_t<decltype(txt)>;
                return getNodeTypeFromType<S>();
            }, arg);
        }
        else {
            return getNodeTypeFromType<T>();
            
        }
    }, value);

    DEBUG_LOG(LogLevel::TRACE, "GOT TYPE " + nodeTypeToString(val) + " In DynamicNode::getTypeFromValue");
    // if (val == NodeValueType::Null) {throw MerkError(" DynamicNode::getTypeFromValue returned NULL");}
    if (val == NodeValueType::DataStructure || val == NodeValueType::Dict) {
        throw MerkError("Got " + nodeTypeToString(val) + " In DynamicNode::getTypeFromValue");
    }
    DEBUG_FLOW_EXIT();
    if (val == NodeValueType::DataStructure || val == NodeValueType::Dict || val == NodeValueType::List) {throw MerkError("DynamicNode::getTypeFromValue -> DataStructure Found");}
    // if (val == NodeValueType::Callable) { throw MerkError("DynamicNode::getTypeFromValue -> Callable"); }
    return val;
}


template <>
int DynamicNode::forceTo(const Node& node, CoerceMode mode) {
    if (node.isNumeric()) { return node.toInt(); }
    return forceTo<int>(node.getValue(), mode);
}

template <>
String DynamicNode::forceTo(const Node& node, CoerceMode mode) {
    if (node.isString()) { return node.toString(); }
    return forceTo<String>(node.getValue(), mode);
}

template <>
float DynamicNode::forceTo(const Node& node, CoerceMode mode) {
    if (node.isFloat()) { return node.toFloat(); }
    return forceTo<float>(node.getValue(), mode);
}

template <>
long DynamicNode::forceTo(const Node& node, CoerceMode mode) {
    if (node.isDouble()) { return node.toDouble(); }
    return forceTo<long>(node.getValue(), mode);
}

template <>
double DynamicNode::forceTo(const Node& node, CoerceMode mode) {
    if (node.isDouble()) { return node.toDouble(); }
    return forceTo<double>(node.getValue(), mode);
}

template <>
char DynamicNode::forceTo(const Node& node, CoerceMode mode) {
    return forceTo<char>(node.getValue(), mode);
}

template <>
char* DynamicNode::forceTo(const Node& node, CoerceMode mode) {
    (void)mode;
    if (node.isChars()) { return node.toChars(); }
    String out;
    auto val = forceToCString(node.getValue(), out);
    return const_cast<char*>(val);
}

float DynamicNode::forceToFloat(VariantType value) {
    return forceTo<float>(value);
    throw MerkError("Not Implemented: forceToFloat");
}
double DynamicNode::forceToDouble(VariantType value) {
    return forceTo<double>(value);
    throw MerkError("Not Implemented: forceToDouble");

}
char* DynamicNode::forceToChar(VariantType value) {
    (void)value;
    // return forceTo<char*>(value);
    throw MerkError("Not Implemented: forceToChar");

}
bool DynamicNode::forceToBool(VariantType value) {
    auto t = DynamicNode::getTypeFromValue(value);

    switch (t)
    {
    case NodeValueType::Bool:{
        auto v = get<bool>(value);
        return v;    
    }
    case NodeValueType::Null: return false;
    
    default:
        throw MerkError("Not Implemented type: " + nodeTypeToString(t) + " -> forceToBool");
        return false;
    }
}

int DynamicNode::forceToInt(VariantType value) {
    if (std::holds_alternative<int>(value)) {return std::get<int>(value);}
    NodeValueType type = getTypeFromValue(value);
    switch (type)
    {
    case NodeValueType::Float: return (int)get<float>(value);
    case NodeValueType::Long: return (int)get<long>(value);
    case NodeValueType::Char: return (int)get<char>(value);
    case NodeValueType::Number: return (int)get<long>(value);
    case NodeValueType::Double: return (int)get<double>(value);
    case NodeValueType::Bool: return get<bool>(value) ? 1 : 0;
    case NodeValueType::String:
    case NodeValueType::Text:
        {   
            if (std::holds_alternative<String>(value)) {
                auto s = std::get<String>(value);
                String val;
                int value{};
                auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);

                if (ec != std::errc{} || ptr != s.data() + s.size()) {
                    fail(type, "as it is not an integer");
                }
                return value;
            }
            if (std::holds_alternative<char>(value)) {
                return (int)std::get<char>(value);
            }


            fail(type, "No Checks...checked out");
        }
    case NodeValueType::Null: throw MerkError("Output will be null in DynamicNode::forceToString"); return 0;
    default:
        fail(type);
    }
}



float DynamicNode::forceToFloat(Node& node) {
    if (node.isNumeric()) {return node.toFloat();}
    throw MerkError("Not Implemented: forceToFloat");
}

double DynamicNode::forceToDouble(Node& node) {
    if (node.isDouble()) {return node.toDouble();}
    throw MerkError("Not Implemented: forceToDouble");

}
char* DynamicNode::forceToChar(Node& node) {
    (void)node;
    // need to change sig back to char instead of char*
    // if (node.isChars()) {return forceTo<char*>(node.getValue());}
    throw MerkError("Not Implemented: forceToChar");

}
bool DynamicNode::forceToBool(Node& node) {
    if (node.isBool()) {return node.toBool();}
    throw MerkError("Not Implemented: forceToBool");
}

int DynamicNode::forceToInt(Node& node) {
    if (node.isNumeric()) {return node.toInt();}
    return forceToInt(node.getValue());
}

String DynamicNode::forceToString(Node& node) {
    return node.toString();
}



SharedPtr<NodeBase> DynamicNode::dispatch(VariantType val, NodeValueType type, bool coerce) {
    (void)coerce;
    DEBUG_FLOW(FlowLevel::MED);

    if (type == NodeValueType::UNKNOWN) {
        if (std::holds_alternative<String>(val)) {
            auto actualVal = std::get<String>(val);
            if (actualVal == "Variable" || actualVal == "Argument" || actualVal == "FunctionCall" || actualVal == "ClassMethodCall" || actualVal == "Parameter") {
                return makeShared<StringNode>(actualVal);
            } else {throw MerkError("Type Unknwon Is Not Of the Exceptions");}
        } else {throw MerkError("Cannot Handle Type UNKNWON, valType: " + nodeTypeToString(DynamicNode::getTypeFromValue(val)));}
    }
    
    if (type == NodeValueType::Null) {
        return makeShared<NullNode>();
    }

    auto valType = getTypeFromValue(val);
    auto passedIsANumber = type == NodeValueType::Int || type == NodeValueType::Number || type == NodeValueType::Long || type == NodeValueType::Double || type == NodeValueType::Float;
    if (valType == type || (valType == NodeValueType::Number && passedIsANumber) || type == NodeValueType::ClassInstance) {
        switch (type)
        {
        case NodeValueType::Number:
            {
                switch (valType)
                {
                case NodeValueType::Int:
                    DEBUG_FLOW_EXIT();
                    return makeShared<IntNode>(std::get<int>(val));
                
                default:
                    throw MerkError("Cannot Cast " + nodeTypeToString(valType) + " to " + nodeTypeToString(type));
                }
            }
        case NodeValueType::Int:
            DEBUG_FLOW_EXIT();
            return makeShared<IntNode>(std::get<int>(val));
        case NodeValueType::String:
            DEBUG_FLOW_EXIT();
            if (!std::holds_alternative<String>(val)) {
                return makeShared<StringNode>(forceToString(val));
            }
            return makeShared<StringNode>(std::get<String>(val));

        case NodeValueType::Bool:
            DEBUG_FLOW_EXIT();
            return makeShared<BoolNode>(std::get<bool>(val));
        case NodeValueType::Double:
            DEBUG_FLOW_EXIT();
            return makeShared<DoubleNode>(std::get<double>(val));
        case NodeValueType::Float:
            DEBUG_FLOW_EXIT();
            return makeShared<FloatNode>(std::get<float>(val));
        case NodeValueType::DataStructure:
        case NodeValueType::Dict:
            throw MerkError("DynamicNode::dispatch found Dict | DataStructure");
        case NodeValueType::Callable:
            {
                auto call = std::get<SharedPtr<Callable>>(val);
                // return makeShared<CallableNode>(call);
                throw MerkError("Tried to create Callable " + call->toString());
            }
            
        // case NodeValueType::DataStructure:
        //     {
        //         DEBUG_FLOW_EXIT();
        //         auto data = std::get<SharedPtr<DataStructure>>(val);
        //         return makeShared<DynamicNode>(std::get<SharedPtr<DataStructure>>(val));
        //         // if (data->getType() == NodeValueType::List) {
        //         //     throw MerkError("Got A List");
        //         // }

        //         // throw MerkError("Found Another DataStructure, Need to disambiguate it");
        //     }
            

        
        default:
            DEBUG_FLOW_EXIT();
            throw MerkError("Not A Currently Supported Type: " + nodeTypeToString(type));
        }
    }

    // else if (coerce) {
    //     switch (valType)
    //     {
    //     case NodeValueType::String:
    //         if (type == NodeValueType::Number) {}
    //         /* code */
    //         break;
        
    //     default:
    //         break;
    //     }
    // }



    DEBUG_FLOW_EXIT();
    throw MerkError("Not A supported Type " + nodeTypeToString(valType) + ". Tried to dispatch to: " + nodeTypeToString(type));
}

Node DynamicNode::dispatchNode(VariantType value, String typeStr, bool coerce) {
    DEBUG_FLOW(FlowLevel::MED);

    auto nodeType = DynamicNode::getNodeTypeFromString(typeStr);
    if (nodeType == NodeValueType::UNKNOWN) {throw MerkError("The Type of " + nodeTypeToString(nodeType, false) + " for Starting Lit Value Is: " + DynamicNode::forceToString(value));}
    auto nodeBase = DynamicNode::dispatch(value, nodeType, coerce);
    auto node = Node(nodeBase);
    DEBUG_FLOW_EXIT();
    return node;
}

VariantType DynamicNode::getValue() const { return value; }

void DynamicNode::setValue(const VariantType& v)  {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    auto type = DynamicNode::getTypeFromValue(v);
    value = v;
    flags.type = type;
    flags.fullType.setBaseType(nodeTypeToString(type, false));
    DEBUG_FLOW_EXIT();
}

NodeValueType DynamicNode::getType() const { return NodeValueType::Any; }

void DynamicNode::validateMutability(const NodeBase& value) {
    if (!value.flags.isMutable) {
        throw MerkError("Cannot mutate immutable value");
    }
}

void DynamicNode::validateMutability(const Node& value) {
    if (!value.getFlags().isMutable) {
        throw MerkError("Cannot mutate immutable value");
    }
}

SharedPtr<NodeBase> DynamicNode::clone() const {
    // auto val = getValue();
    // if (std::holds_alternative<SharedPtr<Callable>>(val)) {
    //     auto callable = std::get<SharedPtr<Callable>>(val);
    //     auto node = this->getValue();
    //     switch (DynamicNode::getTypeFromValue(callable)) {
    //         case NodeValueType::ClassInstance:
    //             return std::static_pointer_cast<ClassInstance>(callable)->clone();
    //         default:
    //             return makeShared<DynamicNode>(*this);
    //     }
    // } else if (std::holds_alternative<SharedPtr<NativeNode>>(val)) {
    //     throw MerkError("Cloning a DataStructure");
    // } else {
    //     auto node = makeShared<DynamicNode>(*this); 
    //     node->flags = flags;
    //     return node;
    // }

    auto node = makeShared<DynamicNode>(*this); 
    node->flags = flags;
    return node;

    throw MerkError("Calling Clone on DynamicNode holding " + nodeTypeToString(DynamicNode::getTypeFromValue(getValue())));
    
}
 
int DynamicNode::toInt() const {  
    if (isInt()) {
        return std::get<int>(value);
    } else if (isBool()) {
        return toBool();
    } else if (isNumeric()) {
        // when other types are added
        throw MerkError("Need To Handle Numeric Types in DynamicNode::toInt()");
    }

    throw MerkError("Cannnot cast " + nodeTypeToString(getType()) + " to " + "Int");
}
bool DynamicNode::isString() const {return std::holds_alternative<String>(value);}
bool DynamicNode::isBool() const {return std::holds_alternative<bool>(value);}
bool DynamicNode::isValid() const { return DynamicNode::getTypeFromValue(value) == getNodeType(); }
String DynamicNode::toString() const {
    switch (DynamicNode::getTypeFromValue(value))
    {
    case NodeValueType::ClassInstance:
    case NodeValueType::Callable:
        return std::static_pointer_cast<ClassInstance>(std::get<SharedPtr<Callable>>(value))->toString();    
    default:
        return DynamicNode::forceToString(value); 
    }
    
}
bool DynamicNode::toBool() const { 
    NodeValueType ofThis = DynamicNode::getTypeFromValue(value);
    auto node = DynamicNode::dispatchNode(value, nodeTypeToString(ofThis, false));
    return node.toBool();
}
bool DynamicNode::isNumeric() const { return isInt() || isFloat() || isDouble();}
bool DynamicNode::isInt() const {return std::holds_alternative<int>(value);}
