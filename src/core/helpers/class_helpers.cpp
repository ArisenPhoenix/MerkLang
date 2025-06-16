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
        chain->setFirstElement(makeUnique<Accessor>(accessor, classScope));
        chain->setResolutionMode(ResolutionMode::ClassInstance);
        // chain->setSecondaryScope(classScope);
        hasStatic = true;
        // DEBUG_LOG(LogLevel::PERMISSIVE, "UPDATED CHAIN IN METHOD", name);
    }
    DEBUG_FLOW_EXIT();
    return hasStatic;
}

Vector<Chain*> applyAccessorScopeFix(MethodDef* methodDef, SharedPtr<Scope> classScope, const String& accessor) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    // DEBUG_LOG(LogLevel::PERMISSIVE, "Checking methodDef Through applyAccesorFix: ", methodDef->toString());


    if (!classScope) {
        throw MerkError("ApplyAccessorFix classScope is null");
    }
    // bool isStatic = false;
    Vector<Chain*> nonStaticElements;
    const auto& bodyChildren = methodDef->getBody()->getChildren();

    ASTUtils::traverse(bodyChildren, [&](BaseAST* node) {
        if (node->getAstType() != AstType::ChainOperation) return;
        auto& params = methodDef->getParameters();
        ChainOperation* chainOp = static_cast<ChainOperation*>(node);
        // DEBUG_LOG(LogLevel::PERMISSIVE, "Checking ChainOperation: ", chainOp->toString());
        // DEBUG_LOG(LogLevel::PERMISSIVE, "Expected accessor: ", accessor);
        bool isAccessorDeclared = !params.empty() && params[0].getName() == accessor;

        Chain* chainR = nullptr;
        if (chainOp->getRightSide() && chainOp->getRightSide()->getAstType() == AstType::Chain){
            // Only fix RHS if it's a chain (e.g. `self.x = other.y` | self.x = self.getValue() etc.)

            chainR = static_cast<Chain*>(chainOp->getRightSide());
            
            if (handleChain(chainR, params, accessor, methodDef->getName(), classScope)) {
                nonStaticElements.emplace_back(chainR);
            };

        }

        Chain* chainL = chainOp->getLeftSide();
        const auto& elems = chainL->getElements();
        if (!elems.empty() && elems[0].name == accessor) {
            if (!isAccessorDeclared){
                // Validate: is accessor declared explicitly as a parameter

                throw MerkError("Method '" + methodDef->getName() + "' references '" + accessor +
                "' via a chain but does not declare it as a parameter.");
            }
            if (handleChain(chainL, params, accessor, methodDef->getName(), classScope)) {
                nonStaticElements.emplace_back(chainL);
            }
            
            // chainOp->setResolutionStartIndex(1, accessor);
            // chainOp->setResolutionMode(ResolutionMode::ClassInstance, accessor);
            // chainOp->setSecondaryScope(classScope);
        }

    }, true, false);

    
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