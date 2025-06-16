#include "utilities/debugger.h"
#include "core/errors.h"
#include "core/scope.h" 
#include "core/classes/method.h"

#include "ast/ast_base.h"
#include "ast/ast_control.h"
#include "ast/ast_callable.h"
#include "ast/ast_chain.h"
#include "ast/ast_class.h"
#include "ast/ast_validate.h"

static thread_local std::unordered_set<const BaseAST*> globalVisited;


void ASTUtils::traverse(const Vector<UniquePtr<BaseAST>>& nodes, const std::function<void(BaseAST*)>& fn, bool recursive, bool includeSelf, std::unordered_set<const BaseAST*> *visited) {
    std::unordered_set<const BaseAST*> localSet;
    if (!visited) visited = &localSet;
    for (const auto& node : nodes) {
        if (!node) {
            throw MerkError("The Node Being Attempted TO traverse does not exist");
        }
        fn(node.get());
        if (recursive) {
            for (const BaseAST* child : node->getAllAst(includeSelf)) {
                ASTUtils::traverse(Vector<const BaseAST*>{child}, [&](const BaseAST* n) {
                    fn(const_cast<BaseAST*>(n));
                }, recursive, includeSelf, visited);
            }
        }
    }
}

void ASTUtils::traverse(const Vector<const BaseAST*>& nodes, const std::function<void(const BaseAST*)>& fn, bool recursive, bool includeSelf, std::unordered_set<const BaseAST*> *visited) {
    std::unordered_set<const BaseAST*> localSet;
    if (!visited) visited = &localSet;
    for (const BaseAST* node : nodes) {
        if (visited->count(node) == 0){
            visited->insert(node);
        } else {
            DEBUG_LOG(LogLevel::ERROR, "Circular Reference To: ", node->getAstTypeAsString(), "MemoryLoc: ", node);
            throw MerkError("Circular Reference Detected in AstUtils::traverseUntil() BaseAst*");
        }
        fn(node);
        if (recursive) {
            for (const BaseAST* child : node->getAllAst(includeSelf)) {
                ASTUtils::traverse(Vector<const BaseAST*>{child}, fn, recursive, includeSelf, visited);
            }
        }
    }
    localSet.clear();
}

bool ASTUtils::traverseUntil(const Vector<UniquePtr<BaseAST>>& nodes, const std::function<bool(BaseAST*)>& predicate, bool recursive, bool includeSelf, std::unordered_set<const BaseAST*> *visited) {
    std::unordered_set<const BaseAST*> localSet;
    if (!visited) visited = &localSet;
    for (const auto& node : nodes) {
        
        if (predicate(node.get())) return true;
        if (recursive) {
            for (const BaseAST* child : node->getAllAst(includeSelf)) {
                if (ASTUtils::traverseUntil(Vector<const BaseAST*>{child}, [&](const BaseAST* n) {
                    return predicate(const_cast<BaseAST*>(n));
                }, recursive, includeSelf, visited)) {
                    return true;
                }
            }
        }
    }
    localSet.clear();

    return false;
}

bool ASTUtils::traverseUntil(const Vector<const BaseAST*>& nodes, const std::function<bool(const BaseAST*)>& predicate, bool recursive, bool includeSelf, std::unordered_set<const BaseAST*> *visited) {
    std::unordered_set<const BaseAST*> localSet;
    if (!visited) visited = &localSet;
    for (const BaseAST* node : nodes) {
        if (visited->count(node) == 0){
            visited->insert(node);
        } else {
            DEBUG_LOG(LogLevel::ERROR, "Circular Reference To: ", node->getAstTypeAsString(), "MemoryLoc: ", node);
            throw MerkError("Circular Reference Detected in AstUtils::traverseUntil() BaseAst*");
        }
        if (predicate(node)) return true;
        if (recursive) {
            for (const BaseAST* child : node->getAllAst(includeSelf)) {
                if (ASTUtils::traverseUntil(Vector<const BaseAST*>{child}, predicate, recursive, includeSelf, visited)) return true;
            }
        }
    }
    localSet.clear();

    return false;
}

