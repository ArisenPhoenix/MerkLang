#ifndef CLASS_BUILTIN_H
#define CLASS_BUILTIN_H

#include "core/callables/classes/native_class.h"
#include "core/callables/classes/class_base.h"
#include "core/callables/param_node.h"
#include "core/callables/classes/native_class.h"

// class Scope;
// class Callable;

// class ScopeCallBackNode : public BaseAST {
//     NodeList args;
//     std::function<Node(NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self, String)> methodFn;

// public:
//     String otherData;
//     ScopeCallBackNode(NodeList, SharedPtr<Scope> callScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> self = nullptr);
//     ~ScopeCallBackNode();
//     Node evaluate(SharedPtr<Scope>, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;
//     AstType getAstType() const override {return AstType::Base;}
//     void setScope(SharedPtr<Scope> newScope) {}
//     SharedPtr<Scope> getScope() const {return nullptr;}
//     Vector<const BaseAST*> getAllAst(bool includeSelf = true) {(void)includeSelf; return {};}

//     Node evaluate() const override {throw MerkError("Cannot evaluate ScopeCallBackNode with no scope passed");}
//     UniquePtr<BaseAST> clone() const override {return nullptr;}
//     FreeVars collectFreeVariables() const override {return {};}
//     String toString() const override {return getAstTypeAsString();}
// };

using NativeClassFactory = std::function<SharedPtr<NativeClass>(SharedPtr<Scope>)>;
std::unordered_map<String, SharedPtr<ClassBase>> getAllNativeClasses(SharedPtr<Scope>);



#endif //CLASS_BUILTIN_H