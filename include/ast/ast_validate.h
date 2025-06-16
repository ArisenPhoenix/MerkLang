#ifndef AST_VALIDATE_H
#define AST_VALIDATE_H

#include <functional>
#include "core/types.h"
#include "ast/ast_base.h"


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

#endif // AST_VALIDATE_H
 