Vector<const BaseAST*> ASTUtils::collectMatching(const Vector<UniquePtr<BaseAST>>& nodes, const std::function<bool(const BaseAST*)>& predicate, bool recursive, bool includeSelf) {
    Vector<const BaseAST*> matches;
    auto collect = [&](BaseAST* node) {
        if (predicate(node)) matches.push_back(node);
    };
    ASTUtils::traverse(nodes, collect, recursive, includeSelf);
    return matches;
}

Vector<const BaseAST*> ASTUtils::collectMatching(const Vector<const BaseAST*>& nodes, const std::function<bool(const BaseAST*)>& predicate, bool recursive, bool includeSelf) {
    Vector<const BaseAST*> matches;
    auto collect = [&](const BaseAST* node) {
        if (predicate(node)) matches.push_back(node);
    };
    ASTUtils::traverse(nodes, collect, recursive, includeSelf);
    return matches;
}

template <typename NodeVector>
void validator(const NodeVector& nodes, const String& accessor, const String& contextName) {
    auto runCheck = [&](const auto& nodeVec) {
        auto throwAccessError = [&](const String& via) {
            throw MerkError("Method '" + contextName + "' references '" + accessor + "' via " + via +
                            " but does not declare it as its first parameter.");
        };

        ASTUtils::traverseUntil(nodeVec, [&](const BaseAST* node) -> bool {
            switch (node->getAstType()) {
                case AstType::Chain: {
                    const auto* chain = static_cast<const Chain*>(node);
                    if (!chain->getElements().empty() && chain->getElements()[0].name == accessor)
                        throwAccessError("Chain");
                    break;
                }
                // case AstType::VariableReference: {
                //     const auto* attr = static_cast<const VariableReference*>(node);
                //     if (attr->getAccessor() == accessor)
                //         throwAccessError("VariableReference");
                //     break;
                // }
                // case AstType::VariableAssignment: {
                //     const auto* attr = static_cast<const VariableAssignment*>(node);
                //     if (attr->getAccessor() == accessor)
                //         throwAccessError("VariableAssignment");
                //     break;
                // }
                // case AstType::VariableDeclaration: {
                //     const auto* attr = static_cast<const VariableDeclaration*>(node);
                //     if (attr->getAccessor() == accessor)
                //         throwAccessError("VariableDeclaration");
                //     break;
                // }
                default: break;
            }
            return false;
        }, true, false);
    };

    runCheck(nodes);  // calls the lambda
}


void validateNoImplicitAccessor(const Vector<UniquePtr<BaseAST>>& nodes, const String& accessor, const String& contextName) {
    validator(nodes, accessor, contextName);
}
void validateNoImplicitAccessor(const Vector<const BaseAST*>& nodes, const String& accessor, const String& contextName) {
    validator(nodes, accessor, contextName);
}




// bool handleChain(Chain* chain, ParamList params, String accessor, String name, SharedPtr<Scope> classScope) {
//     DEBUG_FLOW(FlowLevel::PERMISSIVE);
//     const auto& elems = chain->getElements();
//     bool isAccessorDeclared = !params.empty() && params[0].getName() == accessor;
//     bool hasStatic = false;
//     if (!elems.empty() && elems[0].name == accessor) {
//         if (!isAccessorDeclared){
//             // Validate: is accessor declared explicitly as a parameter
//             throw MerkError("Method '" + name + "' references '" + accessor +
//             "' via a chain but does not declare it as a parameter.");
//         }
//         chain->setFirstElement(makeUnique<Accessor>(accessor, classScope));
//         chain->setResolutionMode(ResolutionMode::ClassInstance);
//         // chain->setSecondaryScope(classScope);
//         hasStatic = true;
//         // DEBUG_LOG(LogLevel::PERMISSIVE, "UPDATED CHAIN IN METHOD", name);
//     }
//     DEBUG_FLOW_EXIT();
//     return hasStatic;
// }

// Vector<Chain*> applyAccessorScopeFix(MethodDef* methodDef, SharedPtr<Scope> classScope, const String& accessor) {
//     DEBUG_FLOW(FlowLevel::PERMISSIVE);
//     // DEBUG_LOG(LogLevel::PERMISSIVE, "Checking methodDef Through applyAccesorFix: ", methodDef->toString());


