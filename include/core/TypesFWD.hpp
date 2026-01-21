#pragma once

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <span>

template <typename T, typename... Args>
std::unique_ptr<T> makeUnique(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
std::shared_ptr<T> makeShared(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}



// Aliases
using String = std::string;
using ClassMembers = std::unordered_map<String, String>;
using RunTimeError = std::runtime_error;

template <typename T>
using UniquePtr = std::unique_ptr<T>;

template <typename T2>
using SharedPtr = std::shared_ptr<T2>;

template <typename T3>
using WeakPtr = std::weak_ptr<T3>;

template <typename V>
using Vector = std::vector<V>;

template <typename M>
using UnorderedMapNode = std::unordered_map<String, M>;


class Node;
class VarNode;
class ParamNode;
class ParamList;
class ArgumentList;
class Arguments;

class Callable;
class CallableNode;
class CallableSignature;

class Function;
class UserFunction;
class FunctionNode;
class Method;
class MethodNode;

class ClassBase;
class ClassInstance;
class ClassNode;
class InstanceNode;
class ClassInstanceNode;
class ClassSignature;

class NameSpace;

class NativeNode;
class ListNode;
class ArrayNode;
class DictNode;
class MapNode;
class SetNode;

class Scope;
struct Token;

using NodeList = Vector<Node>;
using NodeMapU = std::unordered_map<Node, Node>;
using NodeMapO = std::map<Node, Node>;
using NodeSetU = std::unordered_set<Node>;
using DependencyGraph = std::unordered_map<String, std::unordered_set<String>>;
using ClassMembers = std::unordered_map<String, String>;
using ArgumentType = Arguments;


// Enum for the type of the value held by the variant
enum class NodeValueType {
    Number,
    Text,
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
    Dict,
    List,
    Array,
    Set,
    CallableSignature,
    DataStructure,
    NativeOMap,
    NativeUMap,
    NativeOSet,
    NativeUSet,
    UserDefined,

    NativeNode,
    Http,
    File,
};

using NodeTypeMap = std::map<String, NodeValueType>;    // For named parameter types in function signatures

// Enum for Tokens and their types of the value held by the variant
enum class TokenType {
    Type,
    Keyword,
    Identifier,
    Number,
    String,
    Char,
    Bool,
    Text,

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
    SOF_Token,

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
    BeginTyping,

    NoOp,
    Comment,
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
    UnaryOperation,
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
    ThrowStatement,
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

    Namespace,

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

    ImportStatement, 
    Argument,
    Arguments
};

enum class CallableType {
    DEF,        // Can capture outside variables but does not modify them
    FUNCTION,   // Only has access to its parameters, no external variables and supports overloads
    LAMBDA,     // Future lambda type
    NATIVE,     // native functions are hardcoded in C++

    METHOD,
    CLASS,
    CALLABLE,
    INSTANCE
};


using TypeSignatureId = uint32_t;
using TypeId = uint32_t;
static constexpr TypeSignatureId kInvalidTypeSignatureId = 0;
static constexpr TypeId kInvalidTypeId = 0;

class ResolvedType {
    String baseType;  // e.g. "Array", "List", "Schema", "etc"
    Vector<ResolvedType> inner;  // Nested for Array[Map[String, Int]]
public:
    ResolvedType();
    ResolvedType(String baseType);
    ResolvedType(String baseType, Vector<ResolvedType> inner);
    String toString() const;
    String toString();
    void setBaseType(String);
    void setInner(Vector<ResolvedType>);

    String getBaseType() {return baseType;}

    const String& getBaseType() const { return baseType; }
    const Vector<ResolvedType>& getInnerType() const { return inner; }

    bool matches(const ResolvedType& other) const;
    size_t hash() const;

};


class TypeRegistry;


struct DebugStringifyConfig {
    size_t maxTotal = 300;          // max chars per printed value
    size_t maxString = 220;         // max chars for a string field
    size_t maxContainer = 400;      // max chars for list/dict/set/etc.
    size_t maxFields = 32;          // max fields printed for instances
    bool showRemainder = true;      // ...(+N chars)
    bool alwaysEllipsis = true;    // if you want " ..." always
    String handle(String s);
};

// global accessors (single instance)
DebugStringifyConfig& debugStringifyConfig();


String nodeTypeToString(NodeValueType type, bool colored = true); 
NodeValueType stringToNodeType(String);
String astTypeToString(AstType type);
String tokenTypeToString(TokenType type, bool colored = false);
String getTokenDescription(TokenType type);


// Forward declaration Ast types
class ConditionalBlock;  
class BaseAST;
class ASTStatement;
class ElifStatement;
class ElseStatement;
class CodeBlock;
class IfStatement;
class FunctionBlock;
class ClassInstanceNode;
class CallableBody;
class MethodBody;
class ChainElement;
struct DataTypeFlags;