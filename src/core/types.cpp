#include "core/types.h"




String nodeTypeToString(NodeValueType type) {
    switch (type) {
        case NodeValueType::String: return highlight("String", Colors::bold_green);
        case NodeValueType::Char: return highlight("Char", Colors::bold_green);

        case NodeValueType::Number: return highlight("Keyword", Colors::bold_blue);
        case NodeValueType::Int: return highlight("Int", Colors::bold_blue);
        case NodeValueType::Bool: return highlight("Bool", Colors::cyan);
        case NodeValueType::Float: return highlight("Float", Colors::bold_blue);
        case NodeValueType::Double: return highlight("Double", Colors::bold_blue);
        case NodeValueType::Long: return highlight("Long", Colors::bold_blue);
        
        case NodeValueType::Vector: return highlight("Vector", Colors::yellow);
        case NodeValueType::Shared_Vector: return highlight("Shared_Vector", Colors::yellow);

        case NodeValueType::Parameter: return highlight("Parameter", Colors::bold_white);
        case NodeValueType::Class: return highlight("Class", Colors::bold_white);

        case NodeValueType::Function: return highlight("Function", Colors::purple);
        case NodeValueType::Method: return highlight("Method", Colors::purple);

        case NodeValueType::Null: return highlight("Null", Colors::orange);
        case NodeValueType::Any: return highlight("Any", Colors::green);
        case NodeValueType::None: return highlight("None", Colors::orange);
        
        case NodeValueType::Uninitialized: return highlight("Uninitialized", Colors::red);
        case NodeValueType::UNKNOWN: return highlight("Unknown", Colors::bold_red);
        case NodeValueType::Callable: return highlight("Callable", Colors::red);
        case NodeValueType::ClassInstance: return highlight("ClassInstance", Colors::red);

        default: throw std::runtime_error("Unknown NodeValueType encountered in nodeTypeToString.");

    }
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


        case AstType::Function: return "Function";
        case AstType::NoOp: return highlight("NoOp", Colors::light_gray);

        case AstType::ImportStatement: return highlight("ImportStatement", Colors::light_blue);

        case AstType::AST: return highlight("AST", Colors::red);

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

        case TokenType::String: return colored ? highlight("String", Colors::green) : "String";
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

        default: return "Unknown or invalid token.";
    }
}

String identifierTypeToString(IdentifierType identifierType) {
    switch (identifierType)
    {
    case IdentifierType::Variable: return "Variable";
    case IdentifierType::Function: return "Function";
    case IdentifierType::Method: return "Method";
    case IdentifierType::Class: return "Class";
    default: return "Unknown";

    }
}
