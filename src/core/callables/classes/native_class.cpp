#include "core/types.h"
#include "core/callables/classes/native_method.h"
#include "core/callables/classes/native_class.h"
#include "core/callables/argument_node.h"
 
NativeClass::NativeClass(String name, String access, SharedPtr<Scope> classScope)
: ClassBase(std::move(name), std::move(access), classScope) {
    setSubType(CallableType::NATIVE);
    // if (callableTypeAsString(getCallableType()) != "CLASS") {throw MerkError("NativeClass is not Typed as Class");}
    // if (callableTypeAsString(getSubType()) != "NATIVE") {throw MerkError("NativeClass is not subTyped as Native");}
}

void NativeClass::addMethod(const String& name, SharedPtr<NativeMethod> method) {
    getClassScope()->registerFunction(name, method);
}

void NativeClass::setConstructor(std::function<void(ArgResultType, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode>)> fn) {
    constructorFn = std::move(fn);
}

NativeClass::~NativeClass() {
    if (getCapturedScope()) {getCapturedScope()->clear();}
    if (getClassScope()) {getClassScope()->clear();}
}

Node NativeClass::execute(ArgResultType args,
                SharedPtr<Scope> scope,
                [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const  {
    if (!instanceNode) {throw MerkError("An InstanceNode must be supplied to NativeClass::execute");}
    // auto instancePtr = makeShared<ClassInstance>(getName(), scope, classScope, parameters, accessor);
    // auto ensuredInstanceNode = makeShared<ClassInstanceNode>(instancePtr);

    if (constructorFn) {constructorFn(args, scope, instanceNode);}

    return instanceNode->getInstanceNode();
}

String NativeClass::toString() const  {
    return "NativeClass(" + name + ")";
}

SharedPtr<CallableSignature> NativeClass::toCallableSignature() {
    auto sig = makeShared<CallableSignature>(shared_from_this());

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
}


void NativeClass::setScope(SharedPtr<Scope> newScope) const {
    MARK_UNUSED_MULTI(newScope);
}



// // SharedPtr<CallableSignature> NativeClass::toCallableSignature() const {
// //     auto sig = makeShared<CallableSignature>(makeShared<NativeClass>(*this), CallableType::CLASS);
// //     sig->setParameterTypes(parameters.getParameterTypes());
// //     sig->setSubType(CallableType::NATIVE);
// //     return sig;
// // }