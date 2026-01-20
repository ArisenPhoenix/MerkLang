#include "core/types.h"
#include "core/node/Node.hpp"



String nodeTypeToString(NodeValueType type, bool colored) {
    switch (type) {
        case NodeValueType::String: return colored ? highlight("String", Colors::bold_green) : "String";
        case NodeValueType::Char: return colored ? highlight("Char", Colors::bold_green) : "Char";

        case NodeValueType::Number: return colored ? highlight("Number", Colors::bold_blue) : "Number";
        case NodeValueType::Int: return colored ? highlight("Int", Colors::bold_blue) : "Int";
        case NodeValueType::Bool: return colored ? highlight("Bool", Colors::cyan) : "Bool";
        case NodeValueType::Float: return colored ? highlight("Float", Colors::bold_blue) : "Float";
        case NodeValueType::Double: return colored ? highlight("Double", Colors::bold_blue) : "Double";
        case NodeValueType::Long: return colored ? highlight("Long", Colors::bold_blue) : "Long";
        
        case NodeValueType::Vector: return colored ? highlight("Vector", Colors::yellow) : "Vector";
        case NodeValueType::Shared_Vector: return colored ? highlight("Shared_Vector", Colors::yellow) : "Shared_Vector";

        case NodeValueType::Parameter: return colored ? highlight("Parameter", Colors::bold_white) : "Parameter";
        case NodeValueType::Class: return colored ? highlight("Class", Colors::bold_white) : "Class";

        case NodeValueType::Function: return colored ? highlight("Function", Colors::purple) : "Function";
        case NodeValueType::Method: return colored ? highlight("Method", Colors::purple) : "Method";

        case NodeValueType::Null: return colored ? highlight("Null", Colors::orange) : "Null";
        case NodeValueType::Any: return colored ? highlight("Any", Colors::green) : "Any";
        case NodeValueType::None: return colored ? highlight("None", Colors::orange) : "None";
        
        case NodeValueType::Uninitialized: return colored ? highlight("Uninitialized", Colors::red) : "Uninitialized";
        case NodeValueType::UNKNOWN: return colored ? highlight("UNKNOWN", Colors::bold_red) : "UNKNOWN";
        case NodeValueType::Callable: return colored ? highlight("Callable", Colors::red) : "Callable";
        case NodeValueType::ClassInstance: return colored ? highlight("ClassInstance", Colors::red) : "ClassInstance";
        case NodeValueType::List: return colored ? highlight("List", Colors::bold_white) : "List";
        case NodeValueType::Array: return colored ? highlight("Array", Colors::bold_white) : "Array";
        case NodeValueType::Dict: return colored ? highlight("Dict", Colors::bold_white) : "Dict";
        case NodeValueType::Set: return colored ? highlight("Set", Colors::bold_white) : "Set";
        case NodeValueType::CallableSignature: return colored ? highlight("CallableSignature", Colors::bg_magenta) : "CallableSignature";
        case NodeValueType::DataStructure: return colored ? highlight("DataStructure", Colors::red) : "DataStructure";
        case NodeValueType::NativeOMap: return colored ? highlight("NativeOMap", Colors::bold_red) : "NativeOMap";
        case NodeValueType::NativeUMap: return colored ? highlight("NativeUMap", Colors::bold_red) : "NativeUMap";
        case NodeValueType::NativeOSet: return colored ? highlight("NativeOSet", Colors::bold_red) : "NativeOSet";
        case NodeValueType::NativeUSet: return colored ? highlight("NativeUSet", Colors::bold_red) : "NativeUSet";
        case NodeValueType::UserDefined: return colored ? highlight("UserDefined", Colors::bold_green) : "UserDefined";
        case NodeValueType::Scope: return colored ? highlight("Scope", Colors::bold_green) : "Scope";
        case NodeValueType::Text: return colored ? highlight("Text", Colors::green) : "Text";
        case NodeValueType::File: return colored ? highlight("File", Colors::bg_cyan) : "File";
        case NodeValueType::Http: return colored ? highlight("Http", Colors::cyan) : "Http";
        // case NodeValueType::ClassInstance: 
        default: return "UNKNOWN";
        // default: throw RunTimeError("Unknown NodeValueType encountered in nodeTypeToString. ");
    }
}

