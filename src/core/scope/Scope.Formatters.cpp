#include "core/Scope.hpp"

String ScopeCounts::toString() {
    String out;
    out += "FunctionCalls: " + std::to_string(functionCalls) + "\n";
    out += "MethodCalls: " + std::to_string(methodCalls) + "\n";
    out += "InstanceCalls: " + std::to_string(instanceCalls) + "\n";
    out += "ClassCalls: " + std::to_string(classCalls) + "\n";
    out += "Blocks: " + std::to_string(blocks) + "\n";
    return out;
}


// Helper to format a pointer or return a "None" string
String formatPointer(const Scope* ptr) {
    std::ostringstream oss;
    if (ptr) {
        oss << ptr;  // Stream the memory address
    } else {
        oss << "None (Root Scope)";
    }
    return oss.str();
}

String scopeKindToString(ScopeKind kind) {
    switch (kind)
    {
    case ScopeKind::Root: return "Root";
    case ScopeKind::Block: return "Block";
    case ScopeKind::FunctionDef: return "FunctionDef";
    case ScopeKind::FunctionCall: return "FunctionCall";
    case ScopeKind::ClassDef: return "ClassDef";
    case ScopeKind::ClassScope: return "ClassScope";
    case ScopeKind::Instance: return "Instance";
    case ScopeKind::Captured: return "Captured";
    case ScopeKind::Detached: return "Detached";
    case ScopeKind::Isolated: return "Isolated";
    case ScopeKind::MethodDef: return "MethodDef";
    case ScopeKind::MethodCall: return "MethodCall";

    default:
        return "UNKNOWN";
    }
};


String Scope::formattedScope() {
    std::ostringstream oss;
    oss << this;
    return oss.str();
}



String ScopeMeta::metaString() const {
    std::ostringstream oss;
    oss << "isRoot: " << (isRoot ? "true" : "false") << " | ";
    oss << "scopeLevel: " << scopeLevel << " | ";
    oss << "isDetached: " << (isDetached ? "true" : "false") << " | ";
    oss << "isCallable: " << (isCallableScope ? "true" : "false") << " | ";
    oss << "isCloned: " << (isClonedScope ? "true" : "false") << " | ";
    oss << "kind: " << scopeKindToString(kind);
    return oss.str();
}