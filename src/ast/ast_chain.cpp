#include "core/types.h"
#include "core/scope.h"
#include "core/node.h"
#include "ast/ast_base.h"

#include "utilities/debugger.h"
#include "utilities/debugging_functions.h"
#include "utilities/helper_functions.h"
#include "ast/ast_callable.h"

#include "ast/ast_function.h"
#include "core/classes/class_base.h"
#include "core/classes/method.h"
#include "ast/ast_chain.h"

#include "core/errors.h"
#include "core/scope.h"


String opKindAsString(ChainOpKind opKind){
    switch (opKind) {
        case ChainOpKind::Reference: return "Reference";
        case ChainOpKind::Assignment: return "Assignment";
        case ChainOpKind::Declaration: return "Declaration";
        default: throw MerkError("Unknown OpKind");
    }
}


ChainElement::ChainElement() {
    name = "";
    type = TokenType::Unknown;
    object = nullptr;
    delimiter = "";
}

// ChainElement& ChainElement::operator=(ChainElement&& other) noexcept {
//     name = other.name;
//     type = other.type;
//     object = std::move(other.object->clone());
//     delimiter = other.delimiter;
//     return *this;
// }

ChainElement::ChainElement(ChainElement&& other) noexcept
  : name(std::move(other.name))
  , object(std::move(other.object))        // just steal the pointer
  , delimiter(std::move(other.delimiter))
  , type(other.type)
{}

// ChainElement::ChainElement(ChainElement&& other) noexcept {
//     name = other.name;
//     type = other.type;
//     object = std::move(other.object->clone());
//     delimiter = other.delimiter;
// }

ChainElement& ChainElement::operator=(ChainElement&& other) noexcept {
    if (this != &other) {
      name      = std::move(other.name);
      object    = std::move(other.object);    // DON’T clone!
      delimiter = std::move(other.delimiter);
      type      = other.type;
    }
    return *this;
  }

// Chain::Chain() : ASTStatement() {}
ChainElement::~ChainElement() {
    // clear();
}
void ChainElement::clear() {
    object.reset();   // deletes the AST, then sets the ptr to null
}

Chain::Chain(SharedPtr<Scope> scope) : ASTStatement(scope) { branch = "Chain"; }
void Chain::addElement(ChainElement&& elem) {
    elements.push_back(std::move(elem));

}


void Chain::clear() {
    for (auto& elem : elements){
        elem.clear();
    } 

}
Chain::~Chain() {
    if (getSecondaryScope()){
        getSecondaryScope().reset();
    }
    clear();
}
void Chain::setResolutionStartIndex(int index) {resolutionStartIndex = index;}
void Chain::setSecondaryScope(SharedPtr<Scope> secondary) {classScope = secondary;}
void Chain::setResolutionMode(ResolutionMode newMode) {mode = newMode;}

int Chain::getResolutionStartIndex() const {return resolutionStartIndex;}
ResolutionMode Chain::getResolutionMode() const {return mode;}
SharedPtr<Scope> Chain::getSecondaryScope() const {
    if (classScope.expired()){
        return nullptr;
    }
    if (auto scope = classScope.lock()){
        return scope;
    } else {
        return nullptr;
    }
}

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


