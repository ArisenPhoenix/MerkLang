#ifndef SCOPE_H
#define SCOPE_H

#include "core/types.h"
#include "core/node.h"
#include "core/context.h"
#include "core/registry/function_registry.h"
#include "core/registry/class_registry.h"

class UserFunction;
class CallableSignature;

class ClassBase;
class ClassSignature;


class Scope : public std::enable_shared_from_this<Scope> {
private:
    WeakPtr<Scope> parentScope;          // Weak pointer to the parent scope - weak to avoid undue circular references
    int scopeLevel;                            // The level of the scope in the hierarchy

public:

    // Constructor
    explicit Scope(int scopeLevel, bool interpretMode, bool isRoot = false);
    explicit Scope(WeakPtr<Scope> parentScope = WeakPtr<Scope>(), bool interpretMode = false);
    explicit Scope(SharedPtr<Scope> parentScope, SharedPtr<FunctionRegistry> globalF, SharedPtr<ClassRegistry> globalC, bool interpreMode);

    ~Scope();

    SharedPtr<FunctionRegistry>  globalFunctions;
    SharedPtr<ClassRegistry>     globalClasses;
  
    // …you only ever stash local deltas here:
    std::unordered_map<String,SharedPtr<CallableSignature>>  localFunctions;
    std::unordered_map<String,SharedPtr<ClassSignature>>     localClasses;

    // Scope Management
    SharedPtr<Scope> createChildScope(); // Create a child scope
    SharedPtr<Scope> getParent() const;  // Get the parent scope
    int currentLine;
    int currentColumn;
    int getScopeLevel() const;           // Get the current scope level

    // Created For Debugging Scope when developing functions. Will keep until such time as deemed unnecessary
    bool isDetached = false;
    bool isCallableScope = false;
    bool isClonedScope = false;
    String owner = "";
    SharedPtr<Scope> makeCallScope();
    void includeMetaData(SharedPtr<Scope> newScope, bool isDetached = false) const;

    // Context Management
    const Context& getContext() const { return context; }
    Context& getContext() { return context; }
    const Context& getAllVariables(Context& callingContext) const;

    // Variable management
    void updateVariable(const String& name, const Node& value);          // Update an existing variable
    void declareVariable(const String& name, UniquePtr<VarNode> value);
    bool hasVariable(const String& name) const;
    VarNode& getVariable(const String& name);
    void printContext(int depth = 0) const;

    // lookup a function: first in your local overlay
    std::optional<SharedPtr<CallableSignature>> lookupFunction(const String& name, const Vector<Node>& args) const;
    std::optional<SharedPtr<CallableSignature>> lookupFunction(const String& name) const;
    // std::optional<std::reference_wrapper<SharedPtr<ClassSignature>>> lookupClass(const String& name, const Vector<Node>& args) const;
    std::optional<SharedPtr<ClassSignature>> lookupClass(const String& name) const;

// registering in *this* scope now only writes into your overlay:
    void registerFunctionHere(const String& name,SharedPtr<CallableSignature> f);
    
    // Registry Management
    // const FunctionRegistry& getFunctionRegistry() const;
    // FunctionRegistry& getFunctionRegistry();

    const SharedPtr<FunctionRegistry> getFunctionRegistry() const;
    SharedPtr<FunctionRegistry> getFunctionRegistry();

    const FunctionRegistry& getAllFunctions(FunctionRegistry& callingRegister) const;
    bool hasFunction(const String& name) const;

    void registerFunction(const String& name, SharedPtr<UserFunction> function);
    void registerFunction(const String& name, SharedPtr<CallableSignature> function);

    // std::optional<std::reference_wrapper<SharedPtr<CallableSignature>>> getFunction(const String& name, const Vector<Node>& args);
    std::optional<SharedPtr<CallableSignature>>  getFunction(const String& name, const Vector<Node>& args);
    std::optional<SharedPtr<CallableSignature>> getFunction(const String& name);


    // Class Management
    // const ClassRegistry& getClassRegistry() const;
    SharedPtr<ClassRegistry> getClassRegistry();

    // ClassRegistry& getClassRegistry();
    bool hasClass(const String& name) const;
    // void registerClass();
    void registerClass(const String& name, SharedPtr<ClassBase> cls);
    void registerClass(const String& name, SharedPtr<ClassSignature> classSig);
    std::optional<SharedPtr<ClassSignature>> getClass(const String& name);

    // for dynamic resolution of chains
    // Node lookup(const String& name, IdentifierType type);
    
    // SharedPtr<std::unordered_map<String, CallableType>> globalCallables;

    // Scope Other
    Vector<SharedPtr<Scope>> getChildren();
    bool hasChildren();
    void debugPrint() const;
    void printChildScopes(int indentLevel = 0) const;


    void setParent(SharedPtr<Scope> scope);

  
    SharedPtr<Scope> detachScope(const std::unordered_set<String>& freeVarNames);

    void updateChildLevelsRecursively();
    void setScopeLevel(int newLevel);

    SharedPtr<Scope> clone(bool strict = false) const;

    bool parentIsValid();

    void registerCallableType(const String& name, CallableType type);
    std::optional<CallableType> getCallableType(const String& name) const;
    Node resolveCallable(const String& name, const Vector<Node>& args = {});
    void attachToInstanceScope(SharedPtr<Scope> instanceScope);
    
    bool removeChildScope(const SharedPtr<Scope>& target);
    void appendChildScope(const SharedPtr<Scope>& child, const String& callerLabel, bool recursive = true);
    void appendChildScope(SharedPtr<Scope> childScope, bool recursive = true);

    void clear();

private:
    void setVariable(const String& name, UniquePtr<VarNode> value, bool isDeclaration);
    Vector<SharedPtr<Scope>> childScopes;      // List containing child scopes of (this) scope
    Context context;                           // The context for variable management
    // FunctionRegistry functionRegistry;         // Registry for function management
    // ClassRegistry classRegistry;               // Add class registry

    bool interpretMode;
    bool isRoot;
};

#endif // SCOPE_H
