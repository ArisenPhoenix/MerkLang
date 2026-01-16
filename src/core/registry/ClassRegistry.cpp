#include "core/registry/ClassRegistry.hpp"
#include "utilities/debugger.h"

void ClassRegistry::debugPrint() const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    if (classMap.empty()) {
        debugLog(true, "No Classes registered.");
    } else {
        debugLog(true, "\nRegistered Classes:");
        for (const auto &pair : classMap) {
            debugLog(true, "  ", pair.first,  "(", pair.second, ")");
        }
    }
    
    DEBUG_FLOW_EXIT();
}


ClassRegistry::~ClassRegistry() {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

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
    for (auto& [name, obj] : classMap) {
        obj.reset();
    }
    classMap.clear();
}

void ClassRegistry::registerClass(const String& name, SharedPtr<ClassSignature> classSignature) {
    if (classMap.find(name) != classMap.end()) { throw MerkError("Class '" + name + "' already exists."); }
    classMap[name] = classSignature;
}

std::optional<SharedPtr<ClassSignature>> ClassRegistry::getClass(const String& name) const {
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


void ClassRegistry::merge(const ClassRegistry& other) {
    for (const auto& [name, sig] : other.getClasses()) {
        if (!hasClass(name)) {
            classMap[name] = sig;
        }
    }
}


size_t ClassRegistry::size() { return classMap.size(); }
size_t ClassRegistry::size() const { return classMap.size(); }