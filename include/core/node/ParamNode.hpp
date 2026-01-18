#pragma once

#include <optional>
#include "core/types.h"
#include "core/node/Node.hpp"

class ArgumentList;
class Scope;
class TypeSignatureRegistry;
class TypeRegistry;

class ParamNode {
public:
    DataTypeFlags flags;                 // keep existing
    VariantType defaultValue = UninitializedType();

    bool isVarArgs = false;
    bool hasDefaultValue = false;

    // NEW: typed annotation state (Option C)
    bool hasAnnotation = false;          // “was :Type written?”
    TypeSignatureId typeSig = 0;         // bound later, 0 = unbound

public:
    ParamNode() = default;
    ~ParamNode() = default;

    ParamNode(DataTypeFlags flags, VariantType defaultVal = UninitializedType());

    // legacy ctor still OK; treat type as legacy
    ParamNode(String name, NodeValueType type, bool isVarArgs = false);

    // Core accessors
    const String& getNameRef() const { return flags.name; }
    String getName() const { return flags.name; }
    bool isVarArgsParameter() const { return isVarArgs; }
    void setIsVarArgsParam(bool v) { isVarArgs = v; }

    bool hasDefault() const { return hasDefaultValue; }
    VariantType getDefaultValue() const;

    // Annotation API (new)
    void setAnnotation(const ResolvedType& rt);          // called by parser
    bool isTyped() const { return hasAnnotation; }
    TypeSignatureId getTypeSig() const { return typeSig; }

    // Bind phase: resolve annotation -> typeSig
    void bindType(TypeSignatureRegistry& reg, Scope& scope);

    // Debug
    String toString() const;
    String toShortString() const;
    size_t hash() const;
    ParamNode copy() const;

private:
    void updateDefaultFlags();
};

// ------------------------
// ParamList
// ------------------------
class ParamList {
private:
    Vector<ParamNode> parameters;

public:
    ParamList() = default;
    ~ParamList() = default;

    void addParameter(const ParamNode& p) { parameters.push_back(p); }

    size_t size() const { return parameters.size(); }
    bool empty() const { return parameters.empty(); }
    ParamNode& back() { return parameters.back(); }
    ParamNode back() const { return parameters.back(); }

    const ParamNode& operator[](size_t index) const;

    const ParamNode* getByName(const String& name) const;
    Vector<String> getNames() const;

    // NEW: bind all param annotations
    void bindTypes(TypeSignatureRegistry& reg, Scope& scope);

    // NEW: call-time binding (does not mutate ParamList)
    Vector<Node> bindArguments(const Vector<Node>& args) const;
    Vector<Node> bindArguments(ArgumentList& args) const;

    // Keep old API but re-route it (optional)
    void verifyArguments(Vector<Node> args);        // wrapper
    void verifyArguments(ArgumentList& args);       // wrapper

    // Helpers
    Vector<NodeValueType> getParameterTypes();
    String toString() const;
    String toShortString() const;

    ParamList clone();
    ParamList clone() const;

    auto begin() { return parameters.begin(); }
    auto end() { return parameters.end(); }
    auto begin() const { return parameters.begin(); }
    auto end() const { return parameters.end(); }
    auto cbegin() const { return parameters.cbegin(); }
    auto cend() const { return parameters.cend(); }

    void erase(size_t index);
    bool eraseByName(const String& name);

    void printAST(std::ostream& os, int indent = 0) const;
};

