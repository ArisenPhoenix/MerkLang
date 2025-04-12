#ifndef CLASS_REGISTRY_H
#define CLASS_REGISTRY_H

#include "core/types.h"
#include "core/functions/callable.h"

#include "core/classes/class_base.h"


class ClassSignature;

class ClassRegistry {
private:
    std::unordered_map<String, SharedPtr<CallableSignature>> classMap;
public:
    ~ClassRegistry() = default;
    
    void registerClass(const String& name, SharedPtr<ClassBase> classDef);
    void registerClass(const String& name, SharedPtr<CallableSignature> classSignature);
    
    bool hasClass(const String& name) const;
    
    std::optional<std::reference_wrapper<SharedPtr<CallableSignature>>> getClass(const String& name);
    
    void debugPrint() const;
};

#endif // CLASS_REGISTRY_H