NodeValueType stringToNodeType(String type) {
        if (type == "String") {return NodeValueType::String;}
        if (type == "Char") {return NodeValueType::Char;}
        if (type == "Number") {return NodeValueType::Number;}
        if (type == "Int") {return NodeValueType::Int;}
        if (type == "Bool") {return NodeValueType::Bool;}
        if (type == "Float") {return NodeValueType::Float;}
        if (type == "Double") {return NodeValueType::Double;}
        if (type == "Long") {return NodeValueType::Long;}
        if (type == "Parameter") {return NodeValueType::Parameter;}
        if (type == "Class") {return NodeValueType::Class;}
        if (type == "Function") {return NodeValueType::Function;}
        if (type == "Method") {return NodeValueType::Method;}
        if (type == "Null") {return NodeValueType::Null;}
        if (type == "Any") {return NodeValueType::Any;}
        if (type == "None") {return NodeValueType::None;}
        if (type == "Uninitialized") {return NodeValueType::Uninitialized;}
        if (type == "Callable") {return NodeValueType::Callable;}
        if (type == "ClassInstance") {return NodeValueType::ClassInstance;}
        if (type == "List") {return NodeValueType::List;}
        if (type == "Array") {return NodeValueType::Array;}
        if (type == "Dict") {return NodeValueType::Dict;}
        if (type == "Set") {return NodeValueType::Set;}
        if (type == "DataStructure") {return NodeValueType::DataStructure;}
        if (type == "CallableSignature") {return NodeValueType::CallableSignature;}
        if (type == "UNKNOWN") {throw RunTimeError("Cannot Be Passed An Explicit UNKNOWN TYPE");}
        if (type == "NativeOMap") {return NodeValueType::NativeOMap; }
        if (type == "NativeUMap") {return NodeValueType::NativeUMap; }
        if (type == "NativeOSet") {return NodeValueType::NativeOSet; }
        if (type == "NativeUSet") {return NodeValueType::NativeUSet; }
        if (type == "Chars") {return NodeValueType::Char;}
        
        return NodeValueType::UNKNOWN;
}

String astTypeToString(AstType type) {
    switch (type) {
        case AstType::Base: return highlight("Base - shouldn't be present", Colors::bold_red);
        case AstType::Literal: return highlight("Literal", Colors::bold_blue);
        case AstType::VariableDeclaration: return highlight("VariableDeclaration", Colors::bold_cyan);
        case AstType::VariableReference: return highlight("VariableReference", Colors::teal);
        case AstType::VariableAssignment: return highlight("VariableAssignment", Colors::bold_cyan);
        case AstType::ParameterAssignment: return highlight("ParameterAssignment", Colors::teal);

        case AstType::BinaryOperation: return highlight("BinaryOperation", Colors::bold_blue);
        case AstType::UnaryOperation: return highlight("UnaryOperation", Colors::bold_green);

        case AstType::Conditional: return highlight("Conditional", Colors::bold_green);
        case AstType::IfStatement: return highlight("IfStatement", Colors::yellow);
        case AstType::ElifStatement: return highlight("ElifStatement", Colors::yellow);
        case AstType::ElseStatement: return highlight("ElseStatement", Colors::yellow);
        case AstType::WhileLoop: return highlight("WhileLoop", Colors::bold_yellow);

        case AstType::Break: return highlight("Break", Colors::bold_red);
        case AstType::Return: return highlight("Return", Colors::bold_red);
        case AstType::Continue: return highlight("Continue", Colors::bold_red);
        case AstType::LoopBlock: return highlight("LoopNode", Colors::light_gray);

        case AstType::Block: return highlight("Block", Colors::bold_purple);
        case AstType::CodeBlock: return highlight("CodeBlock", Colors::bold_purple);

        case AstType::FunctionBlock: return highlight("FunctionBlock", Colors::bold_purple);
        case AstType::FunctionCall: return highlight("FunctionCall", Colors::bold_red);
        case AstType::FunctionDefinition: return highlight("FunctionDefinition", Colors::bold_red);
        case AstType::FunctionReference: return highlight("FunctionReference", Colors::bold_blue);
        case AstType::UserFunction: return highlight("UserFunction", Colors::bold_blue);
        case AstType::NativeFunction: return highlight("NativeFunction", Colors::bold_blue);


        case AstType::ClassBlock: return highlight("ClassBlock", Colors::bold_cyan);
        case AstType::ClassCall: return highlight("ClassCall", Colors::bold_red);
        case AstType::ClassDefinition: return highlight("ClassDefinition", Colors::bold_red);
        case AstType::ClassReference: return highlight("ClassReference", Colors::bold_blue);

        case AstType::ClassMethodBlock: return highlight("ClassMethodBody", Colors::bold_purple);
        case AstType::ClassMethodDef: return highlight("ClassMethodDef", Colors::bold_red);
        case AstType::ClassMethodCall: return highlight("ClassMethodCall", Colors::bold_red);
        

        case AstType::ClassMethodRef: return highlight("ClassMethodRef", Colors::bold_blue);

        case AstType::AttributeDeclaration: return highlight("AttributeDeclaration", Colors::bold_cyan);
        case AstType::AttributeAssignment: return highlight("AttributeAssignment", Colors::bold_cyan);
        case AstType::AttributeReference: return highlight("AttributeReference", Colors::teal);
        case AstType::Chain: return highlight("Chain", Colors::yellow);
        case AstType::ChainOperation: return highlight("ChainOperation", Colors::bg_green);
        case AstType::Accessor: return highlight("Accessor", Colors::red);

        case AstType::CallableBody: return highlight("CallableBody", Colors::bold_red);
        case AstType::CallableCall: return highlight("CallableCall", Colors::bold_red);
        case AstType::CallableDefinition: return highlight("CallableDefinition", Colors::bold_red);
        case AstType::CallableReference: return highlight("CallableReference", Colors::bold_red);
        case AstType::Function: return "Function";
        case AstType::NoOp: return highlight("NoOp", Colors::light_gray);

        case AstType::ImportStatement: return highlight("ImportStatement", Colors::light_blue);

        case AstType::AST: return highlight("AST", Colors::red);
        case AstType::Argument: return highlight("Argument", Colors::bg_cyan);
        case AstType::Arguments: return highlight("Arguments", Colors::bold_yellow);
        case AstType::Namespace: return highlight("Namespace", Colors::bold_blue);

        default: return "Unknown";
    }
}

