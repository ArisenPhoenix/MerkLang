#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "core/TypesFWD.hpp" // String, Vector, NodeValueType, ArgumentList, TypeId, TypeSignatureId



enum class TypeSigKind : uint8_t {
    Any,
    Nominal,
    Container,
    Union,
    Interface,
    Invocable,
    // Primitive,
    // Class,
    // Method,

};

struct TypeMatchOptions {
    bool allowAny = true;
    bool allowNumericWidening = true;
    bool allowDuck = false; // later
};

struct TypeMatchResult {
    bool ok = false;
    int score = -1;
    int cost = 0;

    static TypeMatchResult No() { return {}; }
    static TypeMatchResult Yes(int score_, int cost_ = 0) { return {true, score_, cost_}; }
};