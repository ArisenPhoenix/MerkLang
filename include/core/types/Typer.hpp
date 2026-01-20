#pragma once
#include "core/types/Type.hpp"
class TypeSignatureRegistry;
class CallableSignature;

namespace Typer {
    TypeSignatureId ensureInvocableSig(
        CallableSignature& sig,
        Scope& scope
    );

    TypeSignatureId TypeMethod();
    TypeSignatureId TypeClass();
    TypeSignatureId TypeFunction();
    TypeSignatureId TypeNodeValue();
    TypeSignatureId TypeResolvedType();

    TypeSignatureId BuildInvocableSig(CallableSignature& sig, TypeSignatureRegistry& reg, Scope& scope );


}