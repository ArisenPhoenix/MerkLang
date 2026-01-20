#include "core/types/Typer.hpp"
#include "ast/AstCallable.hpp"                 // CallableSignature
#include "core/Scope.hpp"                      // Scope
#include "core/registry/TypeSignatureRegistry.hpp"
#include "core/node/ParamNode.hpp"


namespace Typer {

    TypeSignatureId BuildInvocableSig(CallableSignature& sig, TypeSignatureRegistry& reg, Scope& scope ) {
        ParamList params = sig.getParameters();   // copy
        params.bindTypes(reg, scope);

        InvocableSigType m;
        m.name = sig.getCallable()->getName();
        m.variadic = (!params.empty() && params.back().isVarArgsParameter());
        m.ret = reg.any();
        m.retEnforced = false;

        m.params.reserve(params.size());
        m.enforced.reserve(params.size());

        for (auto& p : params) {
            if (p.isTyped() && p.getTypeSig() != kInvalidTypeSignatureId) {
                m.params.push_back(p.getTypeSig());
                m.enforced.push_back(1);
            } else {
                m.params.push_back(reg.any());
                m.enforced.push_back(0);
            }
        }

        return reg.invocableType(std::move(m));
    }
    TypeSignatureId ensureInvocableSig(CallableSignature& sig, Scope& scope) {
        // If already built, keep it.
        if (sig.getTypeSignature() != kInvalidTypeSignatureId) {
            return sig.getTypeSignature();
        }

        auto& reg = scope.localTypes; // your scope-attached TSR

        // IMPORTANT: bind types for params *in this scope* so annotations resolve to ids
        ParamList params = sig.getParameters();
        params.bindTypes(reg, scope);

        InvocableSigType m;
        m.name = sig.getCallable()->getName();
        m.variadic = (!params.empty() && params.back().isVarArgsParameter());

        // Return type: you can keep Any for now
        m.ret = reg.any();
        m.retEnforced = false;

        m.params.reserve(params.size());
        m.enforced.reserve(params.size());

        for (size_t i = 0; i < params.size(); ++i) {
            const auto& p = params[i];

            // If annotated and resolved -> enforced
            if (p.isTyped() && p.getTypeSig() != 0) {
                m.params.push_back(p.getTypeSig());
                m.enforced.push_back(1);
            } else {
                m.params.push_back(reg.any());
                m.enforced.push_back(0);
            }
        }

        TypeSignatureId id = reg.invocableType(std::move(m));
        sig.setTypeSignature(id);
        return id;
    }

}
// #include "core/Scope.hpp"
// #include "core/callables/classes/Method.hpp"

// namespace Typer {
// TypeSignatureId TypeMethod(SharedPtr<Method> method, TypeRegistry& typeRegistry) {
//     InvocableType mt;
//     mt.methodName = method->getName();
//     SharedPtr<Scope> scope = method->getCapturedScope();
//     SharedPtr<Scope>& workingScope = scope;
//     // Parameters
//     for (auto& param : method->parameters) {
//         // if (param)
//         if (param.flags.type != NodeValueType::Any) {
//             auto tid = typeRegistry.bindResolvedType(param.typeExpr, workingScope);
//             mt.params.push_back(tid);
//             mt.enforced.push_back(1);
//         } else {
//             mt.params.push_back(typeRegistry.any());
//             mt.enforced.push_back(0);
//         }
//     }

//     // Return type
//     if (method->getRequiresReturn()) {
//         mt.ret = typeRegistry.bindResolvedType(method->getReturnType, workingScope);
//         mt.retEnforced = true;
//     } else {
//         mt.ret = typeRegistry.any();
//         mt.retEnforced = false;
//     }

//     // Intern canonical signature
//     sig.typeSig = typeRegistry.invocableType(std::move(mt));
// }

//     TypeSignatureId TypeClass() {

//     }

//     TypeSignatureId TypeFunction() {

//     }

//     TypeSignatureId TypeNodeValue() {

//     }

//     TypeSignatureId TypeResolvedType() {

//     }
// }