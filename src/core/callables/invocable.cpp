#include "core/scope.h"

#include "core/callables/invocalble.h"

Invocable::Invocable(String name, ParamList params, CallableType definedType, bool requiresReturn, bool isStatic)
: Callable(name, params, definedType, requiresReturn, isStatic) {
    DEBUG_FLOW(FlowLevel::NONE);
    DEBUG_FLOW_EXIT();
}




void Invocable::setCapturedScope(SharedPtr<Scope> scope) {
    if (!scope) {throw MerkError("Cannot set a null scope in UserFunction.");}
    capturedScope = scope;
    if (!scope) {throw MerkError("Cannot set a null scope in UserFunction.");}
    getBody()->setScope(capturedScope);
    if (!getBody()->getScope()) {throw MerkError("Invocable::setCapturedScope -> The Body Has No Scope After Being Set");}
    capturedScope->owner = generateScopeOwner(callableTypeAsString(callType), name);
}


SharedPtr<Scope> Invocable::getCapturedScope() const {
    return capturedScope;
}


