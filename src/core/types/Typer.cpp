#include "core/types/Typer.hpp"
#include "ast/AstCallable.hpp"                 
#include "core/Scope.hpp"                    
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
        if (sig.getTypeSignature() != kInvalidTypeSignatureId) {
            return sig.getTypeSignature();
        }

        auto& reg = scope.localTypes; 

        ParamList params = sig.getParameters();
        params.bindTypes(reg, scope);

        InvocableSigType m;
        m.name = sig.getCallable()->getName();
        m.variadic = (!params.empty() && params.back().isVarArgsParameter());

        // Return type: keeping Any for now
        m.ret = reg.any();
        m.retEnforced = false;

        m.params.reserve(params.size());
        m.enforced.reserve(params.size());

        for (size_t i = 0; i < params.size(); ++i) {
            const auto& p = params[i];
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
