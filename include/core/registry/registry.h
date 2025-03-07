// core/registry/registry.h
#ifndef FUNCTION_REGISTRY_H
#define FUNCTION_REGISTRY_H

#include <unordered_map>
#include <memory>
#include <optional>
#include <vector>
#include "core/types.h"
#include "utilities/debugger.h"
#include "core/functions/function_node.h"

class FunctionRegistry {
private:
    std::unordered_map<String, Vector<SharedPtr<FunctionSignature>>> functions;
public:

    ~FunctionRegistry();
    
    void registerFunction(const String& name, SharedPtr<FunctionSignature> signature);
    
    bool hasFunction(const String& name) const;

    std::optional<std::reference_wrapper<FunctionSignature>> getFunction(const String& name);

    std::optional<std::reference_wrapper<FunctionSignature>> getFunction(const String& name, const Vector<Node>& args);

    void debugPrint() const;

    const std::unordered_map<String, Vector<SharedPtr<FunctionSignature>>>& getFunctions() const {return functions;};

    FunctionRegistry clone() const;


};

#endif // FUNCTION_REGISTRY_H

