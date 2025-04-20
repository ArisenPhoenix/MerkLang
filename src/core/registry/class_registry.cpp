#include "core/registry/class_registry.h"
#include "utilities/debugger.h"

void ClassRegistry::debugPrint() const {
    for (const auto& [name, classSig] : classMap) {
        MARK_UNUSED_MULTI(name);
        DEBUG_LOG(LogLevel::DEBUG, "Registered Class: ", name);
    }
}

void ClassRegistry::registerClass(const String& name, SharedPtr<ClassBase> classDef) {
    if (classMap.find(name) != classMap.end()) {
        throw MerkError("Class '" + name + "' already exists.");
    }
    // Create a ClassSignature from the given ClassBase and store it.
    // classMap[name] = std::make_shared<CallableSignature>(classDef->toCallableSignature());
    if (!classDef->getScope()) {
        throw MerkError("ClassRegistry::registerClass(): Classdef contains no scope");
    }
    auto classSig = classDef->toCallableSignature();
    classMap[name] = classSig;
}

void ClassRegistry::registerClass(const String& name, SharedPtr<CallableSignature> classSignature) {
    if (classMap.find(name) != classMap.end()) {
        throw MerkError("Class '" + name + "' already exists.");
    }
    classMap[name] = classSignature;
}

std::optional<std::reference_wrapper<SharedPtr<CallableSignature>>> ClassRegistry::getClass(const String& name) {
    auto it = classMap.find(name);
    if (it != classMap.end()) {
        // Return the shared_ptr stored in the map
        return std::optional<std::reference_wrapper<SharedPtr<CallableSignature>>>(it->second);
    }
    return std::nullopt;
}

bool ClassRegistry::hasClass(const String& name) const {
    return classMap.find(name) != classMap.end();
}


