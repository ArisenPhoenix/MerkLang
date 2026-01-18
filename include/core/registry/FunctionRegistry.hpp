#pragma once

#include <unordered_map>
#include <memory>
#include <optional>
#include <vector>
// #include "core/types.h"
#include "core/TypesFWD.hpp"
#include "utilities/debugger.h"

class CallableSignature;

class FunctionRegistry {
private:
    std::unordered_map<String, Vector<SharedPtr<CallableSignature>>> functions;
public:

    ~FunctionRegistry();
    
    void registerFunction(const String& name, SharedPtr<CallableSignature> signature);
    
    bool hasFunction(const String& name) const;

    std::optional<Vector<SharedPtr<CallableSignature>>> getFunction(const String& name) const;

    // NOTE: overload resolution now happens at Scope-level (type-aware),
    // so the registry only returns the overload set.

    void debugPrint() const;

    const std::unordered_map<String, Vector<SharedPtr<CallableSignature>>>& getFunctions() const {return functions;};

    FunctionRegistry clone() const;
    void clear();

    void merge(const FunctionRegistry& other);

    size_t size();
    size_t size() const;

    bool empty() const;

    auto begin() { return functions.begin(); }
    auto end() { return functions.end(); }
    auto begin() const { return functions.begin(); }
    auto end() const { return functions.end(); }

    // Add cbegin() and cend() for const iteration, c++ can be a pain
    auto cbegin() const { return functions.cbegin(); }
    auto cend() const { return functions.cend(); }


};