Node Chain::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    DEBUG_LOG(LogLevel::ERROR, "Chain::evaluate", "Num Elements:", elements.size());
    DEBUG_LOG(LogLevel::ERROR, "ResolutionStartIndex: ", resolutionStartIndex);
    SharedPtr<Scope> currentScope = getSecondaryScope() ? getSecondaryScope() : (scope ? scope : getScope());
    if (!currentScope) {throw MerkError("Chain::evaluate: no valid scope");}
    // DEBUG_LOG(LogLevel::PERMISSIVE, "Chain::evaluate() | Starting Scope:");

    currentScope->debugPrint();
    currentScope->printChildScopes();
    // DEBUG_LOG(LogLevel::PERMISSIVE, "Chain::evaluate() | Num Elements:", elements.size());

    // DEBUG_LOG(LogLevel::PERMISSIVE, "Chain::evaluate() | Call Scope:");
    scope->debugPrint();
    scope->printChildScopes();
    Node currentVal;
    auto& baseElem = elements[resolutionStartIndex];
    if (!baseElem.object){
        throw MerkError("Base Element in Chain is null");
    }

    switch (baseElem.type)
    {
    case TokenType::ClassCall:  // Actuall refers to a ClassInstance, previous to the instantiation it is a call
        // DEBUG_LOG(LogLevel::PERMISSIVE, "Chain::evaluate(): BaseElem is a ClassCall");
        currentVal = baseElem.object->evaluate(currentScope);
        break;
    case TokenType::Variable:  // Actually refers to a variableReference
        // DEBUG_LOG(LogLevel::PERMISSIVE, "Chain::evaluate(): BaseElem is a Variable of Value", baseElem.object->getAstTypeAsString(), "| Name: ", baseElem.name);
        currentVal = baseElem.object->evaluate(currentScope);
        break;
    case TokenType::FunctionCall:
        // DEBUG_LOG(LogLevel::PERMISSIVE, "Chain::evaluate(): BaseElem is a FunctionCall");
        currentVal = baseElem.object->evaluate(currentScope);
        break;
    default:
        DEBUG_LOG(LogLevel::ERROR, "Chain::evaluate(): BaseElem is error type of", tokenTypeToString(baseElem.type));
        DEBUG_FLOW_EXIT();
        throw MerkError("See Above Error Message");
        break;
    }
    int size = elements.size();
    if (resolutionStartIndex + 1 >= size){
        DEBUG_LOG(LogLevel::FLOW, "Returning currentVal as moving further would go past chain index size");
        DEBUG_FLOW_EXIT();
        return currentVal;
    }

    for (size_t i = resolutionStartIndex + 1; i < elements.size(); ++i) {
        const auto& elem = elements[i];

        if (currentVal.isClassInstance()) {
            auto instance = std::get<SharedPtr<ClassInstance>>(currentVal.getValue());
            currentScope = instance->getInstanceScope();
            currentVal = instance->getField(elem.name, elem.type);

            // If the value is itself a callable, update the scope again
            SharedPtr<Scope> assoc = getAssociatedScope(currentVal);
            if (assoc) currentScope = assoc;

        } else {
            // currentScope = getAssociatedScope(currentVal);
            if (!currentScope) {
                DEBUG_FLOW_EXIT();

                throw MerkError("Unable to find valid scope during chain traversal for " + elem.name);
            }

            currentVal = elem.object->evaluate(currentScope);
        }

    }




    // DEBUG_LOG(LogLevel::PERMISSIVE, "Final Chain Evaluation Val: ", currentVal);
    DEBUG_FLOW_EXIT();
    return currentVal;
}


// if (currentNode.isCallable && nextElementIsFunctionCall) {
//     auto callable = currentNode->getCallable();
//     if (callable) {
//         return callable->execute(args, currentScope);  // or class scope
//     }
// }

Vector<ChainElement>& Chain::getMutableElements() {
    return elements;
}
void Chain::printAST(std::ostream& os, int indent) const {
    indent = printIndent(os, indent);
    debugLog(true, getAstTypeAsString()+":");

    // printIndent(os, indent);
    for (const auto& elem : elements) {
        // indent = printIndent(os, indent);
        elem.printAST(os, indent);
        // DEBUG_LOG(LogLevel::PERMISSIVE, "Added: ", elem.object->getAstTypeAsString(), "to AST");
    }
}

ChainElement& Chain::getElement(int index) {
    if (elements.empty()) {
        throw MerkError("Chain::getObject() called on empty chain.");
    }

    if (index == -1) {
        return getLast();
    }

    if (index < 0 || static_cast<size_t>(index) >= elements.size()) {
        throw MerkError("Chain::getObject() index out of range: " + std::to_string(index));
    }

    return elements[index];
}


ChainElement& Chain::getLast() {
    if (elements.empty()) {
        throw MerkError("Chain::lastObject() called on empty chain.");
    }
    return elements.back();
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
    // copy->setResolutionMode()
    copy->setResolutionStartIndex(getResolutionStartIndex());
    // if (mode == ResolutionMode::ClassInstance && classScope){
    //     copy->setSecondaryScope(getSecondaryScope());
    // }
    copy->setResolutionMode(getResolutionMode());
    copy->setSecondaryScope(getSecondaryScope());
    return copy;
}



