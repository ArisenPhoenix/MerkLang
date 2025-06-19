#include <iostream>
#include "core/types.h"
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
    
MethodDef::MethodDef(String name, ParamList parameters, UniquePtr<MethodBody> body, CallableType methodType, SharedPtr<Scope> scope)
: CallableDef(name, std::move(parameters), std::move(body), methodType, scope) {
}

MethodDef::MethodDef(UniquePtr<FunctionDef> funcDef)
    : CallableDef(funcDef->getName(), funcDef->getParameters(), makeUnique<MethodBody>(std::move(funcDef->getBody())), CallableType::METHOD, funcDef->getScope()) 
{
    funcDef.reset();
}

ParamList& ClassDef::getParameters() {return parameters;}
ParamList& MethodDef::getParameters() {return parameters;}
const ParamList& MethodDef::getParameters() const {return parameters;}

MethodCall::MethodCall(String name, Vector<UniquePtr<ASTStatement>> arguments, SharedPtr<Scope> scope)
    : CallableCall(name, std::move(arguments), scope) {
}

Node MethodCall::evaluate([[maybe_unused]] SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode ) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    scope->owner = generateScopeOwner("MethodCall", name);
    Vector<Node> evaluatedArgs = handleArgs(scope);

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

    method->parameters.verifyArguments(evaluatedArgs);
    // For each parameter, declare it in the new scope.
    for (size_t i = 0; i < method->parameters.size(); ++i) {
        VarNode paramVar(evaluatedArgs[i]);
        callScope->declareVariable(method->parameters[i].getName(), makeUnique<VarNode>(paramVar));
    }

    scope->appendChildScope(callScope, "MethodCall::evaluate");
    
    method->setCapturedScope(callScope);
    
    if (!method->getBody()->getScope()){
        throw ScopeError("MethodCall func->getBoby()->getScope  created an unusable scope");
    }
   

    Node value = method->execute(evaluatedArgs, callScope, instanceNode);
    DEBUG_FLOW_EXIT();
    return value; 
}



void MethodDef::setMethodAccessor(String& accessorName) {accessor = accessorName;}
String MethodDef::getMethodAccessor() const {return accessor;}




Node ClassDef::evaluate(SharedPtr<Scope> defScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    
    if (!defScope) {throw MerkError("No Scope Was Found in ClassDef::evaluate()");}

    if (!body->getScope()){throw MerkError("scope not present in ClassDef::evaluate in body");}

    FreeVars freeVarNames = body->collectFreeVariables();
    if (freeVarNames.size() == 0) {DEBUG_LOG(LogLevel::TRACE, "There Are No Free Variables in classdef: ", name);}

    SharedPtr<Scope> classDefCapturedScope = defScope->detachScope(freeVarNames);

    if (!classDefCapturedScope) {throw MerkError("Failed to create detachedScope in ClassDef::evaluate");}



    // auto classScope = classDefCapturedScope->createChildScope();
    // auto classScope = classDefCapturedScope->makeCallScope();
    SharedPtr<Scope> classScope = classDefCapturedScope->makeCallScope();
    classScope->isDetached = true; // detached until ClassBase owns it
    classScope->owner = generateScopeOwner("ClassMainScopeClass", name);


    if (!classScope){throw MerkError("classScope was not created correctly on the ClassBase.");}
    DEBUG_LOG(LogLevel::TRACE, highlight("Attempting captured setting on cls", Colors::yellow));
    // classScope->debugPrint();

    SharedPtr<ClassBase> cls = makeShared<ClassBase>(name, accessor, classScope);
    cls->setParameters(parameters.clone());
    cls->setCapturedScope(classDefCapturedScope);
    if (!cls->getCapturedScope()) {throw MerkError("CapturedScope was not set correctly on the ClassBase.");}

    auto* classBody = static_cast<ClassBody*>(getBody());
    classBody->setClassScope(classScope);
    if (!cls->getClassScope()){throw MerkError("ClassDef::evlaute classScope is null");}

    classBody->setCapturedScope(cls->getCapturedScope());
    String bodyAccess = accessor;
    classBody->setAccessor(bodyAccess);

    DEBUG_LOG(LogLevel::TRACE, "ClassScope Below: ");
    // cls->getClassScope()->debugPrint();
    

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


    // defScope->appendChildScope(classDefCapturedScope, "ClassDef::evaluate");
    defScope->registerClass(name, cls);
    // defScope->owner = generateScopeOwner("ClassDef Outer Scope", name);
    
    DEBUG_LOG(LogLevel::TRACE, "ClassDef::classScope created: ");
    // if (cls->getClassScope()->hasVariable("x")){throw MerkError("ClassDef::evaluate -> cls Already Has Variable 'x' at instance construction time.");}

    
    DEBUG_FLOW_EXIT();
    return ClassNode(cls);
}

