#include <iostream>
#include "core/types.h"
#include "core/functions/argument_node.h"
#include "utilities/debugger.h"
#include "utilities/helper_functions.h"
#include "core/node.h"
#include "core/scope.h"

#include "core/evaluator.h"
#include "ast/ast.h"

#include "ast/ast_callable.h"
#include "ast/ast_chain.h"
#include "core/classes/method.h" 
#include "core/classes/class_base.h"
#include "ast/ast_class.h"
#include "ast/exceptions.h"
#include "ast/ast_validate.h"


ClassBody::ClassBody(UniquePtr<CodeBlock>&& block)
    : CallableBody(block->getScope()) {
    this->children = std::move(block->children);

    block.reset();
}
ClassBody::ClassBody(SharedPtr<Scope> scope)
    : CallableBody(scope) {}

MethodBody::MethodBody(UniquePtr<CodeBlock>&& body) : CallableBody(std::move(body)){}
MethodBody::MethodBody(SharedPtr<Scope> scope) : CallableBody(scope) {}
MethodBody::MethodBody(UniquePtr<CallableBody>* body) : CallableBody(std::move(body)) {}

ClassCall::ClassCall(String name, Vector<UniquePtr<ASTStatement>> arguments, SharedPtr<Scope> scope)
    : CallableCall(name, std::move(arguments), scope) {
    branch = "Classical";
}

ClassDef::ClassDef(String name, ParamList parameters, UniquePtr<ClassBody> body, String accessor, SharedPtr<Scope> scope)
    : CallableDef(name, std::move(parameters), std::move(body), CallableType::CLASS, scope), accessor(accessor) {}
void ClassDef::setClassAccessor(String accessorName){accessor = accessorName;}

String ClassDef::getClassAccessor() {return accessor;}
    
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

ParamList& ClassDef::getParameters() {return parameters;}
ParamList& MethodDef::getParameters() {return parameters;}

const ParamList& MethodDef::getParameters() const {return parameters;}

MethodCall::MethodCall(String name, Vector<UniquePtr<ASTStatement>> arguments, SharedPtr<Scope> scope)
    : CallableCall(name, std::move(arguments), scope) {
    branch = "Callable";
}


void MethodDef::setMethodAccessor(String& accessorName) {accessor = accessorName;}

String MethodDef::getMethodAccessor() const {return accessor;}

void MethodDef::setNonStaticElements(Vector<Chain*> nonStaticEls) {
    nonStaticElements = nonStaticEls;
}

Vector<Chain*> MethodBody::getNonStaticElements() {
    return nonStaticElements;
}

bool MethodBody::getIsStatic() {
    return isStatic;
}

void MethodBody::setNonStaticElements(Vector<Chain*> nonStaticEls){
    nonStaticElements = nonStaticEls;
    if (nonStaticElements.size() > 0) {
        isStatic = true;
    }
}

void ClassBody::setAccessor(String& classAccessor) {accessor = classAccessor;}

bool MethodDef::isConstructor() const {return name == "construct";}

void ClassBody::setCapturedScope(SharedPtr<Scope> capturedScope) {classCapturedScope = capturedScope;}

void ClassBody::setClassScope(SharedPtr<Scope> newScope) {classScope = newScope;}

SharedPtr<Scope> MethodDef::getClassScope() const {return classScope;}

void MethodDef::setClassScope(SharedPtr<Scope> scope) {
    if (!scope) {
        DEBUG_LOG(LogLevel::ERROR, highlight("Setting MethodDef Scope Failed", Colors::yellow));

        throw MerkError("MethodDef new ClassScope is null");
    }
    classScope = scope;
}

Accessor::Accessor(String accessor, SharedPtr<Scope> scope)
    : ASTStatement(scope), accessor(std::move(accessor)) {
}

void Accessor::setScope(SharedPtr<Scope> scope) {(void)scope;}

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

