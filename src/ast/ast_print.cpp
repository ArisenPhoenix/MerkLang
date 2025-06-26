#include "utilities/debugger.h"
#include "core/types.h"

#include "utilities/helper_functions.h"
#include "ast/ast_base.h"
#include "ast/ast.h"
#include "ast/ast_chain.h"
#include "ast/ast_control.h"
#include "ast/ast_callable.h"
#include "ast/ast_function.h"
#include "ast/ast_method.h"


#include "ast/ast_class.h"
#include "core/scope.h"




// The printAST method is mostly for debugging and verification purposes. It is useful for identification of bugs and visualizing the structure
String scopeLevelAsString(SharedPtr<Scope> scope, String astCaller) {
    String scopeString = scope ? std::to_string(scope->getScopeLevel()) : "Error Getting Scope For " + astCaller;
    return "Scope Level " + scopeString;
}

// AST Basic
String LiteralValue::toString() const {
    return getAstTypeAsString() + "(value=" + value.toString() +
        ", isString=" + (_isString ? "true" : "false") +
        ", isBool=" + (_isBool ? "true" : "false") +
        scopeLevelAsString(getScope(), getAstTypeAsString()) + ")";;
}

String VariableDeclaration::toString() const {
        return getAstTypeAsString() + "(name=" + name +
            ", variable=" + variable.toString() +
            scopeLevelAsString(getScope(), getAstTypeAsString()) + ")";
}

String VariableReference::toString() const {
    return getAstTypeAsString() + "(name=" + name +
           scopeLevelAsString(getScope(), getAstTypeAsString()) + ")";
}

String VariableAssignment::toString() const {
    return getAstTypeAsString() + "(variable=" + name + ", value=" + valueExpression->toString() + ")";
}

String BinaryOperation::toString() const {
    return getAstTypeAsString() + "(operator=" + op +
        ", left=" + (left ? left->toString() : "null") +
        ", right=" + (right ? right->toString() : "null") +
        scopeLevelAsString(getScope(), getAstTypeAsString()) + ")";

}

String UnaryOperation::toString() const {
    return "UnaryOperation(" + op +
    scopeLevelAsString(getScope(), getAstTypeAsString()) + ")";
}


void LiteralValue::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    printIndent(os, indent);
    debugLog(true, highlight(getAstTypeAsString(), Colors::blue), "(value =", value,  scopeLevelAsString(getScope(), getAstTypeAsString()), ")");

    DEBUG_FLOW_EXIT();
};

void VariableDeclaration::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    indent = printIndent(os, indent);
    debugLog(true, highlight(getAstTypeAsString(), Colors::cyan), "(variable =", variable, scopeLevelAsString(getScope(), getAstTypeAsString()), "):");
    valueExpression->printAST(os, indent);

    DEBUG_FLOW_EXIT();
};

void VariableAssignment::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    indent = printIndent(os, indent);
    debugLog(true,  highlight(getAstTypeAsString(), Colors::cyan), "(variable =", name, scopeLevelAsString(getScope(), getAstTypeAsString()), ")");
    if (valueExpression) {
        valueExpression->printAST(os, indent);
    }

    DEBUG_FLOW_EXIT();
};

void VariableReference::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    printIndent(os, indent);
    debugLog(true, highlight(getAstTypeAsString(), Colors::cyan), "(variable =", name, ", ", scopeLevelAsString(getScope(), getAstTypeAsString()), ")" );

    DEBUG_FLOW_EXIT();
};

void BinaryOperation::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    printIndent(os, indent);
    debugLog(true, highlight(getAstTypeAsString(), Colors::bold_blue), "(operator =", op, scopeLevelAsString(getScope(), getAstTypeAsString()), ")");
    if (left) {
        left->printAST(os, indent + 2);
    }
    if (right) {
        right->printAST(os, indent + 2);
    }
    DEBUG_FLOW_EXIT();    
};

void UnaryOperation::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    indent = printIndent(os, indent);
    debugLog(true, highlight(getAstTypeAsString(), Colors::bold_blue), "(operator =", op, scopeLevelAsString(getScope(), getAstTypeAsString()) + ")");
    operand->printAST(os, indent);

    DEBUG_FLOW_EXIT();
}


String Return::toString() const {
    return getAstTypeAsString()+"(value=" + (returnValue ? returnValue->toString() : "None") + ")";
}