void MethodDef::setNonStaticElements(Vector<Chain*> nonStaticEls) {
    nonStaticElements = nonStaticEls;
}

Node MethodBody::evaluate() const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    throw MerkError(highlight("EVALUATING METHOD BODY WITH NO INSTANCE SCOPE: ", Colors::bg_bright_magenta));
    DEBUG_FLOW_EXIT();
    return Node();
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

Node MethodBody::evaluate(SharedPtr<Scope> callScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    // DEBUG_LOG(LogLevel::PERMISSIVE, highlight("EVALUATING METHOD BODY WITH INSTANCE SCOPE: ", Colors::bg_bright_magenta), instanceScope ? std::to_string(instanceScope->getScopeLevel()) : "NULL");
    auto val = Evaluator::evaluateMethod(getMutableChildren(), callScope, instanceNode);
    DEBUG_FLOW_EXIT();
    return val;
}

void ClassBody::setAccessor(String& classAccessor) {accessor = classAccessor;}


Node ClassBody::evaluate(SharedPtr<Scope> classScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    return Evaluator::evaluateClassBody(classCapturedScope, classScope, getScope(), accessor, getMutableChildren());
}

Node ClassCall::evaluate(SharedPtr<Scope> callScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!callScope){throw MerkError("Initial Scope Failed in ClassCall::evaluate()");}
    if (!getScope()) {throw MerkError("ClassCall::evaluate(): getScope() is null");}
    Vector<Node> argValues = handleArgs(callScope);

    return Evaluator::evaluateClassCall(callScope, name, argValues);
}

bool MethodDef::isConstructor() const {return name == "construct";}

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

    return Evaluator::evaluateMethodDef(scope, getScope(), getClassScope(), name, getBody(), parameters, callType, methodType);
}


ClassDef::~ClassDef() {
    getBody().reset();
    // DEBUG_LOG(LogLevel::ERROR, "Destroying ClassDef:" + name);
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


    // DEBUG_LOG(LogLevel::ERROR, "Destroying MethodDef:" + name);
}

ClassCall::~ClassCall() {
    getScope()->clear();
    // DEBUG_LOG(LogLevel::ERROR, "Destroying ClassCall:" + name);
}

MethodCall::~MethodCall() {
    getScope()->clear();
    DEBUG_LOG(LogLevel::TRACE, "Destroying MethodCall:" + name);
}

MethodBody::~MethodBody() {
    clear();
    // DEBUG_LOG(LogLevel::ERROR, "Destroying MethodBody:");
}


ClassBody::~ClassBody() {
    clear();
    // DEBUG_LOG(LogLevel::DEBUG, "Destroying Class Body");
}

Accessor::Accessor(String accessor, SharedPtr<Scope> scope)
    : ASTStatement(scope), accessor(std::move(accessor)) {
}

Node Accessor::evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) const {
    (void)scope;
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    if (!instanceNode || !instanceNode->isClassInstance()) throw MerkError("No instance provided to Accessor");

    ClassInstanceNode instance = instanceNode->getInstanceNode(); 

    DEBUG_FLOW_EXIT();
    return instance;
    // return nodeCopy;
}




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




void Accessor::setScope(SharedPtr<Scope> scope) {
    (void)scope;
}