String tokenTypeToString(TokenType type, bool colored) {
    switch (type) {
        case TokenType::Type: return colored ? highlight("Type", Colors::red) : "Type";
        case TokenType::Keyword: return colored ? highlight("Keyword", Colors::bold_white) : "Keyword";

        case TokenType::Identifier: return colored ? highlight("Identifier", Colors::bg_yellow) :  "Identifier";
        case TokenType::VarAssignment: return colored ? highlight("VarAssignment", Colors::bold_cyan) : "VarAssignment";
        case TokenType::VarDeclaration: return colored ? highlight("VarDeclaration", Colors::bold_cyan) : "VarDeclaration";
        case TokenType::Variable: return colored ? highlight("Variable", Colors::bold_cyan) : "Variable";

        case TokenType::Char: return colored ? highlight("Char", Colors::green) : "Char";
        case TokenType::String: return colored ? highlight("String", Colors::green) : "String";
        case TokenType::Text: return colored ? highlight("Text", Colors::bold_green) : "String";
        case TokenType::Bool: return colored ? highlight("Bool", Colors::bold_blue) : "Bool";
        case TokenType::Number: return colored ? highlight("Number", Colors::yellow) : "Number";

        case TokenType::Operator: return colored ? highlight("Operator", Colors::bold_cyan) : "Operator";
        case TokenType::Punctuation: return colored ? highlight("Punctuation", Colors::bold_green) : "Punctuation";

        case TokenType::If: return colored ? highlight("If", Colors::yellow) : "If";
        case TokenType::Elif: return colored ? highlight("Elif", Colors::yellow) : "Elif";
        case TokenType::Break: return colored ? highlight("Break", Colors::red) : "Break";
        case TokenType::Else: return colored ? highlight("Else", Colors::yellow) : "Else";
        case TokenType::DoWhile: return colored ? highlight("DoWhile", Colors::bold_yellow) : "DoWhile";
        case TokenType::Case: return colored ? highlight("Case", Colors::bold_yellow) : "Case";

        case TokenType::Default: return colored ? highlight("Default", Colors::red) : "Default";

        case TokenType::Continue: return colored ? highlight("Continue", Colors::red) : "Continue";

        case TokenType::Function: return colored ? highlight("Function", Colors::bold_red) : "Function";
        case TokenType::FunctionCall: return colored ? highlight("FunctionCall", Colors::bold_red) : "FunctionCall";
        case TokenType::FunctionDef: return colored ? highlight("FunctionDefinition", Colors::bold_red) : "FunctionDefinition";

        case TokenType::FunctionRef: return colored ? highlight("FunctionReference", Colors::bold_blue) : "FunctionReference";
        case TokenType::Parameter: return colored ? highlight("Parameter", Colors::bold_cyan) : "Parameter";
        case TokenType::Argument: return colored ? highlight("Argument", Colors::bold_cyan) : "Argument";

        case TokenType::ClassDef: return colored ? highlight("ClassDefinition", Colors::pink) : "ClassDef";
        case TokenType::ClassCall: return colored ? highlight("ClassCall", Colors::pink) : "ClassCall";
        case TokenType::ClassRef: return colored ? highlight("ClassReference", Colors::pink) : "ClassRef";

        case TokenType::ClassMethodCall: return colored ? highlight("ClassMethodCall", Colors::purple) : "ClassMethodCall";
        case TokenType::ClassMethodDef: return colored ? highlight("ClassMethodDef", Colors::purple) : "ClassMethodDef";
        case TokenType::ClassMethodRef: return colored ? highlight("ClassMethodRef", Colors::purple) : "ClassMethodRef";


        case TokenType::Unknown: return colored ? highlight("Unknown", Colors::pink) : "Unknown";

        case TokenType::Newline: return colored ? highlight("Newline", Colors::purple) : "Newline";
        case TokenType::Indent: return colored ? highlight("Indent", Colors::purple) : "Indent";
        case TokenType::Dedent: return colored ? highlight("Dedent", Colors::purple) : "Dedent";
        case TokenType::AccessorVariable: return colored ? highlight("AccessorVariable", Colors::bold_cyan) : "AccessorVariable";

        case TokenType::ChainEntryPoint: return colored ? highlight("ChainEntryPoint", Colors::yellow) : "ChainEntryPoint";

        case TokenType::EOF_Token: return colored ? highlight("EOF_Token", Colors::red) : "EOF_Token";
        case TokenType::SOF_Token: return colored ? highlight("SOF_Token", Colors::red) : "SOF_Token"; 
        case TokenType::LeftBracket: return colored ? highlight("LeftBracket", Colors::bold_yellow) : "LeftBracket";
        case TokenType::RightBracket: return colored ? highlight("RightBracket", Colors::bold_yellow) : "RightBracket";
        case TokenType::LeftArrow: return colored ? highlight("LeftArrow", Colors::bold_yellow) : "LeftArrow";
        case TokenType::RightArrow: return colored ? highlight("RightArrow", Colors::bold_yellow) : "RightArrow";
        case TokenType::NoOp: return colored ? highlight("NoOp", Colors::bg_black) : "NoOp";
        default: return "Invalid TokenType";
    }
}

