#include "core/types.h"

#include "core/scope.h"
#include "core/classes/class_base.h"
#include "core/classes/method.h"

#include "ast/ast_base.h"
#include "ast/ast.h"
#include "ast/ast_chain.h"
#include "ast/ast_class.h"

bool handleChain(Chain* chain, ParamList params, String accessor, String name, SharedPtr<Scope> classScope);

Vector<Chain*> applyAccessorScopeFix(MethodDef* methodDef, SharedPtr<Scope> classScope, const String& accessor);

void fixupClassChains(SharedPtr<Scope> classScope, String accessor);

void stripImplicitAccessor(MethodDef* methodDef, const String& accessor);