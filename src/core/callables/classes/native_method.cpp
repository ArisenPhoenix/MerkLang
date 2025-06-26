// #include "core/types.h"

// #include "core/callables/classes/class_base.h"
// #include "core/callables/classes/native_method.h"



// NativeMethod::NativeMethod(
//     String name,
//     ParamList params,
//     SharedPtr<Scope> classScope,
//     std::function<Node(NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self)> methodFn) 
//     : Method(name, params, CallableType::METHOD), methodFn(methodFn) {
//         setClassScope(classScope);
//     }


// NativeMethod::~NativeMethod() = default;

// Node NativeMethod::execute(
//     Vector<Node> args,
//     SharedPtr<Scope> callScope,
//     SharedPtr<ClassInstanceNode> instanceNode) const 
// {
//     if (!instanceNode) {
//         throw RunTimeError("NativeMethod called without class instance.");
//     }
//     return methodFn(args, callScope, instanceNode);
// }

// String NativeMethod::toString() const {
//     return "NativeMethod(" + name + ")";
// }

// SharedPtr<CallableSignature> NativeMethod::toCallableSignature() {
//     auto sig = makeShared<CallableSignature>(makeShared<NativeMethod>(*this), CallableType::METHOD);
//     sig->setParameterTypes(parameters.getParameterTypes());
//     sig->setSubType(CallableType::METHOD);
//     sig->setParameters(parameters.clone());
//     return sig;
// }


// void NativeMethod::setCapturedScope(SharedPtr<Scope> scope){
//     // (void)scope;
//     setClassScope(scope);
// };


// SharedPtr<Scope> NativeMethod::getCapturedScope() const {
//     return getClassScope();
// };


// void NativeMethod::setScope(SharedPtr<Scope> newScope) const {
//     (void)newScope;
// };