void Chain::replaceLastElementWith(ChainElement&& elem) {
    elements.back() = std::move(elem);
}


ChainOperation::ChainOperation(UniquePtr<Chain> lhs,
                               UniquePtr<ASTStatement> rhs,
                               ChainOpKind opKind,
                               SharedPtr<Scope> scope,
                               bool isConst,
                               bool isMutable,
                               bool isStatic
                            )
    : ASTStatement(scope), lhs(std::move(lhs)), rhs(std::move(rhs)), opKind(opKind), isConst(isConst), isMutable(isMutable), isStatic(isStatic) {}

Node ChainOperation::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    // DEBUG_LOG(LogLevel::PERMISSIVE, "ChainOperation::evaluate: ", toString());
    const auto& elements = lhs->getElements();
    const auto& last = elements.back();
    int lastIndex = static_cast<int>(elements.size()) - 1;

    int index = lhs->getResolutionStartIndex();

    const auto& firstElement = elements[index];
    // DEBUG_LOG(LogLevel::PERMISSIVE, "ChainOpKind", opKindAsString(opKind));
// If it's a VariableReference, set its scope explicitly

    // Chain* rh;
    // if (rhs && rhs->getAstType() == AstType::Chain) {
    //     rh = static_cast<Chain*>(getRightSide());
    // }
    if (!lhs->getSecondaryScope()){
        // DEBUG_LOG(LogLevel::PERMISSIVE, "No Secondary Scope on lhs");
    }
    
    SharedPtr<Scope> currentScope = lhs->getSecondaryScope() ? lhs->getSecondaryScope() : getScope() ? getScope() : scope;
    // DEBUG_LOG(LogLevel::PERMISSIVE, "Current Element Being Worked on in ChainOperation::evalute: ");
    elements[index].object->printAST(std::cout, 0);
    Node current;
    if (firstElement.object->getAstType() == AstType::VariableReference && index != 0) {
        // DEBUG_LOG(LogLevel::PERMISSIVE, "Acting On Initial VariableReference");
        auto* varRef = static_cast<VariableReference*>(firstElement.object.get());
        // varRef->setScope(currentScope);
        current = Node(varRef->getName(), "Variable");
    } else {
        current = elements[index].object->evaluate(currentScope);
    }

    // DEBUG_LOG(LogLevel::PERMISSIVE, "Starting Index: ", index);


    // DEBUG_LOG(LogLevel::PERMISSIVE, "Current Value: ", current);


    for (int i = index + 1; i < lastIndex; ++i) {
        // DEBUG_LOG(LogLevel::PERMISSIVE, "Entered For Loop In ChainOperation::evaluate i =", i);
        const auto& elem = elements[i];
        if (current.isClassInstance()) {
            // DEBUG_LOG(LogLevel::PERMISSIVE, "Converting To ClassInstance In Loop");
            auto callable = std::get<SharedPtr<Callable>>(current.getValue());
            auto instance = std::static_pointer_cast<ClassInstance>(callable);
            current = instance->getField(elem.name, elem.type);
            currentScope = instance->getInstanceScope();
        } else {
            current = elem.object->evaluate(currentScope);
            // if (elem.object->getBranch() == "Callable") {
            //     auto type = elem.object->getAstType();
            //     switch (type)
            //     {
            //     case AstType::FunctionCall:
            //     case AstType::ClassMethodCall:
            //         std::static_pointer_cast<CallableNode>(current);
            //         break;
                
            //     default:
            //         break;
            //     }
            // } 
            currentScope = elem.object->getScope();
        }
    }


    // const auto& last = elements.back();
    // DEBUG_LOG(LogLevel::PERMISSIVE, "STARTING CASES in ChainOperation::evaluate");
    // DEBUG_LOG(LogLevel::PERMISSIVE, current.getTypeAsString());
    switch (opKind) {
        case ChainOpKind::Reference:
            if (current.isClassInstance()) {
                // DEBUG_LOG(LogLevel::PERMISSIVE, "Is a class instance");
                auto callable = std::get<SharedPtr<Callable>>(current.getValue());
                callable->getCapturedScope();
                auto instance = std::static_pointer_cast<ClassInstance>(callable);
                // DEBUG_LOG(LogLevel::PERMISSIVE, "ClassInstance Converted");
                if (last.type == TokenType::Variable){
                    current = instance->getField(last.name, last.type);
                    if (callable->getCapturedScope() != instance->getCapturedScope()){
                        throw MerkError("Callable and Instance Captured Scopes are not the same");
                    }
                    instance->getCapturedScope()->printChildScopes(20);
                    instance->getInstanceScope()->printChildScopes(20);

                    // DEBUG_LOG(LogLevel::PERMISSIVE, "Filed: ", last.name, "Gotten");
                } 
                else if (last.type == TokenType::FunctionCall) {
                    auto objectType = last.object->getAstType();
                    
                    if (objectType != AstType::FunctionCall && objectType != AstType::ClassMethodCall && objectType != AstType::FunctionReference) {
                        // DEBUG_LOG(LogLevel::PERMISSIVE, "AstType: ", last.object->getAstTypeAsString(), ", TokenType: ", tokenTypeToString(last.type));
                        throw MerkError("Type Mismatch in ChainOperation::evaluate between TokenType and AstType");
                    }

                    else {
                        // DEBUG_LOG(LogLevel::PERMISSIVE, "AstType: ", last.object->getAstTypeAsString());
                        auto* obj = dynamic_cast<CallableCall*>(last.object.get());
                        if (!obj) {
                            throw std::runtime_error("Failed to cast to CallableCall!");
                        }
                        auto& args = obj->arguments;
                        Vector<Node> arguments;

                        for (auto& arg: args) {
                            arguments.emplace_back(arg->evaluate());
                        }
                        // DEBUG_LOG(LogLevel::PERMISSIVE, "Arguments Evaluated For Method: ", last.name);
                        auto methodSig = instance->call(last.name, arguments);
                        return methodSig;
                        // return Node("NULL");
                    }
                    current = last.object->evaluate(currentScope);
                } 
                else {
                    // DEBUG_LOG(LogLevel::PERMISSIVE, last.object->getAstTypeAsString());
                    throw MerkError("Not A Valid Type For ChainOperation::evaluate -> Reference");
                }
                
                
            }

            // DEBUG_LOG(LogLevel::PERMISSIVE, "Returning Variable Reference");
            return current;

        case ChainOpKind::Assignment: {
            // DEBUG_LOG(LogLevel::PERMISSIVE, "Entered ChainOperation::evaluate -> Assignment");

            Node val = rhs->evaluate(currentScope);
            if (current.isClassInstance()) {
                auto instance = std::get<SharedPtr<ClassInstance>>(current.getValue());
                instance->updateField(last.name, val);
            } else {
                currentScope->updateVariable(last.name, val);
            }
            // DEBUG_LOG(LogLevel::PERMISSIVE, "Returning Assignment -> probably Null");

            DEBUG_FLOW_EXIT();
            return val;
        }

        case ChainOpKind::Declaration: {
            // DEBUG_LOG(LogLevel::PERMISSIVE, "Entered ChainOperation::evaluate -> Declaration");
            Node val = rhs->evaluate(currentScope);
            VarNode var(val, isConst, isMutable);
            // if (!current.isClassInstance()) {
            //     DEBUG_FLOW_EXIT();
            //     throw MerkError("Chain declaration target is not a class instance.");
            // }
            if (current.isClassInstance()) {
                auto instance = std::get<SharedPtr<ClassInstance>>(current.getValue());
                instance->declareField(last.name, var);
            } else {
                currentScope->debugPrint();
                currentScope->printChildScopes();
                auto variable = makeUnique<VarNode>(val, isConst, isMutable, isStatic);
                currentScope->declareVariable(last.name, std::move(variable));
            }

            // DEBUG_LOG(LogLevel::PERMISSIVE, "Returning Declaration -> probably Null");

            DEBUG_FLOW_EXIT();
            return var;
        }
    }
    DEBUG_FLOW_EXIT();
    throw MerkError("Unknown chain operation kind.");
}

