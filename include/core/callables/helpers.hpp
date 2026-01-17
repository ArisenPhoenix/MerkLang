#pragma once
#include "core/TypesFWD.hpp"
#include "core/node/ArgumentNode.hpp"   // ArgumentList
#include "core/node/BoundArgs.hpp"
#include "core/registry/TypeRegistry.hpp" // TypeSignatureRegistry

class CallableSignature;

CallableSignature* resolveOverload(
    const Vector<SharedPtr<CallableSignature>>& overloads,
    const ArgumentList& callArgs,
    TypeRegistry& typeReg,
    CallableType callableType,
    BoundArgs& outBound
);
