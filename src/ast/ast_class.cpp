#include <iostream>
#include "core/types.h"
#include "core/callables/argument_node.h"
#include "utilities/debugger.h"
#include "utilities/helper_functions.h"
#include "core/node.h"
#include "core/scope.h"

#include "core/evaluator.h"
#include "ast/ast.h"

#include "ast/ast_callable.h"
#include "ast/ast_chain.h"
#include "core/callables/classes/method.h" 
#include "core/callables/classes/class_base.h"
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


ClassCall::ClassCall(String name, Vector<UniquePtr<ASTStatement>> arguments, SharedPtr<Scope> scope)
    : CallableCall(name, std::move(arguments), scope) {
    branch = "Classical";
}

ClassDef::ClassDef(String name, ParamList parameters, UniquePtr<ClassBody> body, String accessor, SharedPtr<Scope> scope)
    : CallableDef(name, std::move(parameters), std::move(body), CallableType::CLASS, scope), accessor(accessor) {}
void ClassDef::setClassAccessor(String accessorName){accessor = accessorName;}

String ClassDef::getClassAccessor() {return accessor;}
    
ParamList& ClassDef::getParameters() {return parameters;}

// Vector<Chain*> MethodBody::getNonStaticElements() {
//     return nonStaticElements;
// }


void ClassBody::setAccessor(String& classAccessor) {accessor = classAccessor;}

void ClassBody::setCapturedScope(SharedPtr<Scope> capturedScope) {classCapturedScope = capturedScope;}

void ClassBody::setClassScope(SharedPtr<Scope> newScope) {classScope = newScope;}


Accessor::Accessor(String accessor, SharedPtr<Scope> scope)
    : ASTStatement(scope), accessor(std::move(accessor)) {
}

void Accessor::setScope(SharedPtr<Scope> scope) {(void)scope;}


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
    cls->setParameters(parameters.clone());
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

Node ClassBody::evaluate(SharedPtr<Scope> classScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    return Evaluator::evaluateClassBody(classCapturedScope, classScope, getScope(), accessor, getMutableChildren(), instanceNode);
}

Node ClassCall::evaluate(SharedPtr<Scope> callScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!callScope) {throw MerkError("Initial Scope Failed in ClassCall::evaluate()");}
    if (!getScope()) {throw MerkError("ClassCall::evaluate(): getScope() is null");}

    Vector<Node> argValues = handleArgs(callScope, instanceNode);
    if (argValues.size() < arguments.size()){throw MerkError("Arg Values and Arguments Don't Match in ClassCall::evaluate");}
    // if (argValues.size() == 0) {throw MerkError("There Are No argValues in ClassCall::evaluate.");}

    DEBUG_FLOW_EXIT();

    return Evaluator::evaluateClassCall(callScope, name, argValues, instanceNode);
}

Node Accessor::evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) const {
    (void)scope;
    DEBUG_FLOW(FlowLevel::NONE);

    if (!instanceNode || !instanceNode->isClassInstance()) throw MerkError("No instance provided to Accessor");

    ClassInstanceNode instance = instanceNode->getInstanceNode(); 

    DEBUG_FLOW_EXIT();
    return instance;
}


ClassDef::~ClassDef() {getBody().reset();}

ClassCall::~ClassCall() {getScope()->clear();}

ClassBody::~ClassBody() {clear();}