String ChainOperation::toString() const {
    String kind = opKindAsString(opKind);
 
    return "ChainOperation(kind: " + kind + ", lhs: " + lhs->toString() + ")";
}

void ChainOperation::printAST(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "ChainOperation(" << toString() << ")\n";
    lhs->printAST(os, indent + 2);
    if (rhs) rhs->printAST(os, indent + 2);
}

UniquePtr<BaseAST> ChainOperation::clone() const {
    return makeUnique<ChainOperation>(
        static_unique_ptr_cast<Chain>(lhs->clone()),
        rhs ? static_unique_ptr_cast<ASTStatement>(rhs->clone()) : nullptr,
        opKind, getScope(), isConst, isMutable);
}

Vector<const BaseAST*> ChainOperation::getAllAst(bool includeSelf) const {
    Vector<const BaseAST*> result;
    if (includeSelf) result.push_back(this);
    auto lhsNodes = lhs->getAllAst(includeSelf);
    auto rhsNodes = rhs ? rhs->getAllAst(includeSelf) : Vector<const BaseAST*>();
    result.insert(result.end(), lhsNodes.begin(), lhsNodes.end());
    result.insert(result.end(), rhsNodes.begin(), rhsNodes.end());
    return result;
}





UniquePtr<Chain>& ChainOperation::getLeft() {
    return lhs;
};
UniquePtr<ASTStatement>& ChainOperation::getRight() {
    return rhs;
}
Chain* ChainOperation::getLeftSide() const {
    return lhs.get();
}
ASTStatement* ChainOperation::getRightSide() const {
    return rhs.get();
}