//     if (!classScope) {
//         throw MerkError("ApplyAccessorFix classScope is null");
//     }
//     // bool isStatic = false;
//     Vector<Chain*> nonStaticElements;
//     const auto& bodyChildren = methodDef->getBody()->getChildren();

//     ASTUtils::traverse(bodyChildren, [&](BaseAST* node) {
//         if (node->getAstType() != AstType::ChainOperation) return;
//         auto& params = methodDef->getParameters();
//         ChainOperation* chainOp = static_cast<ChainOperation*>(node);
//         // DEBUG_LOG(LogLevel::PERMISSIVE, "Checking ChainOperation: ", chainOp->toString());
//         // DEBUG_LOG(LogLevel::PERMISSIVE, "Expected accessor: ", accessor);
//         bool isAccessorDeclared = !params.empty() && params[0].getName() == accessor;

//         Chain* chainR = nullptr;
//         if (chainOp->getRightSide() && chainOp->getRightSide()->getAstType() == AstType::Chain){
//             // Only fix RHS if it's a chain (e.g. `self.x = other.y` | self.x = self.getValue() etc.)

//             chainR = static_cast<Chain*>(chainOp->getRightSide());
            
//             if (handleChain(chainR, params, accessor, methodDef->getName(), classScope)) {
//                 nonStaticElements.emplace_back(chainR);
//             };

//         }

//         Chain* chainL = chainOp->getLeftSide();
//         const auto& elems = chainL->getElements();
//         if (!elems.empty() && elems[0].name == accessor) {
//             if (!isAccessorDeclared){
//                 // Validate: is accessor declared explicitly as a parameter

//                 throw MerkError("Method '" + methodDef->getName() + "' references '" + accessor +
//                 "' via a chain but does not declare it as a parameter.");
//             }
//             if (handleChain(chainL, params, accessor, methodDef->getName(), classScope)) {
//                 nonStaticElements.emplace_back(chainL);
//             }
            
//             // chainOp->setResolutionStartIndex(1, accessor);
//             // chainOp->setResolutionMode(ResolutionMode::ClassInstance, accessor);
//             // chainOp->setSecondaryScope(classScope);
//         }

//     }, true, false);

    
//     DEBUG_FLOW_EXIT();
//     return nonStaticElements;
// }

// void fixupClassChains(SharedPtr<Scope> classScope, String accessor) {
//     for (const auto& [name, signatures] : classScope->getFunctionRegistry()->getFunctions()) {
//         for (const auto& sigPtr : signatures) {
//             auto callable = sigPtr->getCallable();
//             if (!callable || callable->getCallableType() != CallableType::METHOD) continue;

//             auto method = std::dynamic_pointer_cast<Method>(callable);
//             if (!method) continue;

//             auto* body = method->getBody();
//             if (!body) continue;

//             ASTUtils::traverse(body->getChildren(), [&](BaseAST* node) {
//                 if (node->getAstType() == AstType::ChainOperation) {
//                     auto* chainOp = static_cast<ChainOperation*>(node);
//                     if (chainOp->getResolutionMode() == ResolutionMode::Normal) {
//                         chainOp->setResolutionMethod(0, ResolutionMode::ClassInstance, nullptr, accessor);
//                     }
//                 }
//             }, true, false);
//         }
//     }
// }

// void stripImplicitAccessor(MethodDef* methodDef, const String& accessor) {
//     DEBUG_FLOW(FlowLevel::PERMISSIVE);
//     ParamList& params = methodDef->getParameters();

//     if (!params.empty() && params[0].getName() == accessor) {
//         params.eraseByName(accessor);
//         // DEBUG_LOG(LogLevel::PERMISSIVE, highlight("Stripped implicit accessor from method: " + methodDef->getName(), Colors::orange));
//         methodDef->parameters = params;
//         DEBUG_FLOW_EXIT();

//         return;
//     }

//     DEBUG_FLOW_EXIT();
    
// }