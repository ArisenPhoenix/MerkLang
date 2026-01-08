#include "core/types.h"
#include "core/helpers/class_helpers.h"

#include "core/node/ParamNode.hpp"

#include "core/Scope.hpp"
#include "core/callables/classes/ClassBase.hpp"
#include "core/callables/classes/Method.hpp"
#include "core/node/Node.hpp"
#include "core/errors.h"

#include "ast/AstBase.hpp"
#include "ast/Ast.hpp"
#include "ast/Exceptions.hpp"
#include "ast/AstChain.hpp"
#include "ast/AstCallable.hpp"
#include "ast/AstClass.hpp"
#include "ast/ast_validate.h"

bool handleChain(Chain* chain, ParamList params, String accessor, String name, SharedPtr<Scope> classScope) {
    DEBUG_FLOW(FlowLevel::LOW);
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
        chain->setResolutionMode(ResolutionMode::ClassInstance);
        hasStatic = true;
    }

    if (!elems.empty()) {
        DEBUG_LOG(LogLevel::TRACE, "First element in Chain is: ", elems[0].name, " (expecting: ", accessor, ")");
    }

    DEBUG_FLOW_EXIT();
    return hasStatic;
}


Vector<Chain*> applyAccessorScopeFix(MethodDef* methodDef, SharedPtr<Scope> classScope, const String& accessor) {
    DEBUG_FLOW(FlowLevel::LOW);

    if (!classScope) {
        throw MerkError("ApplyAccessorFix classScope is null");
    }

    auto& params = methodDef->getParameters();
    bool isAccessorDeclared = !params.empty() && params[0].getName() == accessor;
    Vector<Chain*> nonStaticElements = {};
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

            // Right side (if exists)
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
    auto allAst2 = methodDef->getBody()->getAllAst(true);
    auto instanceReturnReferences = ASTUtils::collectMatching(
        allAst2,
        [](const BaseAST* node) {
            return node->getAstType() == AstType::Return;
        },
        false, false  // no recursion needed here — we've already done it via getAllAst(true)
    );

    for (const BaseAST* rawReturnRef : instanceReturnReferences) {
        auto* rawReturn = const_cast<Return*>(static_cast<const Return*>(rawReturnRef));
        auto& returnExpr = rawReturn->getValue();  // UniquePtr<ASTStatement>&

        if (!returnExpr) continue;

        if (returnExpr->getAstType() == AstType::VariableReference) {
            auto* varRef = static_cast<VariableReference*>(returnExpr.get());

            if (varRef->getName() == accessor) {
                if (!isAccessorDeclared) {
                    throw MerkError("Method '" + methodDef->getName() + "' returns '" + accessor +
                                    "', but does not declare it as a parameter.");
                }
                // Replace VariableReference("self") with Accessor("self")
                returnExpr = makeUnique<Accessor>(accessor, classScope);
            }
        }
    }






    DEBUG_FLOW_EXIT();
    return nonStaticElements;
}


void fixupClassChains(SharedPtr<Scope> classScope, String accessor) {
    for (const auto& [name, signatures] : classScope->getFunctionRegistry()->getFunctions()) {
        for (const auto& sigPtr : signatures) {
            auto callable = sigPtr->getCallable();
            if (!callable || callable->getCallableType() != CallableType::METHOD) continue;

            auto method = std::dynamic_pointer_cast<Method>(callable);
            if (!method) continue;

            auto* body = method->getInvocableBody();
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
    DEBUG_FLOW(FlowLevel::LOW);
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