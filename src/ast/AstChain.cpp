

#include "core/node/Node.hpp"
#include "core/node/ArgumentNode.hpp"
// #include "core/types.h"
#include "core/TypesFWD.hpp"
#include "core/Scope.hpp"


#include "ast/AstBase.hpp"

#include "utilities/debugger.h"
#include "utilities/debugging_functions.h"
#include "utilities/helper_functions.h"
#include "ast/AstCallable.hpp"

#include "ast/AstFunction.hpp"
#include "ast/AstClass.hpp"
#include "ast/ast_helpers.h"
#include "core/callables/classes/ClassBase.hpp"
#include "core/callables/classes/Method.hpp"
#include "ast/AstChain.hpp"

#include "core/errors.h"
#include "core/Scope.hpp"


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

// void out(std::ostream& os) {(void)os;}


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
    if (lastScope) {lastScope.reset();}
    if (getSecondaryScope()){
        getSecondaryScope().reset();
    }
}
Chain::~Chain() { clear(); }
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


// SharedPtr<Scope> getAssociatedScope(const Node& node) {
//     switch (node.getType()) {
//         case NodeValueType::Class:
//             return std::get<SharedPtr<ClassBase>>(node.getValue())->getCapturedScope();
//         case NodeValueType::Function:
//             return std::get<SharedPtr<Function>>(node.getValue())->getCapturedScope();
//         case NodeValueType::Method:
//             return std::get<SharedPtr<Method>>(node.getValue())->getCapturedScope();

//         case NodeValueType::ClassInstance: {
//             auto instance = std::get<SharedPtr<ClassInstance>>(node.getValue());
//             DEBUG_LOG(LogLevel::TRACE, "getAssociatedScope -> ClassInstance scope resolved");
//             return std::get<SharedPtr<ClassInstance>>(node.getValue())->getCapturedScope();
//         }

            
//         default:
//             return nullptr;  // Caller should handle this case
//     }
// }

void Chain::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    // scope = newScope;
    // for (auto& elem : elements) { if (elem.object) {elem.object->setScope(newScope);} }
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
    copy->setResolutionStartIndex(getResolutionStartIndex());

    copy->setResolutionMode(getResolutionMode());
    copy->setSecondaryScope(getSecondaryScope());
    return copy;
}

SharedPtr<Scope> Chain::getLastScope() {
    // if (auto scope = lastScope.lock()) {
    //     return scope;
    // }
    // return nullptr;
    return lastScope;
}

SharedPtr<Scope> Chain::getLastScope() const {
    // if (auto scope = lastScope.lock()) {
    //     return scope;
    // }
    // return nullptr;
    return lastScope;
}

void Chain::replaceLastElementWith(ChainElement&& elem) {
    if (elements.empty()) {
        elements[0] = std::move(elem);
    }
    else {
        elements.back() = std::move(elem);
    }
    
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

    if (!lhs->getElements().empty() && lhs->getElement(0).name == accessor) {
        lhs->setResolutionStartIndex(index);
        lhs->setResolutionMode(newMode);
        lhs->setSecondaryScope(secondaryScope);

        setResolutionStartIndex(index, accessor);
        setResolutionMode(newMode, accessor);
        setSecondaryScope(secondaryScope);
    }

    if (rhs && rhs->getAstType() == AstType::Chain) {
        Chain* chainR = static_cast<Chain*>(getRightSide());
        if (!chainR->getElements().empty() && chainR->getElement(0).name == accessor) {
            chainR->setResolutionStartIndex(index);
            chainR->setResolutionMode(newMode);
            chainR->setSecondaryScope(secondaryScope);
        }
    }
}



void ChainOperation::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    // scope = newScope;
    // lhs->setScope(newScope);
    // if (rhs){
    //     rhs->setScope(newScope);
    // }
}