void Return::printAST(std::ostream& os, int indent) const  {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    indent = printIndent(os, indent);

    debugLog(true, highlight(getAstTypeAsString(), Colors::bold_red));
    if (returnValue) {
        returnValue->printAST(os, indent);
    }
    DEBUG_FLOW_EXIT();
}


// AST CONTROL
String CodeBlock::toString() const {
    return "CodeBlock(scopeLevel=" + std::to_string(getScope()->getScopeLevel()) + " children: " + std::to_string(children.size()) + ")";
}

String WhileLoop::toString() const {
    return "WhileLoop(condition=" + (getCondition() ? getCondition()->toString() : "null") +
            ", body=" + (getBody() ? getBody()->toString() : "null") +
            ", scope=" + std::to_string(getScope()->getScopeLevel()) + ")";

}

void ConditionalBlock::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    printIndent(os, indent);
    debugLog(true, highlight("ConditionalBlock", Colors::green), "(condition = ");

    if (condition) {
        condition->printAST(os, indent+1);
    } 
    else {
        debugLog(true, "null");
    }
    debugLog(true, ")");

    DEBUG_FLOW_EXIT();
}

void CodeBlock::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    if (getScope()->getScopeLevel() == 0){
        debugLog(true, highlight("\n======================== Start printAST ======================== ", Colors::cyan));
    }

    indent = printIndent(os, indent);
    debugLog(true, getAstTypeAsString(), scopeLevelAsString(getScope(), getAstTypeAsString()));

    if (children.empty()) {
        printIndent(os, indent);
        debugLog(true, highlight("[No Children]", Colors::orange));
    }

    for (const auto& child : children) {
        child->printAST(os, indent);
    }

    if (getScope()->getScopeLevel() == 0){
        debugLog(true, highlight("======================== End printAST ======================== \n", Colors::cyan));
    }
    DEBUG_FLOW_EXIT();

}


void CallableBody::printAST(std::ostream& os, int indent) const {
    indent = printIndent(os, indent);
    debugLog(true, getAstTypeAsString(), scopeLevelAsString(getScope(), getAstTypeAsString()));
    for (auto& child : children){
        child->printAST(os, indent);
    }
}

void CallableDef::printAST(std::ostream& os, int indent) const {
    auto paramStr = !parameters.empty() ?  parameters.toShortString() : "";
    indent = printIndent(os, indent);
    debugLog(true, getAstTypeAsString(), name, "(" + paramStr + ")", scopeLevelAsString(getScope(), getAstTypeAsString()));
    body->printAST(os, indent);
};


void CallableCall::printAST(std::ostream& os, int indent) const {
    indent = printIndent(os, indent);
    debugLog(true, getAstTypeAsString(), "()");
}

void ElseStatement::printAST(std::ostream& os, int indent) const  {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    indent = printIndent(os, indent);
    debugLog(true, getAstTypeAsString());
    body->printAST(os, indent);
    DEBUG_FLOW_EXIT();
}

void ElifStatement::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    indent = printIndent(os, indent);
    debugLog(true, getAstTypeAsString());
    condition->printAST(os, indent + 1);
    body->printAST(os, indent + 2);

    DEBUG_FLOW_EXIT();
}

void IfStatement::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    int newIndent = printIndent(os, indent);
    debugLog(true, getAstTypeAsString(), scopeLevelAsString(getScope(), getAstTypeAsString()));

    // ConditionalBlock::printAST(os, indent + 1);
    condition->printAST(os, indent + 1);
    body->printAST(os, newIndent);

    for (const auto& elif : elifNodes) {
        elif->printAST(os, indent);
    }

    if (elseNode) {
        elseNode->printAST(os, indent);
    }

    DEBUG_FLOW_EXIT();

}

void WhileLoop::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    indent = printIndent(os, indent);
    debugLog(true, getAstTypeAsString());

    indent = printIndent(os, indent);
    condition->printAST(os, indent);

    indent = printIndent(os, indent);
    body->printAST(os, indent);

    DEBUG_FLOW_EXIT();

}

void NoOpNode::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    indent = printIndent(os, indent);
    debugLog(true, getAstTypeAsString());

    DEBUG_FLOW_EXIT();

}

void Break::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    indent = printIndent(os, indent);
    debugLog(true, getAstTypeAsString(), scopeLevelAsString(getScope(), getAstTypeAsString()));

    DEBUG_FLOW_EXIT();

}

