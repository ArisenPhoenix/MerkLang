#include "core/types.h"
#include "core/helpers/class_helpers.h"

#include "core/scope.h"
#include "core/classes/class_base.h"
#include "core/classes/method.h"
#include "core/node.h"
#include "core/errors.h"
#include "core/functions/param_node.h"

#include "ast/ast_base.h"
#include "ast/ast.h"
#include "ast/ast_chain.h"
#include "ast/ast_callable.h"
#include "ast/ast_class.h"
#include "ast/ast_validate.h"

bool handleChain(Chain* chain, ParamList params, String accessor, String name, SharedPtr<Scope> classScope) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    const auto& elems = chain->getElements();
    bool isAccessorDeclared = !params.empty() && params[0].getName() == accessor;
    bool hasStatic = false;
    if (!elems.empty() && elems[0].name == accessor) {
        if (!isAccessorDeclared){
            // Validate: is accessor declared explicitly as a parameter
            throw MerkError("Method '" + name + "' references '" + accessor +
            "' via a chain but does not declare it as a parameter.");
        }
        chain->setFirstElement(makeUnique<Accessor>(accessor, classScope), accessor);
        DEBUG_LOG(LogLevel::PERMISSIVE, "Created Accessor");
        chain->setResolutionMode(ResolutionMode::ClassInstance);
        hasStatic = true;
    }

    if (!elems.empty()) {
        DEBUG_LOG(LogLevel::PERMISSIVE, "First element in Chain is: ", elems[0].name, " (expecting: ", accessor, ")");
    }

    DEBUG_FLOW_EXIT();
    return hasStatic;
}


Vector<Chain*> applyAccessorScopeFix(MethodDef* methodDef, SharedPtr<Scope> classScope, const String& accessor) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    if (!classScope) {
        throw MerkError("ApplyAccessorFix classScope is null");
    }

    auto& params = methodDef->getParameters();
    bool isAccessorDeclared = !params.empty() && params[0].getName() == accessor;
    Vector<Chain*> nonStaticElements;
    methodDef->getBody()->printAST(std::cout);

    // const auto& bodyChildren = methodDef->getBody()->getChildren();
    auto allAst = methodDef->getBody()->getAllAst(true);

auto chains = ASTUtils::collectMatching(
    allAst,
    [](const BaseAST* node) {
        return node->getAstType() == AstType::Chain || node->getAstType() == AstType::ChainOperation;
    },
    false, false  // no recursion needed here — we've already done it via getAllAst(true)
);

for (const BaseAST* raw : chains) {
    if (raw->getAstType() == AstType::Chain) {
        auto* chain = const_cast<Chain*>(static_cast<const Chain*>(raw));
        if (!chain->getElements().empty() && chain->getElements()[0].name == accessor) {
            if (!isAccessorDeclared) {
                throw MerkError("Method '" + methodDef->getName() + "' references '" + accessor +
                                "' via a chain but does not declare it as a parameter.");
            }

            if (handleChain(chain, params, accessor, methodDef->getName(), classScope)) {
                nonStaticElements.emplace_back(chain);
            }
        }

    } else if (raw->getAstType() == AstType::ChainOperation) {
        auto* chainOp = const_cast<ChainOperation*>(static_cast<const ChainOperation*>(raw));

        // Left side
        Chain* lhs = chainOp->getLeftSide();
        if (lhs && !lhs->getElements().empty() && lhs->getElements()[0].name == accessor) {
            if (!isAccessorDeclared) {
                throw MerkError("Method '" + methodDef->getName() + "' references '" + accessor +
                                "' via a chain but does not declare it as a parameter.");
            }

            if (handleChain(lhs, params, accessor, methodDef->getName(), classScope)) {
                nonStaticElements.emplace_back(lhs);
            }
        }

        // Right side (optional)
        auto* rhs = chainOp->getRightSide();
        if (rhs && rhs->getAstType() == AstType::Chain) {
            auto* chainR = static_cast<Chain*>(rhs);
            if (!chainR->getElements().empty() && chainR->getElements()[0].name == accessor) {
                if (!isAccessorDeclared) {
                    throw MerkError("Method '" + methodDef->getName() + "' references '" + accessor +
                                    "' via a chain but does not declare it as a parameter.");
                }

                if (handleChain(chainR, params, accessor, methodDef->getName(), classScope)) {
                    nonStaticElements.emplace_back(chainR);
                }
            }
        }
    }
}


    DEBUG_FLOW_EXIT();
    return nonStaticElements;
}

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
//         DEBUG_LOG(LogLevel::PERMISSIVE, "Visiting node: ", node->getAstTypeAsString());

