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
public:

    // Constructor
    explicit Scope(int scopeLevel, bool interpretMode);
    explicit Scope(std::weak_ptr<Scope> parentScope = std::weak_ptr<Scope>(), bool interpretMode = false);
    ~Scope();

    // Scope Management
    SharedPtr<Scope> createChildScope(); // Create a child scope
    SharedPtr<Scope> getParent() const;  // Get the parent scope
    int currentLine;
    int currentColumn;
    void exitScope();                    // Exit the current scope -> moving back to parent, not really used
    int getScopeLevel() const;           // Get the current scope level

    // Created For Debugging Scope when developing functions. Will keep until such time as deemed unnecessary
    bool isDetached = false;
    bool isCallableScope = false;
    bool isClonedScope = false;

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


    
    // Registry Management
    const FunctionRegistry& getFunctionRegistry() const;
    FunctionRegistry& getFunctionRegistry();
    const FunctionRegistry& getAllFunctions(FunctionRegistry& callingRegister) const;
    bool hasFunction(const String& name) const;

    void registerFunction(const String& name, SharedPtr<UserFunction> function);
    void registerFunction(const String& name, SharedPtr<CallableSignature> function);

    std::optional<std::reference_wrapper<CallableSignature>> getFunction(const String& name, const Vector<Node>& args);
    std::optional<std::reference_wrapper<CallableSignature>> getFunction(const String& name);


    // Class Management
    const ClassRegistry& getClassRegistry() const;
    ClassRegistry& getClassRegistry();
    bool hasClass(const String& name) const;
    // void registerClass();
    void registerClass(const String& name, SharedPtr<ClassBase> cls);
    void registerClass(const String& name, SharedPtr<ClassSignature> classSig);
    std::optional<std::reference_wrapper<SharedPtr<CallableSignature>>> getClass(const String& name);

    // for dynamic resolution of chains
    Node lookup(const String& name, IdentifierType type);
    


    // Scope Other
    Vector<SharedPtr<Scope>> getChildren();
    bool hasChildren();
    void debugPrint() const;
    void printChildScopes(int indentLevel = 0) const;


    void setParent(SharedPtr<Scope> scope);

    void appendChildScope(SharedPtr<Scope> childScope);
    SharedPtr<Scope> detachScope(const std::unordered_set<String>& freeVarNames);
    SharedPtr<Scope> mergeScope(SharedPtr<Scope> newScope);

    SharedPtr<Scope> clone() const;

    bool parentIsValid();

    void registerCallableType(const String& name, CallableType type);
    SharedPtr<std::unordered_map<String, CallableType>> globalCallables;
    std::optional<CallableType> getCallableType(const String& name) const;
    Node resolveCallable(const String& name, const Vector<Node>& args = {});
    void attachToInstanceScope(SharedPtr<Scope> instanceScope);
    
    bool removeChildScope(const SharedPtr<Scope>& target);


private:
    void setVariable(const String& name, UniquePtr<VarNode> value, bool isDeclaration);
    std::weak_ptr<Scope> parentScope;          // Weak pointer to the parent scope - weak to avoid undue circular references
    Vector<SharedPtr<Scope>> childScopes;      // List containing child scopes of (this) scope
    int scopeLevel;                            // The level of the scope in the hierarchy
    Context context;                           // The context for variable management
    FunctionRegistry functionRegistry;         // Registry for function management
    ClassRegistry classRegistry;               // Add class registry

    bool interpretMode;
};

#endif // SCOPE_H
