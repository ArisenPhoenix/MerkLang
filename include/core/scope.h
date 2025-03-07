#ifndef SCOPE_H
#define SCOPE_H

#include "core/types.h"
#include "core/node.h"
#include "core/context.h"
#include "core/registry/registry.h"


class UserFunctionNode;
class FunctionSignature;


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
    // void enterScope();
    int getScopeLevel() const;           // Get the current scope level

    // Created For Debugging Scope when developing functions. Will keep until such time as deemed unnecessary
    bool isDetached = false;
    bool isFunctionCallScope = false;
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
    const FunctionRegistry& getFunctionRegistry() const { return functionRegistry; }
    FunctionRegistry& getFunctionRegistry() { return functionRegistry; }
    const FunctionRegistry& getAllFunctions(FunctionRegistry& callingRegister) const;

    void registerFunction(const String& name, SharedPtr<UserFunction> function);
    void registerFunction(const String& name, SharedPtr<FunctionSignature> function);

    std::optional<std::reference_wrapper<FunctionSignature>> getFunction(const String& name, const Vector<Node>& args);
    std::optional<std::reference_wrapper<FunctionSignature>> getFunction(const String& name);
 
    Vector<SharedPtr<Scope>> getChildren();
    bool hasChildren();
    void debugPrint() const;
    void printChildScopes(int indentLevel = 0) const;


    void setParent(SharedPtr<Scope> scope){
        scope->appendChildScope(shared_from_this());
        parentScope = scope;
        scopeLevel = scope->getScopeLevel() + 1;
    }

    void appendChildScope(SharedPtr<Scope> childScope);
    SharedPtr<Scope> detachScope(const std::unordered_set<String>& freeVarNames);
    SharedPtr<Scope> mergeScope(SharedPtr<Scope> newScope);

    SharedPtr<Scope> clone() const;



private:
    void setVariable(const String& name, UniquePtr<VarNode> value, bool isDeclaration);
    std::weak_ptr<Scope> parentScope;          // Weak pointer to the parent scope - weak to avoid undue circular references
    Vector<SharedPtr<Scope>> childScopes;      // List containing child scopes of (this) scope
    int scopeLevel;                            // The level of the scope in the hierarchy
    Context context;                           // The context for variable management
    FunctionRegistry functionRegistry;         // Registry for function management
    bool interpretMode;
};

#endif // SCOPE_H