bool CodeBlock::containsReturnStatement() const {
    DEBUG_FLOW();

    for (const auto& child : children) {
        if (!child) continue;

        // Check if the child is a ReturnNode
        if (child->getAstType() == AstType::Return) {
            return true;
        }

        // If the child is a nested CodeBlock, check recursively
        auto blockPtr = dynamic_cast<CodeBlock*>(child.get());
        if (blockPtr && blockPtr->containsReturnStatement()) {
            DEBUG_FLOW_EXIT();
            return true;
        }
    }

    DEBUG_FLOW_EXIT();
    return false;  // No return statement found
}


// AST FUNCTIONS
void FunctionBody::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    indent = printIndent(os, indent);

    debugLog(true, getAstTypeAsString(), scopeLevelAsString(getScope(), getAstTypeAsString()) + "):");

    if (children.empty()) {
        indent = printIndent(os, indent);
        debugLog(true, "[No Children]");
    }

    for (const auto& child : children) {
        child->printAST(os, indent);
    }

    DEBUG_FLOW_EXIT();

}

void FunctionDef::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    indent = printIndent(os, indent);
    debugLog(true, getAstTypeAsString(), "(Name =", name, scopeLevelAsString(getScope(), getAstTypeAsString()),"):");
    
    parameters.printAST(os, indent);

    body->printAST(os, indent);

    DEBUG_FLOW_EXIT();

}

void FunctionRef::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    printIndent(os, indent);
    debugLog(true, highlight(getAstTypeAsString(), Colors::bold_red), "(Name =", name, scopeLevelAsString(getScope(), getAstTypeAsString()), "):");

    DEBUG_FLOW_EXIT();
}

void FunctionCall::printAST(std::ostream& os, int indent) const {
    indent = printIndent(os, indent);
    debugLog(true, getAstTypeAsString(), "(Name =", name, scopeLevelAsString(getScope(), getAstTypeAsString()), "):");
    for (const auto& arg : arguments){
        printIndent(os, indent);
        debugLog(true, arg->toString());
    }
    os << std::endl;
}

// void ParameterAssignment::printAST(std::ostream& os, int indent) const {
//     DEBUG_FLOW(FlowLevel::VERY_LOW);

//     indent = printIndent(os, indent);
//     debugLog(true, "ParameterAssignment (variable = ", getName(), scopeLevelAsString(getScope(), getAstTypeAsString()), ")"); 
//     getExpression()->printAST(os, indent);

//     DEBUG_FLOW_EXIT();

// }


String MethodDef::toString() const {return astTypeToString(getAstType());}


void ClassDef::printAST(std::ostream& os, int indent) const {
    indent = printIndent(os, indent);
    debugLog(true, "ClassDef: ", name);
    body->printAST(os, indent);
}
void ClassCall::printAST(std::ostream& os, int indent) const {
    indent = printIndent(os, indent);
    std::ostringstream argString;
    for (const auto& arg : arguments){
        argString << arg->toString();
    }
    debugLog(true, getAstTypeAsString(), "(" + argString.str() + ")");
}




void Accessor::printAST(std::ostream& os, int indent) const {
    indent = printIndent(os, indent);
    debugLog(true, getAstTypeAsString() + "(" + getAccessor() + scopeLevelAsString(getScope(), getAstTypeAsString()), ")");
}



String ChainOperation::toString() const {
    String kind = opKindAsString(opKind);
    auto lhs = getLeftSide();
    String scoping = lhs->getSecondaryScope() ? lhs->getSecondaryScope()->formattedScope() : "null";
    return getAstTypeAsString() + "(kind: " + kind + ", lhs: " + lhs->toString() + scopeLevelAsString(getScope(), getAstTypeAsString()) + ")";
}

void ChainOperation::printAST(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << getAstTypeAsString() << ")\n";
    lhs->printAST(os, indent + 2);
    if (rhs) rhs->printAST(os, indent + 2);
}



void Chain::printAST(std::ostream& os, int indent) const {
    indent = printIndent(os, indent);
    debugLog(true, getAstTypeAsString()+":");

    for (const auto& elem : elements) {
        elem.printAST(os, indent);
    }
}

String Chain::toString() const {
    std::ostringstream oss;
    int size = elements.size() - 1;
    for (const auto& elem : elements) {
        oss << elem.name;
        if (elem.name != elements[size].name){
            oss << ".";
        }
        
    }
    return oss.str();
}
