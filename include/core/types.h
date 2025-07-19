#ifndef TYPES_H
#define TYPES_H

#include <variant>
#include <string>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <memory>
#include <vector>
#include <any>
#include <map>
#include <functional>
#include <unordered_set>



// This is essentially a global types repository used throughout the whole codebase

// Forward declarations
class Node;
class Function;
class FunctionNode;
class Callable;

class ClassBase;
class ClassNode;
class CallableNode;
class Method;
class MethodNode;
class InstanceNode;
class ClassInstance;
class Scope;

// Aliases
using String = std::string;
template <typename T>
using UniquePtr = std::unique_ptr<T>;

template <typename T2>
using SharedPtr = std::shared_ptr<T2>;

template <typename T3>
using WeakPtr = std::weak_ptr<T3>;

template <typename V>
using Vector = std::vector<V>;

using RunTimeError = std::runtime_error;

template <typename M>
using UnorderedMapNode = std::unordered_map<String, M>;

template <typename F>
using Funciton = std::function<F>;

template <typename T, typename... Args>
std::unique_ptr<T> makeUnique(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
std::shared_ptr<T> makeShared(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}



using DependencyGraph = std::unordered_map<String, std::unordered_set<String>>;

// Enum for Tokens and their types of the value held by the variant
enum class TokenType {
    Type,
    Keyword,
    Identifier,
    Number,
    String,
    Bool,

    VarDeclaration,
    VarAssignment,
    Variable,
    AccessorVariable,

    Operator,
    Punctuation,

    Indent,
    Dedent,
    Newline,
    EOF_Token,

    Break,
    Else,
    If,
    Elif,
    DoWhile,
    Case,
    Default,
    Continue,

    Function,
    FunctionDef,
    FunctionCall,
    FunctionRef,

    Parameter,
    Argument,

    ClassDef,
    ClassCall,
    ClassRef,

    ClassMethodDef,
    ClassMethodCall,
    ClassMethodRef,
    ClassAttribute,

    ChainEntryPoint,

    Unknown,
    LeftBracket,
    RightBracket,
    LiteralArrowLeft,
    LiteralArrowRight,
    BraceLeft,
    BraceRight,
    LeftArrow,
    RightArrow,
    BeginTyping
};

// Enum for the type of the value held by the variant
enum class NodeValueType {
    Number,
    Int,
    Float,
    Double,
    Long,
    Char,
    String,
    Vector,
    Bool,
    Shared_Vector,
    Function,
    Parameter,

    Class,
    Method,
    Callable,
    ClassInstance,
    Null,
    None,
    Uninitialized,
    Any,
    UNKNOWN,
    Scope,

    List,
    Array
};



// Actually refers to AST Types
enum class AstType {
    Base,
    AST,
    Literal,
    VariableDeclaration,
    VariableAssignment,
    VariableReference,
    ParameterAssignment,

    BinaryOperation,
    Block,
    Conditional,
    IfStatement,
    ElseStatement,
    ElifStatement,
    WhileLoop,
    CodeBlock,
    
    Break,
    Return,
    Continue,
    LoopBlock,

    CallableBody,
    CallableDefinition,
    CallableReference,
    CallableCall,

    Function,
    FunctionBlock,
    FunctionCall,
    NativeFunction,
    UserFunction,
    FunctionDefinition,
    FunctionReference,

    ClassDefinition,
    ClassCall,
    ClassReference,
    ClassBlock,

    ClassMethodBlock,
    ClassMethodDef,
    ClassMethodRef,
    ClassMethodCall,

    AttributeDeclaration,
    AttributeReference,
    AttributeAssignment,

    Chain,
    ChainOperation,
    Accessor,

    Unknown,
    NoOp,

    // Structures
    KeyValueStructure,
    Enum,

    ImportStatement
};


enum class IdentifierType {
    Variable,
    Function,
    Method,
    Class
};



struct NullType {
    friend std::ostream& operator<<(std::ostream& os, const NullType&) {
        os << "null";
        return os;
    }
};

constexpr NullType Null{};
constexpr bool operator==(const NullType&, const NullType&) {return true;}


struct UninitializedType {
    String toString() const { return "Uninitialized"; }
    constexpr bool operator==(const UninitializedType&) const { return true; }

};
 
using NodeList = Vector<Node>;
class ListNode;
class ArrayNode;


using VariantType = std::variant<
    int,
    float,
    double,
    long,
    char,
    bool,
    String,
    Vector<Node>,
    SharedPtr<Vector<Node>>,
    NullType,
    UninitializedType,
    SharedPtr<Function>,
    SharedPtr<ClassBase>,
    SharedPtr<Method>,
    SharedPtr<Callable>,
    SharedPtr<ClassInstance>,
    SharedPtr<Scope>,
    SharedPtr<ListNode>,
    SharedPtr<ArrayNode>

    // SharedPtr<FunctionNode> // Add this to support FunctionNode
>;


using ClassMembers = std::unordered_map<String, String>;


// Function to map types in VariantType to NodeValueType
template <typename T>
constexpr NodeValueType getNodeTypeFromType() {
    if constexpr (std::is_same_v<T, int>)
        return NodeValueType::Int;
    else if constexpr (std::is_same_v<T, float>)
        return NodeValueType::Float;
    else if constexpr (std::is_same_v<T, double>)
        return NodeValueType::Double;
    else if constexpr (std::is_same_v<T, long>)
        return NodeValueType::Long;
    else if constexpr (std::is_same_v<T, bool>)
        return NodeValueType::Bool;
    else if constexpr (std::is_same_v<T, String>)
        return NodeValueType::String;
    else if constexpr (std::is_same_v<T, SharedPtr<ListNode>>) 
        return NodeValueType::List;
    else if constexpr (std::is_same_v<T, SharedPtr<ArrayNode>>) 
        return NodeValueType::Array;
    else if constexpr (std::is_same_v<T, std::nullptr_t>)
        return NodeValueType::Null;
    else if constexpr (std::is_same_v<T, UninitializedType>)
        return NodeValueType::Uninitialized;
    else {return NodeValueType::Any;}
    // else if constexpr (std::is_same_v<T, NodeValueType::Any>)
    //     return NodeValueType::Any;
    // else
    //     throw RunTimeError("Unsupported type in VariantType.");
}


String nodeTypeToString(NodeValueType type); 

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


using NodeMap = std::map<String, Node>;                 // For named arguments in function calls
using NodeTypeMap = std::map<String, NodeValueType>;    // For named parameter types in function signatures


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


enum class CallableType {
    DEF,        // Can capture outside variables but does not modify them
    FUNCTION,   // Only has access to its parameters, no external variables and supports overloads
    LAMBDA,     // Future lambda type
    NATIVE,     // Placeholder in case there are native functions that are hardcoded in C++

    METHOD,
    CLASS,
    CALLABLE,
    INSTANCE
};


String callableTypeAsString(CallableType callableType);


String identifierTypeToString(IdentifierType identifierType);





class ResolvedType {
    String baseType;  // e.g. "Array", "List", "Schema", or "UserStruct"
    Vector<ResolvedType> inner;  // Nested for Array[Map[String, Int]]
public:
    ResolvedType(String baseType);
    ResolvedType(String baseType, Vector<ResolvedType> inner);
    String toString() const;
};


#endif // TYPES_H


