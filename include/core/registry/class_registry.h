#ifndef CLASS_REGISTRY_H
#define CLASS_REGISTRY_H

#include "core/types.h"
#include "core/functions/callable.h"

#include "core/classes/class_base.h"


class ClassSignature;

class ClassRegistry {
private:
    std::unordered_map<String, SharedPtr<ClassSignature>> classMap;
public:
    ~ClassRegistry();
    
    void registerClass(const String& name, SharedPtr<ClassBase> classDef);
    void registerClass(const String& name, SharedPtr<ClassSignature> classSignature);
    
    bool hasClass(const String& name) const;

    std::optional<SharedPtr<ClassSignature>> getClass(const String& name);
    const std::unordered_map<String, SharedPtr<ClassSignature>>& getClasses() const {return classMap;};
    ClassRegistry clone() const;
    void debugPrint() const;
    void clear(); 

};

#endif // CLASS_REGISTRY_H
