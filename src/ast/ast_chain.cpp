#include "core/types.h"
#include "core/scope.h"
#include "core/node.h"
#include "ast/ast_base.h"

#include "utilities/debugger.h"
#include "utilities/debugging_functions.h"
#include "utilities/helper_functions.h"
#include "ast/ast_callable.h"

#include "ast/ast_function.h"
#include "ast/ast_class.h"
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

void out(std::ostream& os) {(void)os;}
void ChainElement::printAST(std::ostream& os, int indent) const {
    
    printIndent(os, indent);
    os << "Name: " << name << " TokenType: " << tokenTypeToString(type);
    object ? object->printAST(os, indent) : out(os);
};

ChainElement::ChainElement(ChainElement&& other) noexcept
  : name(std::move(other.name))
  , object(std::move(other.object))        // just steal the pointer
  , delimiter(std::move(other.delimiter))
  , type(other.type)
{}


ChainElement& ChainElement::operator=(ChainElement&& other) noexcept {
    if (this != &other) {
      name      = std::move(other.name);
      object    = std::move(other.object);    // DON’T clone!
      delimiter = std::move(other.delimiter);
      type      = other.type;
    }
    return *this;
  }

ChainElement::~ChainElement() {
    clear();
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
void Chain::setLastScope(SharedPtr<Scope> mostRecentScope) const {
    lastScope = mostRecentScope;
}



void Chain::setFirstElement(UniquePtr<ASTStatement> probablyScope, String name) {
    if (elements.size() == 0){
        throw MerkError("There are no elements to set in Chain::setFirstElement");
    }

    elements[0].object = std::move(probablyScope);
    elements[0].name = name;
}

Vector<ChainElement>& Chain::getMutableElements() {
    return elements;
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

SharedPtr<Scope> Chain::getLastScope() {
    if (auto scope = lastScope.lock()) {
        return scope;
    }
    return nullptr;
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

    


   



UniquePtr<BaseAST> ChainOperation::clone() const {
    return makeUnique<ChainOperation>(
        static_unique_ptr_cast<Chain>(lhs->clone()),
        rhs ? static_unique_ptr_cast<ASTStatement>(rhs->clone()) : nullptr,
        opKind, getScope(), isConst, isMutable);
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

void ChainOperation::setResolutionMethod(int index, ResolutionMode newMode, SharedPtr<Scope> secondaryScope, String accessor) {
    // if (lhs->getElements().empty()) {
        
    // }
    if (!lhs->getElements().empty() && lhs->getElement(0).name == accessor) {
        lhs->setResolutionStartIndex(index);
        lhs->setResolutionMode(newMode);
        lhs->setSecondaryScope(secondaryScope);

        setResolutionStartIndex(index, accessor);
        setResolutionMode(newMode, accessor);
        setSecondaryScope(secondaryScope);
        // DEBUG_LOG(LogLevel::PERMISSIVE, "SET THE lhs' handling context");
    }

    if (rhs && rhs->getAstType() == AstType::Chain) {
        Chain* chainR = static_cast<Chain*>(getRightSide());
        if (!chainR->getElements().empty() && chainR->getElement(0).name == accessor) {
            chainR->setResolutionStartIndex(index);
            chainR->setResolutionMode(newMode);
            chainR->setSecondaryScope(secondaryScope);
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



void ChainOperation::setScope(SharedPtr<Scope> newScope) {
    lhs->setScope(newScope);
    if (rhs){
        rhs->setScope(newScope);
    }
}



Node Chain::evaluate(SharedPtr<Scope> methodScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    SharedPtr<Scope> currentScope = getElements().front().object->getAstType() == AstType::Accessor ? instanceNode->getInstanceScope() : methodScope;
    DEBUG_LOG(LogLevel::PERMISSIVE, highlight("STARTING SCOPE FOR Chain::evaluate() is: " + std::to_string(currentScope->getScopeLevel()), Colors::bg_bright_green));

    if (!currentScope) {throw MerkError("Chain::evaluate: no valid scope");}
    int index = resolutionStartIndex;

    setLastScope(currentScope);
    Node currentVal;

    auto& baseElem = elements[index];
    if (baseElem.object->getAstType() == AstType::Accessor){
        currentVal = baseElem.object->evaluate(currentScope, instanceNode); // should evaluate to a ClassInstanceNode
        
    } else {
        currentVal = baseElem.object->evaluate(currentScope, instanceNode);
    }
    index ++;

    for (size_t i = index; i < elements.size(); ++i) {
        const auto& elem = elements[i];
        DEBUG_LOG(LogLevel::PERMISSIVE, "Current Val Is: ", currentVal);
        DEBUG_LOG(LogLevel::PERMISSIVE, "ELEMENT TYPE: ", elem.object->getAstTypeAsString());
        AstType objType = elem.object->getAstType();
        if (currentVal.isClassInstance()) {
            auto instance = std::get<SharedPtr<ClassInstance>>(currentVal.getValue());
            currentScope = instance->getInstanceScope();
            instanceNode = makeShared<ClassInstanceNode>(instance);
            
            // methodScope->debugPrint();

            if (objType == AstType::VariableDeclaration){
                DEBUG_LOG(LogLevel::PERMISSIVE, highlight("ABOUT TO EVALUATE VariableDeclaration -> Scopes Below", Colors::bg_green));
                currentVal = elem.object->evaluate(methodScope, instanceNode);
            } 

            else if (objType == AstType::VariableAssignment) {
                DEBUG_LOG(LogLevel::PERMISSIVE, highlight("ABOUT TO EVALUATE VariableAssignment -> Scopes Below", Colors::bg_green));
                currentVal = elem.object->evaluate(methodScope, instanceNode);
            }
            
            else if (objType == AstType::VariableReference) {
                DEBUG_LOG(LogLevel::PERMISSIVE, highlight("ABOUT TO EVALUATE VariableReference -> Scopes Below", Colors::bg_green));
                currentVal = elem.object->evaluate(currentScope, instanceNode);
            }

            else if (objType == AstType::FunctionCall || objType == AstType::ClassMethodCall) {
                DEBUG_LOG(LogLevel::PERMISSIVE, highlight("ABOUT TO EVALUATE FunctionCall/ClassMethodCall -> Scopes Below", Colors::bg_green));
                
                auto methodCall = static_unique_ptr_cast<MethodCall>(std::move(elem.object->clone()));

                try {
                    auto arguments = methodCall->handleArgs(currentScope);
                    auto name = methodCall->getName();
                    
                    // currentVal = instance->call(name, arguments);
                    auto instanceScope = instance->getInstanceScope();
                    SharedPtr<CallableSignature> callableMethodSig = instanceScope->getFunction(name, arguments);
                    auto callable = callableMethodSig->getCallable();
                    auto method = std::static_pointer_cast<Method>(callable);
                    currentVal = method->execute(arguments, method->getCapturedScope()->makeCallScope(), instanceNode);
                    method->getBody()->printAST(std::cout);

                    DEBUG_LOG(LogLevel::PERMISSIVE, "MethodName: ", elem.name);
                    DEBUG_LOG(LogLevel::PERMISSIVE, "Element AST: ");
                    elem.object->printAST(std::cout);
                } catch (VariableNotFoundError& e) {
                    DEBUG_LOG(LogLevel::PERMISSIVE, highlight("Variable NOT Found Error When Parsing Method", Colors::bg_bright_magenta));
                    int indent = 0;
                    std::ostringstream stream;
                    for (auto& elem : elements) {
                        elem.object->printAST(stream, indent);
                        indent ++;
                    }
                    throw MerkError(e.what());

                }
                
                
            }
            
            currentScope = elem.object->getScope();
            setLastScope(currentScope);

        } else {
            currentVal = elem.object->evaluate(currentScope);
            currentScope = elem.object->getScope();
            setLastScope(currentScope);
        }
    }
    DEBUG_LOG(LogLevel::PERMISSIVE, "EXITING Chain::evaluate");
    DEBUG_FLOW_EXIT();
    return currentVal;
    
}

Node ChainOperation::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    String hasLeft = getLeftSide() ? "true" : "false";
    String hasRight = getRightSide() ? "true" : "false";
    String hasInstanceNode = instanceNode ? "true" : "false";

    DEBUG_LOG(LogLevel::PERMISSIVE, "OP KIND: ", opKindAsString(opKind));


    auto leftVal = lhs->evaluate(scope, instanceNode);
    auto currentScope = lhs->getLastScope();
    auto& last = lhs->getElements().back();

    if (!currentScope) throw MerkError("There Is No Current Scope in ChainOperation::evaluate");

    SharedPtr<Scope> leftScope = lhs->getLastScope();
    SharedPtr<Scope> rightScope;
    Node rightVal;

    if (getRightSide()){
        if (getRightSide()->getAstType() == AstType::Chain){
            auto r = static_cast<Chain*>(getRightSide());
            rightVal = r->evaluate(currentScope, instanceNode);
            rightScope = r->getLastScope();
        } else {
            rightVal = getRightSide()->evaluate(currentScope, instanceNode);
        }
    } else {
        DEBUG_LOG(LogLevel::PERMISSIVE, highlight("No Right Side For ChainOperation", Colors::bg_green));
    }



    if (!leftVal.isValid()) {
        // throw MerkError("Left Hand Side of ChainOperation is invalid");
        return leftVal;
    } else {
        DEBUG_LOG(LogLevel::PERMISSIVE, "Left Hand Side of ChainOperation is Valid", leftVal);
    }
    
    switch (opKind) {
        case ChainOpKind::Reference:
            if (leftVal.isClassInstance()) {
                DEBUG_LOG(LogLevel::PERMISSIVE, "ChainOperation Reference Is a class instance");
                // auto instanceNode = static_cast<ClassInstanceNode>(leftVal);
                auto instanceNode = ClassInstanceNode::from(leftVal);
                auto instanceScope = instanceNode.getInstanceScope();

                DEBUG_LOG(LogLevel::PERMISSIVE, "ClassInstance Converted");
                if (last.type == TokenType::Variable){
                    int scopeLevel = instanceNode.isValid() ? instanceNode.getInstanceScope()->getScopeLevel() : 0;
                    DEBUG_LOG(LogLevel::PERMISSIVE, "Getting Field From Scope: ", scopeLevel);
                    // instanceNode->getInternalScope();
                    auto instance = instanceNode.getInstance();
                    
                    return instance->getField(last.name, last.type);
                } 
                // else if (last.type == TokenType::FunctionCall) {
                //     auto objectType = last.object->getAstType();
                    
                //     if (objectType != AstType::FunctionCall && objectType != AstType::ClassMethodCall && objectType != AstType::FunctionReference) {
                //         // DEBUG_LOG(LogLevel::PERMISSIVE, "AstType: ", last.object->getAstTypeAsString(), ", TokenType: ", tokenTypeToString(last.type));
                //         throw MerkError("Type Mismatch in ChainOperation::evaluate between TokenType and AstType");
                //     }

                //     else {
                //         DEBUG_LOG(LogLevel::PERMISSIVE, "AstType: ", last.object->getAstTypeAsString());
                //         auto* obj = dynamic_cast<CallableCall*>(last.object.get());
                //         if (!obj) {
                //             throw std::runtime_error("Failed to cast to CallableCall!");
                //         }
                //         auto& args = obj->arguments;
                //         Vector<Node> arguments;
                //         auto instance = instanceNode.getInstance();
                //         auto sharedInstanceNode = makeShared<ClassInstanceNode>(instanceNode);
                //         for (auto& arg: args) {
                //             auto evaluated = arg->evaluate(currentScope, sharedInstanceNode);
                //             arguments.emplace_back(evaluated);
                //         }
                //         // DEBUG_LOG(LogLevel::PERMISSIVE, "Arguments Evaluated For Method: ", last.name);
                //         auto methodSig = instance->call(last.name, arguments);
                //         return methodSig;
                //         // return Node("NULL");
                //     }
                //     auto instance = instanceNode.getInstance();
                //     auto sharedInstanceNode = makeShared<ClassInstanceNode>(instanceNode);
                //     leftVal = last.object->evaluate(currentScope, sharedInstanceNode);
                //     return leftVal;
                // } 
                // else {
                //     // DEBUG_LOG(LogLevel::PERMISSIVE, last.object->getAstTypeAsString());
                //     throw MerkError("Not A Valid Type For ChainOperation::evaluate -> Reference");
                // }
                
                
            }

            // DEBUG_LOG(LogLevel::PERMISSIVE, "Returning Variable Reference");
            return leftVal;

        case ChainOpKind::Assignment: 
            DEBUG_LOG(LogLevel::PERMISSIVE, "Entered ChainOperation::evaluate -> Assignment");
            DEBUG_LOG(LogLevel::PERMISSIVE, "CURRENT LEFT VAL: ", leftVal);
            // Node val = rhs->evaluate(currentScope, instanceNode);
            if (leftVal.isClassInstance()) {
                auto instance = std::get<SharedPtr<ClassInstance>>(leftVal.getValue());
                instance->updateField(last.name, rightVal);
            } else {
                currentScope->updateVariable(last.name, leftVal);
            }
            // DEBUG_LOG(LogLevel::PERMISSIVE, "Returning Assignment -> probably Null");

            DEBUG_FLOW_EXIT();
            return rightVal;
        // }

        case ChainOpKind::Declaration: 
            DEBUG_LOG(LogLevel::PERMISSIVE, "Entered ChainOperation::evaluate -> Declaration");
            if (rhs){
                throw MerkError("There Shouldn't Be An RHS for A DECLARATION");
            }
            
            // Node val = rhs->evaluate(currentScope);
            // VarNode var(val, isConst, isMutable);
            // if (leftVal.isClassInstance()) {
            //     auto instance = std::get<SharedPtr<ClassInstance>>(leftVal.getValue());
            //     instance->declareField(last.name, var);
            // } else {
            //     currentScope->debugPrint();
            //     currentScope->printChildScopes();
            //     auto variable = makeUnique<VarNode>(val, isConst, isMutable, isStatic);
            //     currentScope->declareVariable(last.name, std::move(variable));
            // }

            // // DEBUG_LOG(LogLevel::PERMISSIVE, "Returning Declaration -> probably Null");

            // DEBUG_FLOW_EXIT();
            // return var;
            DEBUG_LOG(LogLevel::PERMISSIVE, highlight("VAR CREATED: ", Colors::bg_green), leftVal, "IN SCOPE: ");
            // scope->debugPrint();
            return leftVal;
        // }
    }
    DEBUG_FLOW_EXIT();
    throw MerkError("Unknown chain operation kind.");

}