// core/registry/registry.h
#ifndef FUNCTION_REGISTRY_H
#define FUNCTION_REGISTRY_H

#include <unordered_map>
#include <memory>
#include <optional>
#include <vector>
#include "core/types.h"
#include "utilities/debugger.h"
// #include "core/functions/function_node.h"

// class FunctionSignature;
class CallableSignature;

class FunctionRegistry {
private:
    std::unordered_map<String, Vector<SharedPtr<CallableSignature>>> functions;
public:

    ~FunctionRegistry();
    
    void registerFunction(const String& name, SharedPtr<CallableSignature> signature);
    
    bool hasFunction(const String& name) const;

    std::optional<std::reference_wrapper<SharedPtr<CallableSignature>>> getFunction(const String& name);

    std::optional<std::reference_wrapper<SharedPtr<CallableSignature>>> getFunction(const String& name, const Vector<Node>& args);

    void debugPrint() const;

    const std::unordered_map<String, Vector<SharedPtr<CallableSignature>>>& getFunctions() const {return functions;};

    FunctionRegistry clone() const;
    void clear();


};

#endif // FUNCTION_REGISTRY_H

