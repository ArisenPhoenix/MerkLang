#include "core/registry/class_registry.h"
#include "utilities/debugger.h"

void ClassRegistry::debugPrint() const {
    for (const auto& [name, classSig] : classMap) {
        MARK_UNUSED_MULTI(name);
        DEBUG_LOG(LogLevel::DEBUG, "Registered Class: ", name);
    }
}


ClassRegistry::~ClassRegistry() {
    DEBUG_FLOW(FlowLevel::LOW);

    clear();  // Clear the map itself
    DEBUG_FLOW_EXIT();
}

void ClassRegistry::registerClass(const String& name, SharedPtr<ClassBase> classDef) {
    if (classMap.find(name) != classMap.end()) { throw MerkError("Class '" + name + "' already exists."); }
    if (!classDef->getClassScope()) { throw MerkError("ClassRegistry::registerClass(): Classdef contains no scope"); }
    auto classSig = std::static_pointer_cast<ClassSignature>(classDef->toCallableSignature());
    classMap[name] = classSig;
}

void ClassRegistry::clear() {
    classMap.clear(); // Then clear the container
}

void ClassRegistry::registerClass(const String& name, SharedPtr<ClassSignature> classSignature) {
    if (classMap.find(name) != classMap.end()) { throw MerkError("Class '" + name + "' already exists."); }
    classMap[name] = classSignature;
}

std::optional<SharedPtr<ClassSignature>> ClassRegistry::getClass(const String& name) {
    auto it = classMap.find(name);
    if (it != classMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool ClassRegistry::hasClass(const String& name) const {
    return classMap.find(name) != classMap.end();
}



ClassRegistry ClassRegistry::clone() const {
    ClassRegistry clonedClasses;
    for (const auto& [name, classSig] : this->classMap) {
        clonedClasses.classMap[name] = classSig;
    }
    return clonedClasses;
}