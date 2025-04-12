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
 

Chain::Chain(SharedPtr<Scope> scope) : ASTStatement(scope) { branch = "Chain"; }
    
void Chain::addElement(const ChainElement& element) {
    elements.push_back(element);
}

const Vector<ChainElement>& Chain::getElements() const {
    return elements;
}


// SharedPtr<Scope> tryExtractScopeFromNode(const Node& node) {
//     const auto& variantVal = node.getValue();

//     if (std::holds_alternative<SharedPtr<ClassBase>>(variantVal)) {
//         return ClassNode(std::get<SharedPtr<ClassBase>>(variantVal)).getInternalScope();
//     }
//     if (std::holds_alternative<SharedPtr<Function>>(variantVal)) {
//         return FunctionNode(std::get<SharedPtr<Function>>(variantVal)).getInternalScope();
//     }
//     if (std::holds_alternative<SharedPtr<Method>>(variantVal)) {
//         return MethodNode(std::get<SharedPtr<Method>>(variantVal)).getInternalScope();
//     }

//     return nullptr;
// }
// case NodeValueType::ClassInstance: {
//     auto instance = std::get<SharedPtr<ClassInstance>>(current.getValue());
//     nextScope = instance->getScope();
//     break;
// }

SharedPtr<Scope> getAssociatedScope(const Node& node) {
    switch (node.getType()) {
        case NodeValueType::Class:
            return std::get<SharedPtr<ClassBase>>(node.getValue())->getCapturedScope();
        case NodeValueType::Function:
            return std::get<SharedPtr<Function>>(node.getValue())->getCapturedScope();
        case NodeValueType::Method:
            return std::get<SharedPtr<Method>>(node.getValue())->getCapturedScope();
        case NodeValueType::ClassInstance:
            DEBUG_LOG(LogLevel::TRACE, "getAssociatedScope -> ClassInstance scope resolved");

            return std::get<SharedPtr<ClassInstance>>(node.getValue())->getCapturedScope();
        default:
            return nullptr;  // Caller should handle this case
    }
}


void Chain::assign(SharedPtr<Scope> scope, const Node& value) const {
    if (elements.empty()) {
        throw MerkError("Cannot assign to an empty chain.");
    }

    // Step 1: Resolve up to the *second-to-last* element
    const auto& first = elements.front();
    Node current = scope->lookup(first.name, first.type);

    for (size_t i = 1; i < elements.size() - 1; ++i) {
        const auto& elem = elements[i];

        if (!current.isValid()) {
            throw MerkError("Invalid node encountered while resolving '" + elem.name + "' for assignment.");
        }

        SharedPtr<Scope> nextScope = getAssociatedScope(current);
        if (!nextScope) {
            throw MerkError("No accessible scope found while resolving '" + elem.name + "' for assignment.");
        }

        current = nextScope->lookup(elem.name, elem.type);
    }

    // Step 2: Get the final element (target of assignment)
    const auto& target = elements.back();

    if (!current.isValid()) {
        throw MerkError("Cannot assign to '" + target.name + "' because the parent object is invalid.");
    }

    SharedPtr<Scope> targetScope = getAssociatedScope(current);
    if (!targetScope) {
        throw MerkError("Cannot assign to '" + target.name + "' — no accessible scope in parent.");
    }

    // Step 3: Perform the assignment (reuses the existing update mechanism)
    targetScope->updateVariable(target.name, value);
}




Node Chain::resolve(SharedPtr<Scope> scope) const {
    if (elements.empty()) {
        throw MerkError("Cannot resolve an empty chain.");
    }

    const auto& first = elements.front();
    Node current = scope->lookup(first.name, first.type);

    for (size_t i = 1; i < elements.size(); ++i) {
        const auto& elem = elements[i];

        if (!current.isValid()) {
            throw MerkError("Chain resolution failed: '" + elem.name + "' encountered after an invalid node.");
        }

        SharedPtr<Scope> nextScope = getAssociatedScope(current);
        if (!nextScope) {
            throw MerkError("No accessible scope found on object while resolving '" + elem.name + "'");
        }

        current = nextScope->lookup(elem.name, elem.type);
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

Node Chain::evaluate(SharedPtr<Scope> scope) const {
    return resolve(scope);
}

UniquePtr<BaseAST> Chain::clone() const {
    auto copy = std::make_unique<Chain>(getScope());
    copy->elements = elements; // assuming a copyable vector
    return copy;
}