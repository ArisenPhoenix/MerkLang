#pragma once

#include "core/Scope.hpp"

// Lightweight callable-frame specialization.
// Keeps Scope API unchanged so call sites can adopt it incrementally.
class StackScope final : public Scope {
public:
    StackScope(SharedPtr<Scope> parentScope,
               SharedPtr<FunctionRegistry> globalF,
               SharedPtr<ClassRegistry> globalC,
               SharedPtr<TypeRegistry> globalT,
               bool interpretMode)
        : Scope(std::move(parentScope),
                std::move(globalF),
                std::move(globalC),
                std::move(globalT),
                interpretMode) {}
};

