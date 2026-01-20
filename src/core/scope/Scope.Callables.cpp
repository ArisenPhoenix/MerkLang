#include "core/Scope.hpp"
#include "core/callables/Callable.hpp"
#include "core/callables/classes/Method.hpp"




const SharedPtr<FunctionRegistry> Scope::getFunctionRegistry() const {return globalFunctions;}
SharedPtr<FunctionRegistry> Scope::getFunctionRegistry() {return globalFunctions;}

void Scope::registerFunction(const String& name, SharedPtr<UserFunction> function) {
    DEBUG_FLOW(FlowLevel::MED);
    if (function->getCallableType() == CallableType::CLASS) {
        throw MerkError("Cannot Register A Class Into Function Registry");
    }
    auto signature = function->toCallableSignature();
    localFunctions.registerFunction(name, signature);
    DEBUG_FLOW_EXIT();
}

void Scope::registerFunction(const String& name, SharedPtr<Callable> anyCallable) {
    DEBUG_FLOW(FlowLevel::MED);
    if (anyCallable->getCallableType() == CallableType::CLASS) {
        throw MerkError("Cannot Register A Class Into Function Registry");
    }
    auto signature = anyCallable->toCallableSignature();
    localFunctions.registerFunction(name, signature);
    DEBUG_FLOW_EXIT();
}



bool Scope::hasFunction(const String& name) const {
    
    if (auto func = lookupFunction(name)){
        return true;
    }
    if (auto parent = getParent()){
        return getParent()->hasFunction(name);
    } else {
    
    }

    return globalFunctions->hasFunction(name);
}


std::optional<Vector<SharedPtr<CallableSignature>>> Scope::lookupFunction(const String& name) const {
    // Prefer local overload sets (shadowing). If not found, walk up parents, then fall back to globals.
    if (auto funcs = localFunctions.getFunction(name)) { return funcs; }
    if (auto parent = parentScope.lock()) { return parent->lookupFunction(name); }
    if (globalFunctions && globalFunctions->hasFunction(name)) {
        auto sig = globalFunctions->getFunction(name);
        if (sig.has_value()) return sig;
    }
    return std::nullopt;
  }


std::optional<SharedPtr<CallableSignature>> Scope::getFunction(const String& name, const ArgumentList& args) {

    DEBUG_FLOW(FlowLevel::MED);

    // Overload resolution is now type-aware and centralized here.
    auto resolved = resolveFunctionOverload(name, args);
    DEBUG_FLOW_EXIT();
    return resolved;
}

// For Function Reference
std::optional<Vector<SharedPtr<CallableSignature>>> Scope::getFunction(const String& name) {
    if (auto funcs = lookupFunction(name)) { return funcs.value(); }
    if (auto parent = parentScope.lock()) { return parent->getFunction(name); }
    if (globalFunctions) {
        auto globals = globalFunctions->getFunction(name);
        if (globals) { return globals.value(); }
    }

    return std::nullopt;
}





std::optional<SharedPtr<ClassSignature>> Scope::lookupClass(const String& name) const {
    if (auto local = localClasses.getClass(name)) {
        return local;
    }

    if (auto parent = parentScope.lock()) {
        return parent->lookupClass(name);
    }

    if (globalClasses && globalClasses->hasClass(name)) {
        return globalClasses->getClass(name);
    }

    return std::nullopt;
  }





std::optional<SharedPtr<ClassSignature>> Scope::getClass(const String& name) {
    if (auto local = localClasses.getClass(name)) {
        return local;
    }

    if (auto parent = parentScope.lock()) {
        return parent->getClass(name);
    }

    if (globalClasses) {
        return globalClasses->getClass(name);
    }

    return std::nullopt;
}


bool Scope::hasClass(const String& name) const {
    if (localClasses.hasClass(name)) {
        return true;
    }

    if (auto parent = parentScope.lock()) {
        return parent->hasClass(name);
    }

    return globalClasses && globalClasses->hasClass(name);
}

SharedPtr<ClassRegistry> Scope::getClassRegistry() { return globalClasses; }
