#pragma once

#ifndef _WIN32
#include <execinfo.h>
#endif

#include "core/TypesFWD.hpp"
#include "core/registry/Context.hpp"
#include "core/registry/FunctionRegistry.hpp"
#include "core/registry/ClassRegistry.hpp"
#include "core/registry/TypeSignatureRegistry.hpp"
#include "core/registry/TypeSignatureRegistryManager.hpp"

using ScopeCache = std::unordered_map<String, Scope>; 


enum class ScopeKind { Root, Block, FunctionDef, FunctionCall, ClassDef, ClassScope, Instance, Captured, Detached, Isolated, MethodDef, MethodCall };

String scopeKindToString(ScopeKind kind);

class ScopeMeta {
public:
    String owner = "";
    bool isDetached = false;
    bool isCallableScope = false;
    bool isClonedScope = false;
    int currentLine;
    int currentColumn;
    bool disregardDeclarations = false;
    bool isRoot = false;
    int scopeLevel;                      // The level of the scope in the hierarchy
    ScopeKind kind = ScopeKind::Block;
    String metaString() const;
}; 


struct ScopeCounts {
    int functionCalls = 0;
    int methodCalls = 0;
    int blocks = 0;
    int roots = 0;
    int classCalls = 0;
    int instanceCalls = 0;

    String toString();
};




class Scope : public ScopeMeta, public std::enable_shared_from_this<Scope> {
private:
    WeakPtr<Scope> parentScope;          // Weak pointer to the parent scope - weak to avoid undue circular references
    static inline size_t liveScopeCount = 0;
    static inline size_t totalScopeCreated = 0;
    ClassMembers classMembers;   //map for future uses
    static inline ScopeCounts counts{};

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

