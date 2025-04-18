#include "core/types.h"
#include "core/scope.h"
#include "core/node.h"

#include "utilities/debugger.h"
#include "utilities/debugging_functions.h"
#include "utilities/helper_functions.h"
#include "ast/ast_function.h"
#include "core/classes/class_base.h"
#include "core/classes/method.h"
#include "ast/ast_chain.h"
 


ChainElement::ChainElement() {
    name = "";
    type = TokenType::Unknown;
    object = nullptr;
    delimiter = "";
}

ChainElement& ChainElement::operator=(ChainElement&& other) noexcept {
    name = other.name;
    type = other.type;
    object = std::move(other.object->clone());
    delimiter = other.delimiter;
    return *this;
}
ChainElement::ChainElement(ChainElement&& other) noexcept {
    name = other.name;
    type = other.type;
    object = std::move(other.object->clone());
    delimiter = other.delimiter;
}

// Chain::Chain() : ASTStatement() {}


Chain::Chain(SharedPtr<Scope> scope) : ASTStatement(scope) { branch = "Chain"; }
void Chain::addElement(ChainElement&& elem) {
    elements.push_back(std::move(elem));

}

void Chain::setResolutionStartIndex(int index) {resolutionStartIndex = index;}


const Vector<ChainElement>& Chain::getElements() const {
    return elements;
}


SharedPtr<Scope> tryExtractScopeFromNode(const Node& node) {
    const auto& variantVal = node.getValue();

    if (std::holds_alternative<SharedPtr<ClassBase>>(variantVal)) {
        return ClassNode(std::get<SharedPtr<ClassBase>>(variantVal)).getInternalScope();
    }
    if (std::holds_alternative<SharedPtr<Function>>(variantVal)) {
        return FunctionNode(std::get<SharedPtr<Function>>(variantVal)).getInternalScope();
    }
    if (std::holds_alternative<SharedPtr<Method>>(variantVal)) {
        return MethodNode(std::get<SharedPtr<Method>>(variantVal)).getInternalScope();
    }

    return nullptr;
}


SharedPtr<Scope> getAssociatedScope(const Node& node) {
    switch (node.getType()) {
        case NodeValueType::Class:
            return std::get<SharedPtr<ClassBase>>(node.getValue())->getCapturedScope();
        case NodeValueType::Function:
            return std::get<SharedPtr<Function>>(node.getValue())->getCapturedScope();
        case NodeValueType::Method:
            return std::get<SharedPtr<Method>>(node.getValue())->getCapturedScope();

        case NodeValueType::ClassInstance: {
            auto instance = std::get<SharedPtr<ClassInstance>>(node.getValue());
            DEBUG_LOG(LogLevel::TRACE, "getAssociatedScope -> ClassInstance scope resolved");
            return std::get<SharedPtr<ClassInstance>>(node.getValue())->getCapturedScope();
        }
            
        default:
            return nullptr;  // Caller should handle this case
    }
}

// Node Chain::evaluate(SharedPtr<Scope> scope) const {
//     Node current;
//     SharedPtr<Scope> currentScope = scope;

//     for (size_t i = 0; i < elements.size(); ++i) {
//         const auto& elem = elements[i];

//         Node result = elem.object->evaluate(currentScope);
//         if (!result.isValid()) {
//             throw MerkError("Invalid resolution at chain element: " + elem.name);
//         }

//         // Check if result is a class instance and we're doing a field or method access
//         if (result.getIsCallable()) {
//             NodeValueType nodeType = result.getType();
//             if (nodeType == NodeValueType::Callable){
//                 // Invalid Virtual CallableNode
//             } else if (nodeType == NodeValueType::Function) {
//                 // Function Specific Log --- if Needed
//             } else if (nodeType == NodeValueType::Class) {
//                 // Class Specific Logic
//             } else if (nodeType == NodeValueType::ClassInstance) {
//                 // Class Instance Specific Logic
//             } else if (nodeType == NodeValueType::Method){
//                 // Method Specific Logic
//             }
//             // && i + 1 < elements.size()
//             const auto& next = elements[i + 1];



//             // Only resolve attributes if dot-access
//             if (next.delimiter == ".") {
//                 Node field = result.getField(next.name);
//                 if (!field.isValid()) {
//                     throw MerkError("Field '" + next.name + "' not found in class instance.");
//                 }
//                 current = field;
//                 currentScope = getAssociatedScope(current);  // Could be field-specific
//                 ++i;  // Skip ahead since we've manually handled next
//                 continue;
//             }

//             // Handle :: static access later
//         }

//         // General case
//         current = result;
//         currentScope = getAssociatedScope(current);
//     }

//     return current;
// }
Node Chain::evaluate(SharedPtr<Scope> scope) const {
    Node current;
    // SharedPtr<Scope> currentScope = this->scope ? this->scope : scope;
    SharedPtr<Scope> currentScope = classScope ? classScope : scope;
    if (!currentScope){
        currentScope = getScope();
    }
    // Start from the resolutionStartIndex (default 0)
    for (size_t i = resolutionStartIndex; i < elements.size(); ++i) {
        const auto& elem = elements[i];

        // Evaluate the object within the current scope
        Node result = elem.object->evaluate(currentScope);
        if (!result.isValid()) {
            throw MerkError("Invalid resolution at chain element: " + elem.name);
        }

        current = result;

        // Advance the scope based on the result
        if (current.getIsCallable()) {
            NodeValueType type = current.getType();

            if (type == NodeValueType::ClassInstance) {
                // If the next element is accessed via `.`, try to resolve an attribute/method
                if (i + 1 < elements.size() && elements[i + 1].delimiter == ".") {
                    auto nextName = elements[i + 1].name;
                    Node field = current.getField(nextName);

                    if (!field.isValid()) {
                        throw MerkError("Field '" + nextName + "' not found in class instance.");
                    }

                    current = field;
                    currentScope = getAssociatedScope(current); // in case it's a method or scoped field
                    continue;
                }
            } else if (type == NodeValueType::Callable){
                // Invalid Virtual CallableNode
            } else if (type == NodeValueType::Function) {
                // Function Specific Log --- if Needed
            } else if (type == NodeValueType::Class) {
                // Class Specific Logic
            } else if (type == NodeValueType::ClassInstance) {
                // Class Instance Specific Logic
            } else if (type == NodeValueType::Method){
                // Method Specific Logic
            } else {
                // handle raw values such as Int
            }

            // Optionally add logic for other callable types (e.g., static, methods)
        }

        currentScope = getAssociatedScope(current);  // Advance scope if applicable
    }

    return current;
}



void Chain::printAST(std::ostream& os, int indent) const {
    indent = printIndent(os, indent);
    debugLog(true, getAstTypeAsString());
    for (const auto& elem : elements) {
        elem.printAST(os, indent);
    }
}



String Chain::toString() const {
    std::ostringstream oss;
    for (const auto& elem : elements) {
        oss << elem.name;
        oss << ".";
    }
    return oss.str();
}


UniquePtr<BaseAST> Chain::clone() const {
    auto copy = std::make_unique<Chain>(getScope());
    for (auto& element: elements){
        ChainElement elem;
        elem.delimiter = element.delimiter;
        elem.name = element.name;
        elem.object = std::move(element.object->clone());
        elem.type = element.type;
    }
    // copy->elements = elements; // assuming a copyable vector
    return copy;
}


void Chain::replaceLastElementWith(ChainElement&& elem) {
    elements.back() = std::move(elem);
}