Node Chain::evaluate(SharedPtr<Scope> methodScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    SharedPtr<Scope> currentScope = getElements().front().object->getAstType() == AstType::Accessor ? instanceNode->getInstanceScope() : methodScope;
    
    if (!currentScope) {throw MerkError("Chain::evaluate: no valid scope");}
    int index = resolutionStartIndex;
    
    setLastScope(currentScope);
    Node currentVal;

    auto& baseElem = elements[index];
    if (baseElem.object->getAstType() == AstType::Accessor){
        currentVal = baseElem.object->evaluate(currentScope, instanceNode); // should evaluate to a ClassInstanceNode
        // if (!currentVal.isInstance()) {throw MerkError("Pulled Out A Non Instance Instace from currentVal");}
    } else {
        currentVal = baseElem.object->evaluate(currentScope, instanceNode);
        // if (!currentVal.isInstance()) {throw MerkError("Pulled Out A Non Instance Instace from currentVal");}
    }
    index ++;

    if (baseElem.object->getAstType() == AstType::Unknown) {throw MerkError("Unknown AST Type Found In Chain...see above");}

    for (size_t i = index; i < elements.size(); ++i) {
        const auto& elem = elements[i];
        AstType objType = elem.object->getAstType();

        if (objType == AstType::Unknown) {throw MerkError("Unknown AST Type Found In Chain...see above");}
        if (!currentVal.isValid()) {throw MerkError("Cannot Chain off of a null return value");}

        if (currentVal.isInstance()) {
            auto instance = currentVal.toInstance();
            currentScope = instance->getInstanceScope();
            if (!currentScope) {throw MerkError("Scope Invalid During Chain Iteration");}
            // if (!instanceNode) {methodScope->debugPrint(); methodScope->printChildScopes(); throw MerkError("An InstanceNode was not passed to Chain::evaluate");}
            instanceNode = instance->getInstanceNode();


            switch (objType)
            {
            case AstType::VariableDeclaration:
                {
                    auto varDec = static_cast<VariableDeclaration*>(elem.object.get());
                    auto varName = varDec->getName();
                    
                    auto res = varDec->getExpression()->evaluate(methodScope, instanceNode);
                    
                    instance->declareField(varName, res);
                    currentScope->addMember(varName);  // ensures downstream logic will correctly resolve

                    currentVal = res;

                    break;
                }
            case AstType::VariableAssignment:
                {
                    auto varAss = static_cast<VariableAssignment*>(elem.object.get());
                    auto varName = varAss->getName();
                    
                    currentVal = varAss->getExpression()->evaluate(methodScope, instanceNode);
                    
                    instance->updateField(varName, currentVal);

                    break;
                }
            case AstType::VariableReference:
                {
                    
                    auto varRef = static_cast<VariableReference*>(elem.object.get());
                    auto varName = varRef->getName();
                    
                    currentVal = instance->getField(varRef->getName());
                    break;
                }
            
            case AstType::ClassMethodCall:
                {
                    
                    if (!instanceNode) {throw MerkError("No InstanceNode in Chain::evaluate -> ClassMethodCall");}
                    currentVal = elem.object->evaluate(methodScope, instanceNode);
                    // if (currentVal.isInstance()) {instanceNode = currentVal.toInstance()->getInstanceNode();}

                    break;
                }
                
            default:
                currentVal = elem.object->evaluate(currentScope, instanceNode);
                currentScope = elem.object->getScope();
                
                break;
            }
            if (!currentScope) {throw MerkError("Scope Invalid After Chain Iteration");}

        } else {
            if (elem.object->getAstType() == AstType::ClassMethodCall) { // For Handling ad-hoc/virtual methods that are not actually on the object i.e (clone, etc.)
                DEBUG_LOG(LogLevel::PERMISSIVE, currentVal.toString(), currentVal.getFlags().toString());
                currentScope->debugPrint();
                currentScope->printChildScopes();
                throw MerkError("Hit it! : " + elem.object->toString());
                currentVal = handleVirtualMethod(currentVal, elem.name);                
            } else {
                throw MerkError("Evaluating Some other structure, the current Value is: " + nodeTypeToString(DynamicNode::getTypeFromValue(currentVal.getValue())) + " META: " + currentVal.getFlags().toString());
            }
            
            DEBUG_LOG(LogLevel::DEBUG, highlight("Else OBJECTS AST TYPE: ", Colors::red), astTypeToString(elem.object->getAstType()));            
            currentScope = elem.object->getScope();
            
        }
        
        if (currentVal.isInstance()) {instanceNode = currentVal.toInstance()->getInstanceNode();}
        setLastScope(currentScope);
    }
    setLastScope(currentScope);
    if (!getLastScope()) {throw MerkError("No Last Scope Before Completing Chain::evaluate");}
    DEBUG_FLOW_EXIT();
    return currentVal;
    
}

