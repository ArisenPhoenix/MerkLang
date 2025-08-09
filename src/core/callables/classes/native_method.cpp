#include "core/node/argument_node.h"

#include "core/types.h"

#include "core/callables/classes/class_base.h"
#include "core/callables/classes/native_method.h"


NativeMethod::NativeMethod(
    String name,
    ParamList params,
    SharedPtr<Scope> classScope,
    std::function<Node(ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self)> methodFn) 
    : Method(std::move(name), params, CallableType::METHOD), methodFn(methodFn) {
        DEBUG_FLOW(FlowLevel::NONE);
        setClassScope(classScope);
        setSubType(CallableType::NATIVE);
        if (subType != CallableType::NATIVE) {throw MerkError("SubType is not native, it is: " + callableTypeAsString(subType));}
        DEBUG_FLOW_EXIT();
    }


NativeMethod::~NativeMethod() {

};

Node NativeMethod::execute(
    ArgResultType args,
    SharedPtr<Scope> callScope,
    SharedPtr<ClassInstanceNode> instanceNode) const 
{
    if (!instanceNode) {throw RunTimeError("NativeMethod called without class instance.");}
    return methodFn(args, callScope, instanceNode);
}

String NativeMethod::toString() const {
    return "NativeMethod(" + name + ")";
}

SharedPtr<CallableSignature> NativeMethod::toCallableSignature() {
    auto sig = makeShared<CallableSignature>(shared_from_this(), CallableType::METHOD);
    if (getSubType() != CallableType::NATIVE) {throw MerkError("Is Not A Native type is: " + callableTypeAsString(subType));}
    sig->setParameterTypes(parameters.getParameterTypes());
    sig->setSubType(getSubType());
    sig->setParameters(parameters.clone());
    return sig;
}


void NativeMethod::setCapturedScope(SharedPtr<Scope> scope){
    setClassScope(scope);
};


SharedPtr<Scope> NativeMethod::getCapturedScope() const {
    return getClassScope();
};


void NativeMethod::setScope(SharedPtr<Scope> newScope) const {
    MARK_UNUSED_MULTI(newScope);
};












// virtual MethodBody* getThisBody() = 0;
// virtual UniquePtr<CallableBody> getBody() override = 0;
// virtual CallableBody* getBody() const override = 0;
// virtual CallableBody* getInvocableBody() override = 0;


MethodBody* NativeMethod::getThisBody() {return nullptr;}
UniquePtr<CallableBody> NativeMethod::getBody() {return nullptr;}
CallableBody* NativeMethod::getBody() const {return nullptr;}
CallableBody* NativeMethod::getInvocableBody() {return nullptr;}