String callableTypeAsString(CallableType callableType){
    switch (callableType)
    {
    case CallableType::FUNCTION: return "Function";
    case CallableType::DEF: return "Def";
    case CallableType::LAMBDA: return "Lambda";
    case CallableType::NATIVE: return "Native";
    case CallableType::METHOD: return "Method";
    case CallableType::CLASS: return "Class";
    case CallableType::CALLABLE: return "Callable";
    case CallableType::INSTANCE: return "Instance";
    default: return "Unknown";

    }
}

String getTokenDescription(TokenType type) {
    switch (type) {
        case TokenType::Type: return "The Type To Be Held In The Node";
        case TokenType::Keyword: return "Keyword in the language.";
        case TokenType::Identifier: return "Variable or function name.";
        case TokenType::Number: return "Number of any type.";
        case TokenType::String: return "A string representation";
        case TokenType::Bool: return "A boolean value of true or false";
        case TokenType::Text: return "Composite String | Char type";

        case TokenType::VarAssignment: return "A Variable Is Being Assigned a new value";
        case TokenType::VarDeclaration: return "A Variable Is Being Declared";
        case TokenType::Variable: return "A Variable Is Being Referenced";

        case TokenType::Operator: return "Any operation to be performed between values";
        case TokenType::Punctuation: return "A syntactic construct used to mark block starts, ends, etc.";

        case TokenType::Newline: return "A new line was created in the document";
        case TokenType::Indent: return "An indent in the document";
        case TokenType::Dedent: return "Indentation was decrease, moving into parent scope";
        case TokenType::EOF_Token: return "Indicates the end of a file";

        case TokenType::Break: return "A Break Statement";
        case TokenType::If: return "An If Statement - beginning a new block";
        case TokenType::Else: return "Else - End of an If Block";
        case TokenType::Elif: return "Secondary Condition After an If condition has failed";
        case TokenType::DoWhile: return "DoWhile";
        case TokenType::Case: return "A Case Statement";
        case TokenType::Continue: return "Continue In A Loop";

        case TokenType::ClassDef: return "A Class Is Being Defined";
        case TokenType::ClassCall: return "A Class Is Being Instantiated";
        case TokenType::ClassRef: return "A Class Is Likely Being Passed as a Value";
        case TokenType::ClassMethodDef: return "A Class Method Is Being Defined";
        case TokenType::ClassMethodRef: return "A Class Is Probably Being Used As A Value";
        case TokenType::ClassMethodCall: return "A Class Method is Being Called";

        case TokenType::Function: return "A Function";
        case TokenType::FunctionCall: return "A Function To Be Called";
        case TokenType::FunctionDef: return "A Function Is Defined";

        case TokenType::FunctionRef: return "A Function Is Being Referenced";
        case TokenType::Parameter: return "A Parameter Enclosed Within A Function/Class Declaration";
        case TokenType::Argument: return "An Argument Used within a Function/Class/Method call";

        case TokenType::Unknown: return "An unknown literal was found in the document";
        case TokenType::AccessorVariable: return "An Member Variable of an Object";

        case TokenType::ChainEntryPoint: return "The Beginning of a Chain to be resolved within the Parser, Ex: 'self.x', 'singleton::method', 'obj.<method>'";
        case TokenType::LeftBracket: return "LeftBracket For Typing Lists";
        case TokenType::RightBracket: return "RightBracket For Typing Lists";
        case TokenType::LeftArrow: return "LeftArrow For Typing Arrays";
        case TokenType::RightArrow: return "RightArrow For Typing Arrays";
        case TokenType::NoOp: return "A Token Indicating There's Nothing To Do";
        default: return "Unknown | Invalid Token.";
    }
}