//         auto& params = methodDef->getParameters();
//         bool isAccessorDeclared = !params.empty() && params[0].getName() == accessor;

//         // Handle direct Chain nodes
//         if (node->getAstType() == AstType::Chain) {
//             Chain* chain = static_cast<Chain*>(node);
//             if (handleChain(chain, params, accessor, methodDef->getName(), classScope)) {
//                 nonStaticElements.emplace_back(chain);
//             }
//             // return;
//         }

//         // Also descend into ChainOperations (as you already do)
//         else if (node->getAstType() == AstType::ChainOperation) {
//             ChainOperation* chainOp = static_cast<ChainOperation*>(node);

//             if (chainOp->getRightSide() && chainOp->getRightSide()->getAstType() == AstType::Chain) {
//                 Chain* chainR = static_cast<Chain*>(chainOp->getRightSide());
//                 if (handleChain(chainR, params, accessor, methodDef->getName(), classScope)) {
//                     nonStaticElements.emplace_back(chainR);
//                 }
//             }

//             Chain* chainL = chainOp->getLeftSide();
//             if (chainL && !chainL->getElements().empty() && chainL->getElements()[0].name == accessor) {
//                 if (!isAccessorDeclared) {
//                     throw MerkError("Method '" + methodDef->getName() + "' references '" + accessor +
//                                     "' via a chain but does not declare it as a parameter.");
//                 }

//                 if (handleChain(chainL, params, accessor, methodDef->getName(), classScope)) {
//                     nonStaticElements.emplace_back(chainL);
//                 }
//             }
//         }


//     }, true, false);

    
//     DEBUG_FLOW_EXIT();
//     return nonStaticElements;
// }

void fixupClassChains(SharedPtr<Scope> classScope, String accessor) {
    for (const auto& [name, signatures] : classScope->getFunctionRegistry()->getFunctions()) {
        for (const auto& sigPtr : signatures) {
            auto callable = sigPtr->getCallable();
            if (!callable || callable->getCallableType() != CallableType::METHOD) continue;

            auto method = std::dynamic_pointer_cast<Method>(callable);
            if (!method) continue;

            auto* body = method->getBody();
            if (!body) continue;

            ASTUtils::traverse(body->getChildren(), [&](BaseAST* node) {
                if (node->getAstType() == AstType::ChainOperation) {
                    auto* chainOp = static_cast<ChainOperation*>(node);
                    if (chainOp->getResolutionMode() == ResolutionMode::Normal) {
                        chainOp->setResolutionMethod(0, ResolutionMode::ClassInstance, nullptr, accessor);
                    }
                }
            }, true, false);
        }
    }
}

void stripImplicitAccessor(MethodDef* methodDef, const String& accessor) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    ParamList& params = methodDef->getParameters();

    if (!params.empty() && params[0].getName() == accessor) {
        params.eraseByName(accessor);
        // DEBUG_LOG(LogLevel::PERMISSIVE, highlight("Stripped implicit accessor from method: " + methodDef->getName(), Colors::orange));
        methodDef->parameters = params;
        DEBUG_FLOW_EXIT();

        return;
    }

    DEBUG_FLOW_EXIT();
    
}