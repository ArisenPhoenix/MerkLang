#ifndef AST_VALIDATE_H
#define AST_VALIDATE_H

#include <functional>
#include "core/types.h"
#include "ast/ast_base.h"


// // ast_utils.h
// #ifndef AST_UTILS_H
// #define AST_UTILS_H

// #include "core/types.h"
// #include "ast/ast_base.h"
// #include <functional>
// #include <vector>

// static thread_local std::unordered_set<const BaseAST*> visited;


struct ASTUtils {
    static void traverse(const Vector<UniquePtr<BaseAST>>&, const std::function<void(BaseAST*)>&, bool recursive = true, bool includeSelf = true, std::unordered_set<const BaseAST*> *visited = nullptr);
    static void traverse(const Vector<const BaseAST*>&, const std::function<void(const BaseAST*)>&, bool recursive = true, bool includeSelf = true, std::unordered_set<const BaseAST*> *visited = nullptr);

    static bool traverseUntil(const Vector<UniquePtr<BaseAST>>&, const std::function<bool(BaseAST*)>&, bool recursive = true, bool includeSelf = true, std::unordered_set<const BaseAST*> *visited = nullptr);
    static bool traverseUntil(const Vector<const BaseAST*>&, const std::function<bool(const BaseAST*)>&, bool recursive = true, bool includeSelf = true, std::unordered_set<const BaseAST*> *visited = nullptr);

    static Vector<const BaseAST*> collectMatching(const Vector<UniquePtr<BaseAST>>&, const std::function<bool(const BaseAST*)>&, bool recursive = true, bool includeSelf = true);
    static Vector<const BaseAST*> collectMatching(const Vector<const BaseAST*>&, const std::function<bool(const BaseAST*)>&, bool recursive = true, bool includeSelf = true);
};



void validateNoImplicitAccessor(const Vector<UniquePtr<BaseAST>>& nodes, const String& accessor, const String& contextName);
void validateNoImplicitAccessor(const Vector<const BaseAST*>& nodes, const String& accessor, const String& contextName);

// void validateNoImplicitAccessor(const BaseAST* node, const String& accessor, const String& contextName);
// void traverse(const BaseAST* node, std::function<bool(const BaseAST* node)> visit);

// bool traverseUntil(std::function<bool(const BaseAST*)> condition);
// const BaseAST* getFirstMatch(std::function<bool(const BaseAST*)> predicate);

// Vector<const BaseAST*> getAllMatching(std::function<bool(const BaseAST*)> filter) {
//     Vector<const BaseAST*> results;
//     traverse([&](const BaseAST* node) {
//         if (filter(node)) results.push_back(node);
//         return true; // always continue
//     });
//     return results;
// }

#endif // AST_VALIDATE_H
 