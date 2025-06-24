#include "core/functions/native_function.h"
#include "core/types.h"
#include "core/scope.h"

NativeFunction::NativeFunction(
    String name,
    ParamList params,
    std::function<Node(Vector<Node>, SharedPtr<Scope>, SharedPtr<ClassInstanceNode>)> impl

)
    : Function(std::move(name), std::move(params), CallableType::FUNCTION),
      nativeImpl(std::move(impl)) {
    setSubType(CallableType::NATIVE);
}

Node NativeFunction::execute(Vector<Node> args, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) const {
    // (void)instanceNode;
    if (!nativeImpl) {
        throw MerkError("NativeFunction has no implementation.");
    }
     
    return nativeImpl(args, scope, instanceNode);
}

SharedPtr<CallableSignature> NativeFunction::toCallableSignature() {
    SharedPtr<CallableSignature> sig = makeShared<CallableSignature>(
        shared_from_this(), getCallableType()
    );

    if (!sig) {
        throw MerkError("Native Function Callable Signatrure Was Not Created");
    }
    sig->setSubType(CallableType::NATIVE);
    sig->setParameters(parameters.clone());
    return sig;
}

void NativeFunction::setCapturedScope(SharedPtr<Scope> scope) {
    capturedScope = scope;
}

SharedPtr<Scope> NativeFunction::getCapturedScope() const {
    return capturedScope;
}

void NativeFunction::setScope(SharedPtr<Scope> newScope) const {
    if (newScope) {
        newScope->owner = generateScopeOwner("NativeFunction", name);
    }
}

String NativeFunction::toString() const {
    return "<NativeFunction: " + getName() + ">";
}
