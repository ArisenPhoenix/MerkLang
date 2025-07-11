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
    int scopeLevel;                      // The level of the scope in the hierarchy
    static inline size_t liveScopeCount = 0;
    static inline size_t totalScopeCreated = 0;
public:
    static void printScopeReport() {
        std::cout << "----------------------------" << std::endl;
        std::cout << "Total Scopes Created: " << totalScopeCreated << std::endl;
        std::cout << "Live Scopes Remaining: " << liveScopeCount << std::endl;
        if (liveScopeCount != 0) {
            std::cout << "[Memory Leak Detected!] " << liveScopeCount << " Scope(s) were not destroyed!" << std::endl;
        } else {
            std::cout << "[Memory Clean] All scopes were properly freed." << std::endl;
        }
        std::cout << "----------------------------" << std::endl;
    }

    // Constructor
    explicit Scope(int scopeLevel, bool interpretMode, bool isRoot = false);
    explicit Scope(WeakPtr<Scope> parentScope = WeakPtr<Scope>(), bool interpretMode = false);
    explicit Scope(SharedPtr<Scope> parentScope, SharedPtr<FunctionRegistry> globalF, SharedPtr<ClassRegistry> globalC, bool interpreMode);

    // // DESTRUCTOR
    ~Scope();
    void clear();
    // // Attributes
    String formattedScope();
    SharedPtr<FunctionRegistry>  globalFunctions;
    SharedPtr<ClassRegistry>     globalClasses;
    
    // Local Storage
    std::unordered_map<String, Vector<SharedPtr<CallableSignature>>>  localFunctions;
    std::unordered_map<String,SharedPtr<ClassSignature>>              localClasses;

    int currentLine;
    int currentColumn;

    bool isDetached = false;
    bool isCallableScope = false;
    bool isClonedScope = false;
    String owner = "";

    // Vector<String> protectedMembers;
    // void setProtectedMembers(Vector<String> protectedMems);
    // Vector<String> Scope::getProtectedMembers() const;
    // Scope Manipulation
    void attachToInstanceScope(SharedPtr<Scope> instanceScope);
    bool removeChildScope(const SharedPtr<Scope>& target);
    void appendChildScope(const SharedPtr<Scope>& child, const String& callerLabel, bool recursive = true);
    void appendChildScope(SharedPtr<Scope> childScope, bool recursive = true);
    SharedPtr<Scope> createChildScope(); // Create a child scope
    void includeMetaData(SharedPtr<Scope> newScope, bool isDetached = false) const;
    SharedPtr<Scope> makeInstanceScope(SharedPtr<Scope> classScope);

    // Scope Management
    SharedPtr<Scope> getParent() const;  // Get the parent scope
    int getScopeLevel() const;           // Get the current scope level
    SharedPtr<Scope> makeCallScope();
    SharedPtr<Scope> detachScope(const std::unordered_set<String>& freeVarNames);

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


    void handleFunctionRegistration(String funcMethName, SharedPtr<CallableSignature> funcMeth);
    SharedPtr<CallableSignature> handleLookupFunction(String& name, const Vector<Node>& args) const;
    
    //// Registry Management
    //// Function Management
    std::optional<SharedPtr<CallableSignature>> lookupFunction(const String& name, const Vector<Node>& args) const;
    std::optional<Vector<SharedPtr<CallableSignature>>> lookupFunction(const String& name) const;

    const SharedPtr<FunctionRegistry> getFunctionRegistry() const;
    SharedPtr<FunctionRegistry> getFunctionRegistry();

    const FunctionRegistry& getAllFunctions(FunctionRegistry& callingRegister) const;
    bool hasFunction(const String& name) const;

    void registerFunction(const String& name, SharedPtr<UserFunction> function);
    void registerFunction(const String& name, SharedPtr<CallableSignature> function);
    void registerFunction(const String& name, SharedPtr<Callable> anyCallable);
    SharedPtr<CallableSignature> getFunction(const String& name, const Vector<Node>& args);
    Vector<SharedPtr<CallableSignature>> getFunction(const String& name);


    //// Class Management
    std::optional<SharedPtr<ClassSignature>> lookupClass(const String& name) const;

    SharedPtr<ClassRegistry> getClassRegistry();

    bool hasClass(const String& name) const;
    void registerClass(const String& name, SharedPtr<ClassBase> cls);
    void registerClass(const String& name, SharedPtr<ClassSignature> classSig);
    std::optional<SharedPtr<ClassSignature>> getClass(const String& name);

    // Scope Other
    Vector<SharedPtr<Scope>> getChildren();
    bool hasChildren();
    void debugPrint() const;
    void setParent(SharedPtr<Scope> scope);

    bool has(const SharedPtr<Scope>& scope);

    void updateChildLevelsRecursively();
    void setScopeLevel(int newLevel);

    SharedPtr<Scope> clone(bool strict = false) const;

    bool parentIsValid();

    // // DEBUGGING
    void printChildScopes(int indentLevel = 0) const;
    void linkMethod(SharedPtr<Method> method, String methodName, SharedPtr<Scope> instanceScope);
    void linkMethods(SharedPtr<CallableSignature>  methodSig, String methodName, SharedPtr<Scope> instanceScope);
    void linkMethods(Vector<SharedPtr<MethodSignature>>& methodVec, String& methodName, SharedPtr<Scope> instanceScope);

    void linkInstanceMethods(SharedPtr<Scope> classTemplateScope, SharedPtr<Scope> instanceScope);
    
    SharedPtr<Scope> isolateScope(const std::unordered_set<String>& freeVarNames);
    


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
