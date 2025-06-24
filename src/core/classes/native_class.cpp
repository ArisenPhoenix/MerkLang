#include "core/types.h"
#include "core/classes/native_method.h"
#include "core/classes/native_class.h"

// NativeClass::NativeClass(String name, String accessor, SharedPtr<Scope> classScope)
//         : ClassBase(std::move(name), std::move(accessor), classScope) {}

// void NativeClass::addMethod(const String& name, SharedPtr<NativeMethod> method) {
//     getClassScope()->registerFunction(name, method);
//     // methodMap[name] = method;
// }

// void NativeClass::setConstructor(std::function<void(Vector<Node>, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode>)> fn) {
//     constructorFn = std::move(fn);
// }

// Node NativeClass::execute(Vector<Node> args,
//                 SharedPtr<Scope> scope,
//                 [[maybe_unused]] SharedPtr<ClassInstanceNode> instance = nullptr) const  {
    
//     auto instancePtr = makeShared<ClassInstance>(name, scope, classScope, parameters, accessor);
//     auto instanceNodeShared = makeShared<ClassInstanceNode>(instancePtr);
//     if (constructorFn) {
//         constructorFn(args, scope, instanceNodeShared);
//     }
//     // scope->registerClass()
//     // scope->registerClass(name, toCallableSignature());

//     auto instanceNode = ClassInstanceNode(instancePtr);
//     return instanceNode;
// }

// String NativeClass::toString() const  {
//     return "NativeClass(" + name + ")";
// }

// SharedPtr<CallableSignature> NativeClass::toCallableSignature() {
//     auto sig = makeShared<CallableSignature>(makeShared<NativeClass>(*this), CallableType::CLASS);
//     sig->setParameterTypes(parameters.getParameterTypes());
//     sig->setSubType(CallableType::NATIVE);
//     return sig;
// }

// // SharedPtr<CallableSignature> NativeClass::toCallableSignature() const {
// //     auto sig = makeShared<CallableSignature>(makeShared<NativeClass>(*this), CallableType::CLASS);
// //     sig->setParameterTypes(parameters.getParameterTypes());
// //     sig->setSubType(CallableType::NATIVE);
// //     return sig;
// // }