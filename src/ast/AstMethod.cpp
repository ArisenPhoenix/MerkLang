#include "core/node/ArgumentNode.hpp"
#include "core/TypesFWD.hpp"
#include "core/Scope.hpp"
#include "core/evaluator.h"
#include "core/callables/classes/Method.hpp"

#include "ast/AstMethod.hpp"
#include "ast/ast_helpers.h"
#include "core/node/NodeStructures.hpp"

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

MethodCall::MethodCall(String name, UniquePtr<ArgumentType> arguments, SharedPtr<Scope> scope)
    : CallableCall(name, std::move(arguments), scope) {
    branch = "Callable";
}

SharedPtr<Scope> MethodDef::getClassScope() const {return classScope;}

Node MethodBody::evaluate(SharedPtr<Scope> callScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::NONE);
    if (!callScope){throw MerkError("There Is No callScope provided to MethodBody::evaluate");}
    if (!instanceNode){throw MerkError("No instanceNode was provided to MethodBody::evaluate");}
    // setScope(callScope);
    auto val = Evaluator::evaluateMethodBody(getMutableChildren(), callScope, instanceNode);
    DEBUG_FLOW_EXIT();
    return val;
}

Node MethodCall::evaluate([[maybe_unused]] SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode ) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE); 
    if (!instanceNode) {throw MerkError("MethodCall::evaluate -> " + name + " no instanceNode passed");}
    if (!scope) {throw MerkError("MethodCall::evaluate -> scope passed to is null");}

    auto instanceScope = instanceNode->getInstanceScope();
    if (!instanceScope) {throw MerkError("No Instance Scope");}

    auto executionScope = scope->createChildScope();
    executionScope->owner = generateScopeOwner("MethodExecutor", name);
    
    auto args = handleArgs(executionScope, instanceNode);
  
    auto optSig = instanceScope->getFunction(name, args);
    SharedPtr<CallableSignature> methodSig = nullptr;

    if (optSig.has_value()) {
        methodSig = optSig.value();
    } else {
        if (scope->hasVariable(name)) {
            auto& var = scope->getVariable(name);
            if (var.isFunctionNode()) {
                auto funcNode = var.toFunctionNode();
                optSig = funcNode.getFunction(name, args);
                if (optSig.has_value()) {
                    methodSig = optSig.value();
                }
            }
        }
        

        if (!methodSig) {


            if (name == "append") {
                scope->debugPrint();
                throw MerkError("Didnt' Find append");
            }
            scope->debugPrint();
            throw MerkError("Didn't find " + name);
            auto returnVal = handleVirtualMethod(instanceNode, name);
            scope->removeChildScope(executionScope);
            executionScope->clear();
            return returnVal;
        }
    }

    if (!methodSig) {throw MerkError("Method not a signature");}
    SharedPtr<Method> method = std::static_pointer_cast<Method>(methodSig->getCallable());
    if (!method) {throw MerkError("Method " + name + " not found for given arguments");}


    if (method->getSubType() == CallableType::NATIVE) {
        return method->execute(args, scope, instanceNode);
    }

    
    SharedPtr<Scope> callScope = executionScope->buildMethodCallScope(method, name);

    // instanceScope->appendChildScope(callScope, false);
    auto val = method->execute(args, callScope, instanceNode);
    // instanceScope->removeChildScope(callScope);

    scope->removeChildScope(executionScope);
    executionScope->clear();
    // instanceScope->removeChildScope(executionScope);
    
    // DEBUG_LOG(LogLevel::PERMISSIVE, "METHOD(", name, ")", "RETURNED VAL: ", val.getFlags().toString());
    // if (name == "get") {throw MerkError("Called 'get' -> " + val.getInner()->toString() + " " + val.getFlags().toString());}
    DEBUG_FLOW_EXIT();
    return val;
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
