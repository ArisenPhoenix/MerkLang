#include "utilities/debugger.h"
#include "core/errors.h"
#include "core/Scope.hpp" 
#include "core/callables/classes/Method.hpp"

#include "ast/AstBase.hpp"
#include "ast/AstControl.hpp"
#include "ast/AstCallable.hpp"
#include "ast/AstChain.hpp"
#include "ast/AstClass.hpp"
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