Node ClassDef::evaluate(SharedPtr<Scope> defScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    
    if (!defScope) {throw MerkError("No Scope Was Found in ClassDef::evaluate()");}

    if (!body->getScope()){throw MerkError("scope not present in ClassDef::evaluate in body");}

    FreeVars freeVarNames = body->collectFreeVariables();
    if (freeVarNames.size() == 0) {DEBUG_LOG(LogLevel::TRACE, "There Are No Free Variables in classdef: ", name);}

    SharedPtr<Scope> classDefCapturedScope = defScope->detachScope(freeVarNames);

    if (!classDefCapturedScope) {throw MerkError("Failed to create detachedScope in ClassDef::evaluate");}

    if (!parameters.eraseByName(accessor)) {throw MerkError("No Accessor Was Provided In Class Constructor");}

    SharedPtr<Scope> classScope = classDefCapturedScope->makeCallScope();
    classScope->isDetached = true; // detached until ClassBase owns it
    classScope->owner = generateScopeOwner("ClassMainScopeClass", name);


    if (!classScope){throw MerkError("classScope was not created correctly on the ClassBase.");}
    DEBUG_LOG(LogLevel::TRACE, highlight("Attempting captured setting on cls", Colors::yellow));

    SharedPtr<ClassBase> cls = makeShared<ClassBase>(name, accessor, classScope);
    
    cls->setCapturedScope(classDefCapturedScope);
    // cls->setParameters(classBase)
    DEBUG_LOG(LogLevel::PERMISSIVE, "SETTING CLASSSIG PARAMS");
    cls->setParameters(parameters.clone());
    DEBUG_LOG(LogLevel::PERMISSIVE, "Params: ", cls->getParameters());
    if (!cls->getCapturedScope()) {throw MerkError("CapturedScope was not set correctly on the ClassBase.");}

    auto* classBody = static_cast<ClassBody*>(getBody());
    classBody->setClassScope(classScope);
    if (!cls->getClassScope()){throw MerkError("ClassDef::evlaute classScope is null");}

    classBody->setCapturedScope(cls->getCapturedScope());
    String bodyAccess = accessor;
    classBody->setAccessor(bodyAccess);

    DEBUG_LOG(LogLevel::TRACE, "ClassScope Below: ");    

    classBody->evaluate(cls->getClassScope());

    if (!cls->getClassScope()->hasFunction("construct")) {
        cls->getClassScope()->debugPrint();
        cls->getClassScope()->printChildScopes();
        throw MerkError("Class '" + name + "' must implement a 'construct' method.");
    }
    
    
    classBody->setCapturedScope(nullptr);
    classBody->setClassScope(nullptr);
    classBody->getScope()->owner = generateScopeOwner("ClassBody", name);
    defScope->appendChildScope(classDefCapturedScope, "ClassDef::evaluate");
    classDefCapturedScope->isCallableScope = true;
    classDefCapturedScope->owner = generateScopeOwner("ClassDef--InitialCaptured", name);
    classDefCapturedScope->appendChildScope(cls->getClassScope());

    defScope->registerClass(name, cls);
    
    DEBUG_LOG(LogLevel::TRACE, "ClassDef::classScope created: ");
    auto classNode = ClassNode(cls);
    
    DEBUG_FLOW_EXIT();
    return classNode;
}

Node MethodBody::evaluate() const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    throw MerkError(highlight("EVALUATING METHOD BODY WITH NO INSTANCE SCOPE: ", Colors::bg_bright_magenta));
    DEBUG_FLOW_EXIT();
    return Node();
}

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

Node ClassBody::evaluate(SharedPtr<Scope> classScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    return Evaluator::evaluateClassBody(classCapturedScope, classScope, getScope(), accessor, getMutableChildren());
}

Node ClassCall::evaluate(SharedPtr<Scope> callScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!callScope){throw MerkError("Initial Scope Failed in ClassCall::evaluate()");}
    if (!getScope()) {throw MerkError("ClassCall::evaluate(): getScope() is null");}

    // DEBUG_LOG(LogLevel::PERMISSIVE, highlight("Displaying Arguments", Colors::bold_purple));
    Vector<Node> argValues = handleArgs(callScope);
    if (argValues.size() < arguments.size()){
        throw MerkError("Arg Values and Arguments Don't Match in ClassCall::evaluate");
    }
    if (argValues.size() == 0){
        throw MerkError("There Are No argValues in ClassCall::evaluate.");
    }

    
    DEBUG_FLOW_EXIT();

    return Evaluator::evaluateClassCall(callScope, name, argValues, instanceNode);
}

Node MethodDef::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    if (!scope){
        throw MerkError("Provided Scope to MethodDef::evaluate is null");
    }

    if (!getScope()){
        throw MerkError("MethodDef::evaluate, scope is null");
    }

    if (!getClassScope()) {
        throw MerkError("Class Scope was not supplied to Method: " + name);
    }

    // auto subType = callType;
    // auto primaryType = CallableType::METHOD;
    return Evaluator::evaluateMethodDef(scope, getScope(), getClassScope(), name, getBody(), parameters, callType, methodType);
}

Node Accessor::evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) const {
    (void)scope;
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    if (!instanceNode || !instanceNode->isClassInstance()) throw MerkError("No instance provided to Accessor");

    ClassInstanceNode instance = instanceNode->getInstanceNode(); 

    DEBUG_FLOW_EXIT();
    return instance;
}


ClassDef::~ClassDef() {
    getBody().reset();
}

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

ClassCall::~ClassCall() {
    getScope()->clear();
}

MethodCall::~MethodCall() {
    getScope()->clear();
    DEBUG_LOG(LogLevel::TRACE, "Destroying MethodCall:" + name);
}

MethodBody::~MethodBody() {
    clear();
}

ClassBody::~ClassBody() {
    clear();
}