// String identifierTypeToString(IdentifierType identifierType) {
//     switch (identifierType) {
//         case IdentifierType::Variable: return "Variable";
//         case IdentifierType::Function: return "Function";
//         case IdentifierType::Method: return "Method";
//         case IdentifierType::Class: return "Class";
//         default: return "Unknown";
//     }
// }


ResolvedType::ResolvedType() {
    
}

ResolvedType::ResolvedType(String primaryType) {
    baseType = primaryType;
}

ResolvedType::ResolvedType(String primaryType, Vector<ResolvedType> innerType) {
    setBaseType(primaryType);
    setInner(innerType);
}

void ResolvedType::setBaseType(String otherBaseType) {baseType = otherBaseType;}
void ResolvedType::setInner(Vector<ResolvedType> otherInnerType) {inner = otherInnerType;};

String ResolvedType::toString() const {
    if (inner.empty()) return baseType;

    String result = baseType + "[";
    for (size_t i = 0; i < inner.size(); ++i) {
        result += inner[i].toString();
        if (i < inner.size() - 1) result += ", ";
    }
    result += "]";
    return result;
}


String ResolvedType::toString() {
    if (inner.empty()) return baseType;

    String result = baseType + "[";
    for (size_t i = 0; i < inner.size(); ++i) {
        result += inner[i].toString();
        if (i < inner.size() - 1) result += ", ";
    }
    result += "]";
    return result;
}

bool ResolvedType::matches(const ResolvedType& other) const {
    if (baseType != other.baseType) return false;
    if (inner.size() != other.inner.size()) return false;
    for (size_t i = 0; i < inner.size(); ++i) {
        if (!inner[i].matches(other.inner[i])) return false;
    }
    return true;
}


size_t ResolvedType::hash() const {
    size_t h = std::hash<String>()(baseType);

    auto combine = [](size_t& seed, size_t value) {
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    };

    for (const auto& innerType : inner) {
        combine(h, innerType.hash());
    }

    return h;
}



DebugStringifyConfig& debugStringifyConfig() {
    static DebugStringifyConfig cfg{};
    return cfg;
}

String DebugStringifyConfig::handle(String s) {
    if (s.size() < maxString) { return s; } 
    auto ellipses = alwaysEllipsis ? "..." : "";
    auto remainder = "(+" + std::to_string(s.size() - maxString) + " chars)";
    
    return s.substr(0, maxString) + ellipses + remainder;
}   
