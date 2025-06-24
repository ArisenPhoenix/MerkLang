// #include "core/types.h"

// #include "core/classes/class_base.h"
// #include "core/classes/native_method.h"



// NativeMethod::NativeMethod(
//     String name,
//     ParamList params,
//     SharedPtr<Scope> classScope,
//     std::function<Node(NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self)> methodFn) 
//     : classScope(classScope) {}


// NativeMethod::~NativeMethod() = default;

// Node NativeMethod::execute(Vector<Node> args,
//                 SharedPtr<Scope> callScope,
//                 SharedPtr<ClassInstanceNode> instanceNode = nullptr) const {
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
//     (void)scope;
// };


// SharedPtr<Scope> NativeMethod::getCapturedScope() const {
//     return classScope;
// };


// void NativeMethod::setScope(SharedPtr<Scope> newScope) const {
//     (void)newScope;
// };