SharedPtr<Scope> ChainOperation::getSecondaryScope() {
    if (auto scope = classScope.lock()) {
        return scope;
    }
    return nullptr;
}
void ChainOperation::setSecondaryScope(SharedPtr<Scope> scope) {
    classScope = scope;
}

void ChainOperation::setResolutionMode(ResolutionMode newMode, String accessor) {
    // lhs->setResolutionMode(newMode);
    if (lhs->getElement(0).name == accessor) {
        lhs->setResolutionMode(newMode);
    }
    if (rhs && rhs->getAstType() == AstType::Chain) {
        Chain* chainR = static_cast<Chain*>(getRightSide());
        if (chainR->getElement(0).name == accessor) {
            chainR->setResolutionMode(newMode);
        }
    } 
}
void ChainOperation::setResolutionStartIndex(int index, String accessor) {
    if (lhs->getElement(0).name == accessor) {
        lhs->setResolutionStartIndex(index);
    }
    if (rhs && rhs->getAstType() == AstType::Chain) {
        Chain* chainR = static_cast<Chain*>(getRightSide());
        if (chainR->getElement(0).name == accessor) {
            chainR->setResolutionStartIndex(index);
        }
    }    
}

ResolutionMode ChainOperation::getResolutionMode() { return resolutionMode; }

void ChainOperation::setResolutionMethod(int index, ResolutionMode newMode, SharedPtr<Scope> newScope, String accessor) {
    // if (lhs->getElements().empty()) {
        
    // }
    if (!lhs->getElements().empty() && lhs->getElement(0).name == accessor) {
        lhs->setResolutionStartIndex(index);
        lhs->setResolutionMode(newMode);
        lhs->setSecondaryScope(newScope);

        setResolutionStartIndex(index, accessor);
        setResolutionMode(newMode, accessor);
        setSecondaryScope(newScope);
        // DEBUG_LOG(LogLevel::PERMISSIVE, "SET THE lhs' handling context");
    }

    if (rhs && rhs->getAstType() == AstType::Chain) {
        Chain* chainR = static_cast<Chain*>(getRightSide());
        if (!chainR->getElements().empty() && chainR->getElement(0).name == accessor) {
            chainR->setResolutionStartIndex(index);
            chainR->setResolutionMode(newMode);
            chainR->setSecondaryScope(newScope);
            // DEBUG_LOG(LogLevel::PERMISSIVE, "SET THE rhs' handling context");

        }
    }

    // resolutionStartIndex = index;
    // resolutionMode = newMode;
    // classScope = newScope;
    // setResolutionStartIndex(index, accessor);
    // setResolutionMode(newMode, accessor);
    // setSecondaryScope(newScope);
}