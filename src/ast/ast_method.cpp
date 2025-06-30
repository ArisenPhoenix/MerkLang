

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
    if (!callScope){throw MerkError("There Is No callScope provided to MethodBody::evaluate");}
    if (!instanceNode){throw MerkError("No instanceNode was provided to MethodBody::evaluate");}

    auto val = Evaluator::evaluateMethodBody(getMutableChildren(), callScope, instanceNode);
    DEBUG_FLOW_EXIT();
    return val;
}

Node MethodCall::evaluate([[maybe_unused]] SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode ) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE); 
    if (!instanceNode) {throw MerkError("MethodCall::evaluate -> no instanceNode passed");}
    if (!scope) {throw MerkError("MethodCall::evaluate -> scope passed to is null");}

    auto instanceScope = instanceNode->getInstanceScope();
    if (!instanceScope) {throw MerkError("MethodCall::evaluate -> instanceScope is null");}
    auto scopeParent = scope->getParent();
    if (!instanceScope->has(scope) && !scope->has(instanceScope)) {instanceScope->appendChildScope(scope);}
    
    if (!instanceScope->hasFunction(name)){throw MerkError("Method: " + name + " Couldn't Be Found");}
    
    // auto argScope = scope->isolateScope({});
    // auto argScope = scope->makeCallScope();

    
    
    NodeList evaluatedArgs = handleArgs(scope, instanceNode);
    auto str = joinVectorNodeStrings(evaluatedArgs);
    DEBUG_LOG(LogLevel::PERMISSIVE, highlight("EVALUTED ARGS ============ " + str + " ============", Colors::bg_blue));
    
    auto optSig = instanceScope->getFunction(name, evaluatedArgs);
    
    if (!optSig){throw FunctionNotFoundError(name);}

    SharedPtr<Method> method = std::static_pointer_cast<Method>(optSig->getCallable());
    DEBUG_LOG(LogLevel::PERMISSIVE, "Method CapturedScope");
    method->getCapturedScope()->debugPrint();

    if (method->getSubType() == CallableType::NATIVE) {
        method->parameters.verifyArguments(evaluatedArgs); // as opposed to placing them within the callScope
        return method->execute(evaluatedArgs, scope, instanceNode);
    }

    if (method->getIsStatic()) {
        DEBUG_LOG(LogLevel::PERMISSIVE, "Executing Static Method");
        throw MerkError("Executing Static Method");
        method->execute(evaluatedArgs, scope, instanceNode);
    }

    SharedPtr<Scope> callScope = scope->buildMethodCallScope(method, method->getName());

    if (!callScope) {throw MerkError("Scope Is Not Valid In UserFunction::execute->function");}
    
    DEBUG_LOG(LogLevel::TRACE, "******************************* UserFunction Scope Set *******************************");

    // method->placeArgsInCallScope(evaluatedArgs, callScope);
    
    if (!method->getBody()->getScope()){throw ScopeError("FunctionCall func->getBoby()->getScope  created an unusable scope");}
    if (!callScope) {throw MerkError("FunctionCall:evaluate callScope being passed to func->execute is null");}

    DEBUG_LOG(LogLevel::PERMISSIVE, "Call Scope passed for execution");
    // callScope->debugPrint();
    
    Node value = method->execute(evaluatedArgs, callScope, instanceNode);

    // cleanup
    // scope->removeChildScope(callScope);
    
    instanceScope->removeChildScope(scope);
    scope->setParent(scopeParent);
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
    body->setNonStaticElements(nonStaticElements);
}

ParamList& MethodDef::getParameters() {return parameters;}

const ParamList& MethodDef::getParameters() const {return parameters;}


bool MethodDef::isConstructor() const {return name == "construct";}

Vector<Chain*> MethodDef::getNonStaticElements() {
    return nonStaticElements;
}

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
    auto methodBody = static_cast<MethodBody*>(getBody());
    return Evaluator::evaluateMethodDef(scope, getScope(), getClassScope(), name, methodBody, parameters, methodType, instanceNode);
}
