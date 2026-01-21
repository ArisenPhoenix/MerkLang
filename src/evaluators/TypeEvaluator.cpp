#include "core/evaluators/TypeEvaluator.hpp"
#include "core/callables/Callable.hpp"
#include "utilities/debugger.h"   


[[noreturn]] static void fail(NodeValueType t, const String& msg = "") {
    throw MerkError("Cannot force " + nodeTypeToString(t) + " to Float" + (msg.empty()? "" : (": "+msg)));
}


namespace TypeEvaluator {

template <typename T>
T parseExact(std::string_view s, const char* what) {
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

std::optional<std::string_view> tryStringView(const VariantType& v) {
    if (std::holds_alternative<String>(v)) {
        const auto& s = std::get<String>(v);
        return std::string_view(s.data(), s.size());
    }
    // If you have TextVariant nested, handle it here too.
    // else if constexpr (...) { ... }
    return std::nullopt;
}

template <>
int to<int>(const VariantType& v, CoerceMode mode) {
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
int as<int>(const VariantType& v) {
    return to<int>(v, CoerceMode::Strict);
}

template <>
String to<String>(const VariantType& v, CoerceMode mode) {
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

template <>
String as<String>(const VariantType& v) {
    return to<String>(v, CoerceMode::Permissive);
}

template <>
bool to<bool>(const VariantType& v, CoerceMode mode) {
    if (std::holds_alternative<bool>(v)) {return std::get<bool>(v);}

    NodeValueType t = getTypeFromValue(v);

    switch (t) {
        case NodeValueType::Char:   {return (bool)std::get<char>(v);}

        case NodeValueType::Float:
            if (mode == CoerceMode::Strict) fail(t, "float->int not allowed");
            return (bool)std::get<float>(v);

        case NodeValueType::Double:
            if (mode == CoerceMode::Strict) fail(t, "double->int not allowed");
            return (bool)std::get<double>(v);

        case NodeValueType::Long:
            if (mode == CoerceMode::Strict) fail(t, "long->int not allowed");
            return (bool)std::get<long>(v);

        case NodeValueType::String:
        case NodeValueType::Text: {
            // normalize extraction in ONE place
            if (std::holds_alternative<String>(v)) {
                const auto& s = std::get<String>(v);
                return s.size() > 0;
            }
            fail(t, "text storage not bool");
        }

        default:
            fail(t);
    }
}



template <>
bool as<bool>(const VariantType& v) {
    return to<bool>(v, CoerceMode::Permissive);
}


template<>
long to<long>(const VariantType& v, CoerceMode mode) {
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
long as<long>(const VariantType& v) {
    return to<long>(v, CoerceMode::Permissive);
}

template<>
double to<double>(const VariantType& v, CoerceMode mode) {
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
double as<double>(const VariantType& v) {
    return to<double>(v, CoerceMode::Permissive);
}

template<>
float to<float>(const VariantType& v, CoerceMode mode) {
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
char to<char>(const VariantType& v, CoerceMode mode) {
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

template<>
char as<char>(const VariantType& v) {
    return to<char>(v, CoerceMode::Permissive);
}


NodeValueType getTypeFromValue(const VariantType& value) {
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

    DEBUG_LOG(LogLevel::TRACE, "GOT TYPE " + nodeTypeToString(val) + " In TypeEvaluator::getTypeFromValue");
    // if (val == NodeValueType::Null) {throw MerkError(" TypeEvaluator::getTypeFromValue returned NULL");}
    if (val == NodeValueType::DataStructure || val == NodeValueType::Dict) {
        throw MerkError("Got " + nodeTypeToString(val) + " In TypeEvaluator::getTypeFromValue");
    }
    DEBUG_FLOW_EXIT();
    if (val == NodeValueType::DataStructure || val == NodeValueType::Dict || val == NodeValueType::List) {throw MerkError("TypeEvaluator::getTypeFromValue -> DataStructure Found");}
    // if (val == NodeValueType::Callable) { throw MerkError("TypeEvaluator::getTypeFromValue -> Callable"); }
    return val;
}

NodeValueType inferTypeFromString (String& valueStr, String& typeStr) {
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

NodeValueType getNodeTypeFromString(String& type) {
    DEBUG_FLOW(FlowLevel::MED);

    auto nodeType = stringToNodeType(type);
    if (nodeType == NodeValueType::UNKNOWN) {
        throw MerkError("Cannot Have An UNKNOWN TYPE Passed Back");
    }
    DEBUG_FLOW_EXIT();
    return nodeType;
}

std::pair<VariantType, NodeValueType> getCoercedStringAndType(const String& value, String& typeStr) {
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



}