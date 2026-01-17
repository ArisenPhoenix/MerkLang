#pragma once

#include "core/TypesFWD.hpp"
#include <variant>
#include <iostream>
#include <sstream>
#include <any>
#include <functional>



// This is essentially a global types repository used throughout the whole codebase

// Forward declarations


namespace std { template<> struct hash<Node>; }



template <typename F>
using Funciton = std::function<F>;

struct NullType {
    friend std::ostream& operator<<(std::ostream& os, const NullType&) {
        os << "null";
        return os;
    }

    std::size_t hash() const {
        return 0xDEADBEEF;
    }
};

constexpr NullType Null{};
constexpr bool operator==(const NullType&, const NullType&) {return true;}

struct UninitializedType {
    String toString() const { return "Uninitialized"; }
    constexpr bool operator==(const UninitializedType&) const { return true; }
    std::size_t hash() const {
        return 0xBAADF00D;
    }
};
 

using TextVariant = std::variant< String, char >;
using NumberVariant = std::variant< int, float, double, long >;



using VariantType = std::variant<
    int,
    float,
    double,
    long,
    char,
    char*,
    bool,
    String,
    NullType,
    UninitializedType,


    // Held For CallableRefs
    Vector<SharedPtr<CallableSignature>>,

    // Held Data for Nodes on classes
    SharedPtr<Callable>,

    // For general handling
    // What Data Structure Holds
    Vector<Node>,
    std::unordered_map<Node, Node>,
    std::map<Node, Node>,
    std::unordered_set<Node>,
    SharedPtr<NativeNode>
>;


// Function to map types in VariantType to NodeValueType
template <typename T>
constexpr NodeValueType getNodeTypeFromType() {
    if constexpr (std::is_same_v<T, char>) return NodeValueType::Char;
    // else if constexpr (std::is_same_v<T, NumberVariant>) return NodeValueType::Number;
    // else if constexpr (std::is_same_v<T, TextVariant>) return NodeValueType::Text;

    else if constexpr (std::is_same_v<T, int>) return NodeValueType::Int;
    else if constexpr (std::is_same_v<T, float>) return NodeValueType::Float;
    else if constexpr (std::is_same_v<T, double>) return NodeValueType::Double;
    else if constexpr (std::is_same_v<T, long>) return NodeValueType::Long;
    else if constexpr (std::is_same_v<T, bool>) return NodeValueType::Bool;
    else if constexpr (std::is_same_v<T, String>) return NodeValueType::String;
    else if constexpr (std::is_same_v<T, UninitializedType>) return NodeValueType::Uninitialized;
    else if constexpr (std::is_same_v<T, std::nullptr_t>) return NodeValueType::Null;
    else if constexpr (std::is_same_v<T, NullType>) return NodeValueType::Null;
    
    else if constexpr (std::is_same_v<T, SharedPtr<Callable>> ) return NodeValueType::Callable;
    // else if constexpr (std::is_same_v<T, SharedPtr<DataStructure>>) return NodeValueType::DataStructure;
    else if constexpr (std::is_same_v<T, Vector<SharedPtr<CallableSignature>>>) return NodeValueType::CallableSignature;

    else if constexpr (std::is_same_v<T, Vector<Node>>) return NodeValueType::Vector;
    
    else if constexpr (std::is_same_v<T, NodeMapO>) return NodeValueType::NativeOMap;
    else if constexpr (std::is_same_v<T, NodeMapU>) return NodeValueType::NativeUMap;
    else if constexpr (std::is_same_v<T, NodeSetU>) return NodeValueType::NativeUSet;
    else if constexpr (std::is_same_v<T, NativeNode>) return NodeValueType::NativeNode;
    else return NodeValueType::Any;


    // else if constexpr (std::is_same_v<T, NodeValueType::Any>)
    //     return NodeValueType::Any;
    // else throw RunTimeError("Unsupported type in VariantType." + nodeTypeToString(T));
}


String nodeTypeToString(NodeValueType type, bool colored = true); 
NodeValueType stringToNodeType(String);
String astTypeToString(AstType type);
String tokenTypeToString(TokenType type, bool colored = false);
String getTokenDescription(TokenType type);


struct Colors {
    inline static bool enabled = true;
    static constexpr const char* reset = "\033[0m";
  
    // Standard Colors
    static constexpr const char* red = "\033[31m";
    static constexpr const char* green = "\033[32m";
    static constexpr const char* yellow = "\033[33m";
    static constexpr const char* blue = "\033[34m";
    static constexpr const char* purple = "\033[35m";
    static constexpr const char* cyan = "\033[36m";
    static constexpr const char* white = "\033[97m";
    static constexpr const char* black = "\033[30m";