Node ChainOperation::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    String hasLeft = getLeftSide() ? "true" : "false";
    String hasRight = getRightSide() ? "true" : "false";
    String hasInstanceNode = instanceNode ? "true" : "false";

    DEBUG_LOG(LogLevel::TRACE, "OP KIND: ", opKindAsString(opKind));


    auto leftVal = lhs->evaluate(scope, instanceNode);
    auto currentScope = lhs->getLastScope();

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
    }



    // if (!leftVal.isValid()) {
    //     // throw MerkError("Left Hand Side of ChainOperation is invalid");
    //     DEBUG_LOG(LogLevel::TRACE, "Left Hand Side of ChainOperation is INVALID", leftVal);

    // } else {
    //     DEBUG_LOG(LogLevel::TRACE, "Left Hand Side of ChainOperation is Valid", leftVal);
    // }
    


    
    DEBUG_FLOW_EXIT();
    // throw MerkError("Unknown chain operation kind.");
    return leftVal;
}


// //                      For Perhaps More Advanced Maneuvers


    // switch (opKind) {
    //     case ChainOpKind::Reference:
    //         if (leftVal.isClassInstance()) {
    //             // auto instanceNode = static_cast<ClassInstanceNode>(leftVal);
    //             auto instanceNode = ClassInstanceNode::from(leftVal);
    //             auto instanceScope = instanceNode.getInstanceScope();

    //             if (last.type == TokenType::Variable){
    //                 int scopeLevel = instanceNode.isValid() ? instanceNode.getInstanceScope()->getScopeLevel() : 0;
    //                 // instanceNode->getInternalScope();
    //                 auto instance = instanceNode.getInstance();
                    
    //                 // return instance->getField(last.name, last.type);
    //             } 
    //         }

    //         return leftVal;

    //     case ChainOpKind::Assignment: 

    //         // Node val = rhs->evaluate(currentScope, instanceNode);
    //         if (leftVal.isClassInstance()) {
    //             auto instance = std::get<SharedPtr<ClassInstance>>(leftVal.getValue());
    //             instance->updateField(last.name, rightVal);
    //         } else {
    //             currentScope->updateVariable(last.name, rightVal);
    //         }

    //         DEBUG_FLOW_EXIT();
    //         return rightVal;
    //     // }

    //     case ChainOpKind::Declaration: 
    //         if (rhs){
    //             throw MerkError("There Shouldn't Be An RHS for A DECLARATION");
    //         }
            
    //         // Node val = rhs->evaluate(currentScope);
    //         // VarNode var(val, isConst, isMutable);
    //         // if (leftVal.isClassInstance()) {
    //         //     auto instance = std::get<SharedPtr<ClassInstance>>(leftVal.getValue());
    //         //     instance->declareField(last.name, var);
    //         // } else {
    //         //     currentScope->debugPrint();
    //         //     currentScope->printChildScopes();
    //         //     auto variable = makeUnique<VarNode>(val, isConst, isMutable, isStatic);
    //         //     currentScope->declareVariable(last.name, std::move(variable));
    //         // }


    //         // DEBUG_FLOW_EXIT();
    //         // return var;
    //         // scope->debugPrint();
    //         return leftVal;
    //     // }
    // }