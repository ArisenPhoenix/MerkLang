

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
    getScope()->clear();
    DEBUG_LOG(LogLevel::TRACE, "Destroying MethodCall:" + name);
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
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
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
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    scope->owner = generateScopeOwner("MethodCall", name);
    Vector<Node> evaluatedArgs = handleArgs(scope);


    // DEBUG_LOG(LogLevel::PERMISSIVE, "HANDLED ARGS IN METHOD CALL :: EVALUATE");

    if (!scope->hasFunction(name)){
        throw MerkError("Method: " + name + " Couldn't Be Found");
    }
    DEBUG_LOG(LogLevel::ERROR, highlight("Found Function " + name, Colors::yellow));
    
    auto optSig = scope->getFunction(name, evaluatedArgs);

    if (!optSig){
        throw FunctionNotFoundError(name);
    }
    SharedPtr<Method> method = std::static_pointer_cast<Method>(optSig->getCallable());
    SharedPtr<Scope> callScope = method->getCapturedScope();
    callScope->owner = generateScopeOwner("MethodCall", name);

    callScope = callScope->makeCallScope();
    callScope->owner = generateScopeOwner("MethodCall", name);
    
    callScope->owner = "MethodCall:evaluate (" + name + ")";

    if (!callScope){
        throw MerkError("Scope Is Not Valid In MethodCall::execute->method");
    }
    
    DEBUG_LOG(LogLevel::DEBUG, "******************************* Method Scope Set *******************************");

    method->placeArgsInCallScope(evaluatedArgs, callScope);

    scope->appendChildScope(callScope, "MethodCall::evaluate");
    
    method->setCapturedScope(callScope);
    
    if (!method->getBody()->getScope()){
        throw ScopeError("MethodCall func->getBoby()->getScope  created an unusable scope");
    }
   

    Node value = method->execute(evaluatedArgs, callScope, instanceNode);
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