        std::cout << "SCOPE COUNTS: " << std::endl;
        std::cout << Scope::counts.toString() << std::endl;
        std::cout << "----------------------------" << std::endl;
    }

    // Constructor
    explicit Scope(int scopeLevel, bool interpretMode, bool isRoot = false);
    explicit Scope(WeakPtr<Scope> parentScope = WeakPtr<Scope>(), bool interpretMode = false);
    explicit Scope(SharedPtr<Scope> parentScope, SharedPtr<FunctionRegistry> globalF, SharedPtr<ClassRegistry> globalC, SharedPtr<TypeRegistry> globalT, bool interpreMode);

    // // DESTRUCTOR
    ~Scope();
    void clear(bool internalCall = true);
    // // Attributes
    String formattedScope();
    SharedPtr<FunctionRegistry>  globalFunctions;
    SharedPtr<ClassRegistry>     globalClasses;
    SharedPtr<TypeRegistry>      globalTypes;
    SharedPtr<TypeSignatureRegistryManager> globalTypeSigs;
    
    // // Local Storage
    FunctionRegistry localFunctions;
    ClassRegistry localClasses;
    TypeSignatureRegistry localTypes;
    

    ClassMembers getClassMembers() const;
    void setClassMembers(ClassMembers);
    bool hasMember(String&);
    void addMember(String&);
    void addMember(String&, String&);

    // Scope Manipulation
    void attachToInstanceScope(SharedPtr<Scope> instanceScope);
    bool removeChildScope(const SharedPtr<Scope>& target);
    void appendChildScope(const SharedPtr<Scope>& child, const String& callerLabel, bool recursive = true);
    void appendChildScope(SharedPtr<Scope> childScope, bool recursive = true);
    SharedPtr<Scope> createChildScope(); // Create a child scope
    void includeMetaData(SharedPtr<Scope> newScope, bool isDetached = false) const;
    SharedPtr<Scope> makeInstanceScope(SharedPtr<Scope> classScope);


    SharedPtr<Scope> getRoot();

    // Scope Management
    SharedPtr<Scope> getParent() const;  // Get the parent scope
    int getScopeLevel() const;           // Get the current scope level
    SharedPtr<Scope> makeCallScope();
    SharedPtr<Scope> detachScope(const std::unordered_set<String>& freeVarNames);

    // Context Management
    const Context& getContext() const { return context; }
    Context& getContextWith(const String& varName);
    Context& getContext() { return context; }
    const Context& getAllVariables(Context& callingContext) const;

    // Variable management
    void updateVariable(const String& name, const Node& value);          // Update an existing variable
    void declareVariable(const String& name, UniquePtr<VarNode> value);
    bool hasVariable(const String& name) const;
    VarNode& getVariable(const String& name);
    void printContext(int depth = 0) const;

    void linkTypes();


    // void handleFunctionRegistration(String funcMethName, SharedPtr<CallableSignature> funcMeth);
    SharedPtr<CallableSignature> handleLookupFunction(String& name, const ArgumentList& args) const;

    // Type-aware overload resolution lives at the Scope level.
    std::optional<SharedPtr<CallableSignature>> resolveFunctionOverload(
        const String& name,
        const ArgumentList& args,
        const TypeMatchOptions& opt = {}
    );
    
    //// Registry Management
    //// Function Management
    // Legacy (arg-based) lookup removed: use resolveFunctionOverload instead.
    std::optional<Vector<SharedPtr<CallableSignature>>> lookupFunction(const String& name) const;

    const SharedPtr<FunctionRegistry> getFunctionRegistry() const;
    SharedPtr<FunctionRegistry> getFunctionRegistry();

    const FunctionRegistry& getAllFunctions(FunctionRegistry& callingRegister) const;
    bool hasFunction(const String& name) const;

    void registerFunction(const String& name, SharedPtr<UserFunction> function);
    void registerFunction(const String& name, SharedPtr<CallableSignature> function);
    void registerFunction(const String& name, SharedPtr<Callable> anyCallable);
    std::optional<SharedPtr<CallableSignature>> getFunction(const String& name, const ArgumentList& args);
    std::optional<Vector<SharedPtr<CallableSignature>>> getFunction(const String& name);
    TypeSignatureId resolveTypeNameSig(const String& name);
    void bindTypeAlias(const String& alias, TypeSignatureId sig);

    //// Class Management
    std::optional<SharedPtr<ClassSignature>> lookupClass(const String& name) const;

    SharedPtr<ClassRegistry> getClassRegistry();

    bool hasClass(const String& name) const;
    void registerClass(const String& name, SharedPtr<ClassBase> cls);
    void registerClass(const String& name, SharedPtr<ClassSignature> classSig);
    void registerClass(String& name, SharedPtr<ClassSignature> classSig) const;
    std::optional<SharedPtr<ClassSignature>> getClass(const String& name);

    // Scope Other
    Vector<SharedPtr<Scope>>& getChildren();
    bool hasChildren();
    void debugPrint() const;
    void setParent(SharedPtr<Scope> scope);

    bool has(const SharedPtr<Scope>& scope);
    bool isAncestorOf(const Scope* maybeDesc) const;
    bool hasImmediateChild(const SharedPtr<Scope>& candidate);

    void updateChildLevelsRecursively();
    void setScopeLevel(int newLevel);

    SharedPtr<Scope> clone(bool strict = false) const;

    bool parentIsValid();

    // // DEBUGGING
    void printChildScopes(int indentLevel = 0) const;
    
    SharedPtr<Scope> isolateScope(const std::unordered_set<String>& freeVarNames);


    // scope builders

    SharedPtr<Scope> buildFunctionCallScope(SharedPtr<Function> func, String name);
    SharedPtr<Scope> buildMethodCallScope(SharedPtr<Method> func, String name);

    SharedPtr<Scope> buildInstanceScope(SharedPtr<ClassBase> classTemplate, String className);
    SharedPtr<Scope> buildClassScope(FreeVars freeVarNames, String className);

    SharedPtr<Scope> buildFunctionDefScope(const FreeVars& freeVars, const String& funcName);

    SharedPtr<Scope> buildClassDefScope(const FreeVars& freeVars, const String& className);
    
    TypeSignatureId bindResolvedType(const ResolvedType& rt, const String& aliasName = "");


    SharedPtr<TypeRegistry> getTypeRegistry();
    void registerType(TypeId);
    void registerPrimitiveType(NodeValueType);
    void registerNamedType(String&);

    

    
    ScopeCounts getCounts();
    void validateNoCycles(SharedPtr<Scope> childScope);

    // Type signature lookup is scope-aware.
    // - Built-in/"global" nominal types come from globalTypes.
    // - User-defined/aliased type signatures live in localTypes and parent scopes.
    std::optional<TypeSignatureId> lookupTypeSigName(const String& name);

    TypeSignatureId inferSigFromNode(const Node& n, TypeSignatureRegistry& reg);
private:
    void setVariable(const String& name, UniquePtr<VarNode> value, bool isDeclaration);
    Vector<SharedPtr<Scope>> childScopes;      // List containing child scopes of (this) scope
    Context context;                           // The context for variable management

    bool interpretMode;
    void initRootTypes();
    // bool isRoot;
};

