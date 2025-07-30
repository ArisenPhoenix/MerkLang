#ifndef CLASS_REGISTRY_H
#define CLASS_REGISTRY_H

#include "core/types.h"
#include "core/callables/callable.h"

#include "core/callables/classes/class_base.h"


class ClassSignature;

class ClassRegistry {
private:
    std::unordered_map<String, SharedPtr<ClassSignature>> classMap;
public:
    ~ClassRegistry();
    
    void registerClass(const String& name, SharedPtr<ClassBase> classDef);
    void registerClass(const String& name, SharedPtr<ClassSignature> classSignature);
    
    bool hasClass(const String& name) const;

    std::optional<SharedPtr<ClassSignature>> getClass(const String& name) const;
    const std::unordered_map<String, SharedPtr<ClassSignature>>& getClasses() const {return classMap;};
    ClassRegistry clone() const;
    void debugPrint() const;
    void clear(); 

    void merge(const ClassRegistry& other);

    size_t size();
    size_t size() const;

    bool empty() const;


    auto begin() { return classMap.begin(); }
    auto end() { return classMap.end(); }
    auto begin() const { return classMap.begin(); }
    auto end() const { return classMap.end(); }

    // Add cbegin() and cend() for const iteration, c++ can be a pain
    auto cbegin() const { return classMap.cbegin(); }
    auto cend() const { return classMap.cend(); }


};

#endif // CLASS_REGISTRY_H
