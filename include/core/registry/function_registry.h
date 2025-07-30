// core/registry/registry.h
#ifndef FUNCTION_REGISTRY_H
#define FUNCTION_REGISTRY_H

#include <unordered_map>
#include <memory>
#include <optional>
#include <vector>
#include "core/types.h"
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

    std::optional<SharedPtr<CallableSignature>> getFunction(const String& name, const ArgResultType& args) const;

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

#endif // FUNCTION_REGISTRY_H

