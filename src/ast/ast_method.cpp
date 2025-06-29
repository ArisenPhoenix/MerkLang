

#include "core/types.h"
#include "core/scope.h"
#include "core/evaluator.h"
#include "core/callables/classes/method.h"

#include "ast/ast_method.h"


MethodDef::~MethodDef() {
    if (body) {
        body->clear();  // Clear children
        body.reset();   // Fully destroy body first
    }

    if (getScope()) {
        getScope()->clear();
    }

    if (getClassScope()) {
        getClassScope()->clear();
    }
}

MethodBody::~MethodBody() {
    clear();
}

MethodCall::~MethodCall() {
    if (getScope()) {
        getScope()->clear();
        DEBUG_LOG(LogLevel::TRACE, "Destroying MethodCall:" + name);    
    }
    
}


MethodBody::MethodBody(UniquePtr<CodeBlock>&& body) : CallableBody(std::move(body)){}
MethodBody::MethodBody(SharedPtr<Scope> scope) : CallableBody(scope) {}
MethodBody::MethodBody(UniquePtr<CallableBody>* body) : CallableBody(std::move(body)) {}


MethodDef::MethodDef(String name, ParamList parameters, UniquePtr<MethodBody> body, CallableType funcType, SharedPtr<Scope> scope)
: CallableDef(name, std::move(parameters), std::move(body), CallableType::METHOD, scope) {
    if (funcType == CallableType::METHOD){
        throw MerkError("The CallableType Provided was METHOD");
    }

    
    this->methodType = funcType;
    if (this->methodType == CallableType::METHOD) {
        throw MerkError("MethodDef::methodType is a METHOD");
    } 
    

}

MethodDef::MethodDef(UniquePtr<FunctionDef> funcDef)
    : CallableDef(funcDef->getName(), funcDef->getParameters(), makeUnique<MethodBody>(std::move(funcDef->getBody())), CallableType::METHOD, funcDef->getScope()) 
{
    methodType = funcDef->callType;
    funcDef.reset();
}

MethodCall::MethodCall(String name, Vector<UniquePtr<ASTStatement>> arguments, SharedPtr<Scope> scope)
    : CallableCall(name, std::move(arguments), scope) {
    branch = "Callable";
}

SharedPtr<Scope> MethodDef::getClassScope() const {return classScope;}
Node MethodBody::evaluate(SharedPtr<Scope> callScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::NONE);
    if (!callScope){
        throw MerkError("There Is No callScope provided to MethodBody::evaluate");
    }
    if (!instanceNode){
        throw MerkError("No instanceNode was provided to MethodBody::evaluate");
    }

    auto val = Evaluator::evaluateMethodBody(getMutableChildren(), callScope, instanceNode);
    DEBUG_FLOW_EXIT();
    return val;
}

Node MethodCall::evaluate([[maybe_unused]] SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode ) const {
    DEBUG_FLOW(FlowLevel::NONE);
    
    if (!scope->hasFunction(name)){
        throw MerkError("Method: " + name + " Couldn't Be Found");
    }

    Vector<Node> evaluatedArgs = handleArgs(scope);
    auto optSig = scope->getFunction(name, evaluatedArgs);

    if (!optSig){
        throw FunctionNotFoundError(name);
    }
    SharedPtr<Method> method = std::static_pointer_cast<Method>(optSig->getCallable());

    if (method->getSubType() == CallableType::NATIVE) {
        method->parameters.verifyArguments(evaluatedArgs); // as opposed to placing them within the callScope

        return method->execute(evaluatedArgs, scope, instanceNode);
    }


    SharedPtr<Scope> callScope = scope->buildMethodCallScope(method, name);

    if (!callScope) {throw MerkError("Scope Is Not Valid In UserFunction::execute->function");}
    
    // DEBUG_LOG(LogLevel::TRACE, "******************************* UserFunction Scope Set *******************************");

    method->placeArgsInCallScope(evaluatedArgs, callScope);
    if (!method->getBody()->getScope()){throw ScopeError("MethodCall method->getBoby()->getScope  created an unusable scope");}   

    Node value = method->execute(evaluatedArgs, callScope, instanceNode);
    // scope->removeChildScope(callScope);
    DEBUG_FLOW_EXIT();
    return value; 
}
void MethodDef::setClassScope(SharedPtr<Scope> scope) {
    if (!scope) {
        DEBUG_LOG(LogLevel::ERROR, highlight("Setting MethodDef Scope Failed", Colors::yellow));

        throw MerkError("MethodDef new ClassScope is null");
    }
    classScope = scope;
}

void MethodDef::setMethodAccessor(String& accessorName) {accessor = accessorName;}

String MethodDef::getMethodAccessor() const {return accessor;}

void MethodDef::setNonStaticElements(Vector<Chain*> nonStaticEls) {
    nonStaticElements = nonStaticEls;
}

bool MethodBody::getIsStatic() {
    return getNonStaticElements().size() > 0;
}

void MethodBody::setNonStaticElements(Vector<Chain*> nonStaticEls){
    nonStaticElements = nonStaticEls;
}

bool MethodDef::isConstructor() const {return name == "construct";}



Node MethodDef::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::NONE);

    if (!scope){
        throw MerkError("Provided Scope to MethodDef::evaluate is null");
    }

    if (!getScope()){
        throw MerkError("MethodDef::evaluate, scope is null");
    }

    if (!getClassScope()) {
        throw MerkError("Class Scope was not supplied to Method: " + name);
    }

    return Evaluator::evaluateMethodDef(scope, getScope(), getClassScope(), name, getBody(), parameters, methodType, instanceNode);
}
