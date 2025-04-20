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
    if (std::holds_alternative<SharedPtr<ClassInstance>>(variantVal)) {
        return ClassInstanceNode(std::get<SharedPtr<ClassInstance>>(variantVal)).getInternalScope();
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

void Chain::setScope(SharedPtr<Scope> scope) {
    this->scope = scope;
    for (auto& elem : elements) {
        if (elem.object) elem.object->setScope(scope);
    }
}


// Node Chain::evaluate(SharedPtr<Scope> scope) const {
//     DEBUG_FLOW(FlowLevel::HIGH);
//     Node current;
//     SharedPtr<Scope> currentScope = classScope ? classScope : scope ? scope : getScope();
//     DEBUG_LOG(LogLevel::ERROR, "Chain::evaluate was called with scope level: ", currentScope->getScopeLevel());
//     DEBUG_LOG(LogLevel::ERROR, "Has Class Scope: ", classScope ? "true" : "false", "Has Provided Scope: ", scope ? "true": "false", "Has InternalScope: ", getScope() ? "true" : "false");
//     DEBUG_LOG(LogLevel::ERROR, "Has Finalized Scope: ", currentScope ? "true" : "false");
//     DEBUG_LOG(LogLevel::ERROR, "Has clsScope: ", classScope ? "true" : "false", "Resolution Start Index: ", resolutionStartIndex, "Num Entries: ", elements.size());
    
//     if (!currentScope){
//         currentScope = getScope();
//     }
//     // Start from the resolutionStartIndex (default 0)
//     for (size_t i = resolutionStartIndex; i < elements.size(); ++i) {
//         const auto& elem = elements[i];

//         DEBUG_LOG(LogLevel::ERROR, "Current elem in Chain is: ");
//         elem.printAST(std::cout, currentScope->getScopeLevel());

//         // Evaluate the object within the current scope
//         Node result = elem.object->evaluate(currentScope);
//         if (!result.isValid()) {
//             throw MerkError("Invalid resolution at chain element: " + elem.name);
//         }

//         current = result;
//         DEBUG_LOG(LogLevel::ERROR, "NODE DATA IN Chain::evaluate |", nodeTypeToString(current.getType()));
//         // Advance the scope based on the result
//         DEBUG_LOG(LogLevel::ERROR, "Is Callable: ", current.getIsCallable());
//         if (current.getIsCallable()) {
//             NodeValueType type = current.getType();
            

//             if (type == NodeValueType::ClassInstance) {
//                 // If the next element is accessed via `.`, try to resolve an attribute/method
//                 if (i + 1 < elements.size() && elements[i + 1].delimiter == ".") {
//                     auto nextName = elements[i + 1].name;
//                     Node field = current.getField(nextName);

//                     if (!field.isValid()) {
//                         throw MerkError("Field '" + nextName + "' not found in class instance.");
//                     }

//                     current = field;
//                     if (i > 0 && elements[i - 1].delimiter == "." && current.getType() == NodeValueType::ClassInstance) {
//                         currentScope = getAssociatedScope(current);  // <-- instanceScope (for self)
//                     }
//                     // currentScope = getAssociatedScope(current); // in case it's a method or scoped field
//                     continue;
//                 }
//             } else if (type == NodeValueType::Callable){
//                 // Invalid Virtual CallableNode
//             } else if (type == NodeValueType::Function) {
//                 // Function Specific Log --- if Needed
//             } else if (type == NodeValueType::Class) {
//                 // Class Specific Logic
//             } else if (type == NodeValueType::ClassInstance) {
//                 // Class Instance Specific Logic
//             } else if (type == NodeValueType::Method){
//                 // Method Specific Logic
//             } else {
//                 DEBUG_LOG(LogLevel::ERROR, "No Conditions Were Met When Finding NodeValueType in Chain::evaluate");
                
//                 // handle raw values such as Int
//             }
//             // Optionally add logic for other callable types (e.g., static, methods)
//         }

//         if (mode == ResolutionMode::ClassInstance) {
//             if (classScope){
//                 current = elem.object->evaluate(classScope);
//             }
//             throw MerkError("Resolution Mode was set to ClassInstance, yet no Class Scope Was Apparent");
//         }
//         if (classScope){
//         } else {
//             current = elem.object->evaluate(currentScope);
//         }
       


//         currentScope = getAssociatedScope(current);  // Advance scope if applicable
//     }
//     DEBUG_FLOW_EXIT();
//     return current;
// }


Node Chain::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    DEBUG_LOG(LogLevel::ERROR, "Chain::evaluate", "Num Elements:", elements.size());
    SharedPtr<Scope> currentScope = classScope ? classScope : (scope ? scope : getScope());
    if (!currentScope) throw MerkError("Chain::evaluate: no valid scope");
    Node currentVal;
    for (size_t i = resolutionStartIndex; i < elements.size(); ++i) {
        const auto& elem = elements[i];

        if (mode == ResolutionMode::ClassInstance) {
            if (!classScope){
                throw MerkError("No Class Scope Was Provided for ResolutionMode::ClassInstance");
            }
            DEBUG_LOG(LogLevel::ERROR, "Evaluating Class Instance");
            Node currentVal = elem.object->evaluate(classScope);

        } else {
            DEBUG_LOG(LogLevel::ERROR, "Evaluating Normal Instance");

            currentVal = elem.object->evaluate(currentScope);

        }
        // elem.printAST(std::cout, currentScope->getScopeLevel());

        if (!currentVal.isValid()) {
            throw MerkError("Invalid resolution at chain element: " + elem.name);
        }
        DEBUG_LOG(LogLevel::ERROR, "Current Val: ", currentVal);


        // // Special behavior for class instances
        // if (currentVal.getIsCallable() && currentVal.getType() == NodeValueType::ClassInstance) {
        //     if (i + 1 < elements.size() && elements[i + 1].delimiter == ".") {
        //         auto nextName = elements[i + 1].name;
        //         Node field = currentVal.getField(nextName);
        //         if (!field.isValid()) {
        //             throw MerkError("Field '" + nextName + "' not found in class instance.");
        //         }
        //         currentVal = field;
        //         currentScope = getAssociatedScope(currentVal);
        //         continue;
        //     }
        // }

        currentScope = getAssociatedScope(currentVal);
    }
    DEBUG_LOG(LogLevel::ERROR, "Final Chain Evaluation Val: ", currentVal);
    DEBUG_FLOW_EXIT();
    return currentVal;
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
    for (const auto& element : elements) {
        ChainElement elem;
        elem.delimiter = element.delimiter;
        elem.name = element.name;
        elem.object = element.object->clone();  // don't move from original
        elem.type = element.type;

        copy->addElement(std::move(elem));
    }
    return copy;
}



void Chain::replaceLastElementWith(ChainElement&& elem) {
    elements.back() = std::move(elem);
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