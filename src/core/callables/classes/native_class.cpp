#include "core/types.h"
#include "core/callables/classes/native_method.h"
#include "core/callables/classes/native_class.h"
 
NativeClass::NativeClass(String name, String access, SharedPtr<Scope> classScope)
: ClassBase(std::move(name), std::move(access), classScope) {

    DEBUG_LOG(LogLevel::PERMISSIVE, "Name: ", name, "Accessor: ", access);
    DEBUG_LOG(LogLevel::PERMISSIVE, "Name: ", getName(), "Accessor: ", getAccessor());
    // if (access.empty()) {throw MerkError("No Accessor");}

    // if (name.empty()) {throw MerkError("No Name");}
}

void NativeClass::addMethod(const String& name, SharedPtr<NativeMethod> method) {
    getClassScope()->registerFunction(name, method);
    // methodMap[name] = method;
}

void NativeClass::setConstructor(std::function<void(Vector<Node>, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode>)> fn) {
    constructorFn = std::move(fn);
}

NativeClass::~NativeClass() {
    if (getCapturedScope()) {getCapturedScope()->clear();}
    if (getClassScope()) {getClassScope()->clear();}
}

Node NativeClass::execute(Vector<Node> args,
                SharedPtr<Scope> scope,
                [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const  {
    DEBUG_LOG(LogLevel::PERMISSIVE, "Current Class Being Registered: ", name);
    auto inst = ClassInstance(getName(), scope, classScope, parameters, accessor);
    auto instancePtr = makeShared<ClassInstance>(getName(), scope, classScope, parameters, accessor);
    auto ensuredInstanceNode = makeShared<ClassInstanceNode>(instancePtr);

    if (constructorFn) {constructorFn(args, scope, ensuredInstanceNode);}

    return ensuredInstanceNode->getInstanceNode();
}

String NativeClass::toString() const  {
    return "NativeClass(" + name + ")";
}

SharedPtr<CallableSignature> NativeClass::toCallableSignature() {
    // CallableSignature(makeShared<NativeClass>(*this), CallableType::CLASS);
    auto sig = makeShared<CallableSignature>(makeShared<NativeClass>(*this), CallableType::CLASS);
    // sig->setParameterTypes(parameters.getParameterTypes());
    sig->setParameters(getParameters().clone());
    sig->setSubType(CallableType::NATIVE);
    return sig;
}


void NativeClass::setCapturedScope(SharedPtr<Scope> newScope) {
    capturedScope = newScope;
}

SharedPtr<Scope> NativeClass::getCapturedScope() const {
    if (auto scope = capturedScope.lock()) {
        return scope;
    }
    return nullptr;
    // return capturedScope;
}


void NativeClass::setScope(SharedPtr<Scope>) const {

}



// // SharedPtr<CallableSignature> NativeClass::toCallableSignature() const {
// //     auto sig = makeShared<CallableSignature>(makeShared<NativeClass>(*this), CallableType::CLASS);
// //     sig->setParameterTypes(parameters.getParameterTypes());
// //     sig->setSubType(CallableType::NATIVE);
// //     return sig;
// // }