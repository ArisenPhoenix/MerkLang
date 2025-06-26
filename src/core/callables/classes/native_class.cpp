// #include "core/types.h"
// #include "core/callables/classes/native_method.h"
// #include "core/callables/classes/native_class.h"

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
//                 [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const  {
//     DEBUG_LOG(LogLevel::PERMISSIVE, "Current Class Being Registered: ", name);
//     // String instanceName = getName();
//     // String& instanceName = name;
//     auto inst = ClassInstance(std::move(name), scope, classScope, parameters, accessor);
//     auto instancePtr = makeShared<ClassInstance>(getName(), scope, classScope, parameters, accessor);
//     auto ensuredInstanceNode = makeShared<ClassInstanceNode>(instancePtr);

//     // auto instanceNodeShared = 
//     // auto instanceNodeShared = makeShared<ClassInstanceNode>(instancePtr);
//     // auto instanceNodeShared = ClassInstance(String& name, SharedPtr<Scope> capturedScope, SharedPtr<Scope> instanceScope, ParamList params, String& accessor);

//     if (constructorFn) {
//         constructorFn(args, scope, ensuredInstanceNode);
//     }
//     // scope->registerClass()
//     // scope->registerClass(name, toCallableSignature());

//     return ensuredInstanceNode->getInstanceNode();
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


// void NativeClass::setCapturedScope(SharedPtr<Scope> newScope) {
//     capturedScope = newScope;
// }

// SharedPtr<Scope> NativeClass::getCapturedScope() const {
//     if (auto scope = capturedScope.lock()) {
//         return scope;
//     }
//     return nullptr;
//     // return capturedScope;
// }


// void NativeClass::setScope(SharedPtr<Scope>) const {

// }



// // // SharedPtr<CallableSignature> NativeClass::toCallableSignature() const {
// // //     auto sig = makeShared<CallableSignature>(makeShared<NativeClass>(*this), CallableType::CLASS);
// // //     sig->setParameterTypes(parameters.getParameterTypes());
// // //     sig->setSubType(CallableType::NATIVE);
// // //     return sig;
// // // }