    // Bold Variants
    static constexpr const char* bold_red = "\033[1;31m";
    static constexpr const char* bold_green = "\033[1;32m";
    static constexpr const char* bold_yellow = "\033[1;33m";
    static constexpr const char* bold_blue = "\033[1;34m";
    static constexpr const char* bold_purple = "\033[1;35m";
    static constexpr const char* bold_cyan = "\033[1;36m";
    static constexpr const char* bold_white = "\033[1;97m";

    // 256-Color Approximations
    static constexpr const char* orange = "\033[38;5;214m"; // 256-color orange
    static constexpr const char* pink = "\033[38;5;13m";    // 256-color pink
    static constexpr const char* light_blue = "\033[38;5;45m"; // Sky Blue
    static constexpr const char* teal = "\033[38;5;37m"; // Teal
    static constexpr const char* light_gray = "\033[38;5;250m"; // Light Gray
    static constexpr const char* dark_gray = "\033[38;5;240m"; // Dark Gray
    static constexpr const char* brown = "\033[38;5;130m"; // Brown

     // Background Colors
     static constexpr const char* bg_red = "\033[41m";
     static constexpr const char* bg_green = "\033[42m";
     static constexpr const char* bg_yellow = "\033[43m";
     static constexpr const char* bg_blue = "\033[44m";
     static constexpr const char* bg_magenta = "\033[45m";
     static constexpr const char* bg_cyan = "\033[46m";
     static constexpr const char* bg_white = "\033[47m";
     static constexpr const char* bg_black = "\033[40m";
 
     // Bright Backgrounds
     static constexpr const char* bg_bright_red = "\033[101m";
     static constexpr const char* bg_bright_green = "\033[102m";
     static constexpr const char* bg_bright_yellow = "\033[103m";
     static constexpr const char* bg_bright_blue = "\033[104m";
     static constexpr const char* bg_bright_magenta = "\033[105m";
     static constexpr const char* bg_bright_cyan = "\033[106m";
     static constexpr const char* bg_bright_white = "\033[107m";


     static void setEnabled(bool flag) {
        enabled = flag;
    }
};



// General-purpose color function
inline String highlight(const String& text, const char* color) {
    return Colors::enabled ? String(color) + text + Colors::reset : text;
}

// Highlight a single character in a string
inline String highlightText(
    const String& text, size_t highlightIndex, const char* highlightColor = Colors::red) {
    if (highlightIndex >= text.size()) {
        throw std::out_of_range("Highlight index is out of range for the provided text.");
    }

    // Slice the text and insert highlight
    return text.substr(0, highlightIndex) + highlight(String(1, text[highlightIndex]), highlightColor) +
           text.substr(highlightIndex);
}

// Highlight a token in a context string - for simplicity
inline String highlightToken(const String& context, const String& token, const char* color = Colors::red) {
    // Find the position of the token in the context
    size_t pos = context.find(token);
    if (pos == String::npos) {
        return context; // Return the context unaltered if the token isn't found
    }

    // Slice the context and insert highlight
    return context.substr(0, pos) + highlight(token, color) + context.substr(pos + token.length());
}




struct Token {
    TokenType type;
    String value;
    int line;
    int column;

    Token(TokenType t, const String& v, int l, int c)
        : type(t), value(v), line(l), column(c) {}

    String toString() const {
        return "Token(Type: " + tokenTypeToString(type) +
           ", Value: " + value +
           ", Line: " + std::to_string(line) +
           ", Column: " + std::to_string(column) +
           ", Description: " + getTokenDescription(type) + ")";
    }



    String toColoredString() const {
        return highlight("Token", Colors::bold_green) + "(Type: " + tokenTypeToString(type, true) +
           ", Value: " + value +
           ", Line: " + std::to_string(line) +
           ", Column: " + std::to_string(column) +
           ", Description: " + getTokenDescription(type) + ")";
    }

    String typeAsString() const {return tokenTypeToString(type);}
}; 


String callableTypeAsString(CallableType callableType);




namespace std {
    template<>
    struct hash<NullType> {
        std::size_t operator()(const NullType&) const noexcept {
            // Constant hash for all NullType values
            return 0x9e3779b9; // fixed random prime
        }
    };

    template<>
    struct hash<UninitializedType> {
        std::size_t operator()(const UninitializedType&) const noexcept {
            // Constant hash for all UninitializedType values
            return 0x85ebca6b; // another fixed random prime
        }
    };
}

