#include <iostream>

#include "core/node/Node.hpp"

#include "core/node/ArgumentNode.hpp"
// #include "core/types.h"
#include "core/TypesFWD.hpp"
#include "utilities/debugger.h"
#include "utilities/helper_functions.h"
#include "core/Scope.hpp"

#include "core/Evaluator.hpp"
#include "ast/Ast.hpp"

#include "ast/AstCallable.hpp"
#include "ast/AstChain.hpp"
#include "core/callables/classes/Method.hpp" 
#include "core/callables/classes/ClassBase.hpp"

#include "ast/AstClass.hpp"
#include "ast/Exceptions.hpp"
#include "ast/ast_validate.h"

ClassBody::ClassBody(UniquePtr<CodeBlock>&& block)
    : CallableBody(block->getScope()) {
    this->children = std::move(block->children);

    block.reset();
}
ClassBody::ClassBody(SharedPtr<Scope> scope): CallableBody(scope) {}

ClassCall::ClassCall(String name, UniquePtr<ArgumentType> arguments, SharedPtr<Scope> scope)
    : CallableCall(name, std::move(arguments), scope) {
    branch = "Classical";
}

ClassDef::ClassDef(String name, ParamList parameters, UniquePtr<ClassBody> body, String accessor, SharedPtr<Scope> scope)
    : CallableDef(name, std::move(parameters), std::move(body), CallableType::CLASS, scope), accessor(accessor) {}

void ClassDef::setClassAccessor(String accessorName){accessor = accessorName;}

String ClassDef::getClassAccessor() {return accessor;}
    
ParamList& ClassDef::getParameters() {return parameters;}

void ClassBody::setAccessor(String& classAccessor) {accessor = classAccessor;}

void ClassBody::setCapturedScope(SharedPtr<Scope> capturedScope) {classCapturedScope = capturedScope;}

void ClassBody::setClassScope(SharedPtr<Scope> newScope) {classScope = newScope;}


Accessor::Accessor(String accessor, SharedPtr<Scope> scope)
    : ASTStatement(scope), accessor(std::move(accessor)) {
}

void Accessor::setScope([[maybe_unused]] SharedPtr<Scope> scope) {(void)scope;}


Node ClassDef::evaluate(SharedPtr<Scope> defScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    
    if (!defScope) {throw MerkError("No Scope Was Found in ClassDef::evaluate()");}
    if (!body->getScope()){throw MerkError("scope not present in ClassDef::evaluate in body");}

    FreeVars freeVarNames = body->collectFreeVariables();
    if (freeVarNames.size() == 0) {DEBUG_LOG(LogLevel::TRACE, "There Are No Free Variables in classdef: ", name);}

    SharedPtr<Scope> classDefCapturedScope = defScope->buildClassDefScope(freeVarNames, name);
    SharedPtr<Scope> classScope = classDefCapturedScope->createChildScope();
    if (!classDefCapturedScope){throw MerkError("classScope was not created correctly on the ClassBase.");}

    if (!parameters.eraseByName(accessor)) {throw MerkError("No Accessor Was Provided In Class Constructor");}

    DEBUG_LOG(LogLevel::TRACE, highlight("Attempting captured setting on cls", Colors::yellow));

    SharedPtr<ClassBase> cls = makeShared<ClassBase>(name, accessor, classScope);
    cls->setClassScope(classScope);
    cls->setCapturedScope(classDefCapturedScope);
    cls->setParameters(parameters.clone());

    auto classBody = static_cast<ClassBody*>(getBody());
    classBody->setClassScope(classScope);

    classBody->setCapturedScope(cls->getCapturedScope());
    String bodyAccess = accessor;
    classBody->setAccessor(bodyAccess);

    DEBUG_LOG(LogLevel::TRACE, "ClassScope Below: ");    

    auto out = classBody->evaluate(cls->getClassScope(), instanceNode);

    if (!cls->getClassScope()->hasFunction("construct")) {
        cls->getClassScope()->debugPrint();
        cls->getClassScope()->printChildScopes();
        throw MerkError("Class '" + name + "' must implement a 'construct' method.");
    }
    
    classBody->setCapturedScope(nullptr);
    classBody->setClassScope(nullptr);
    // classBody->getScope()->owner = generateScopeOwner("ClassBody", name);

    defScope->registerClass(name, cls);
    
    if (!defScope->getClass(name)) {
        throw MerkError("Class " + name + " Was not registered");
    }

    // Vector<TypeId> methodIds;
    // auto methods = cls->getAllMethodSignatures();
    // for (auto method : methods) {
    //     Vector<TypeId> types;
    //     for (auto paramType : method->getParameterTypes() ) {
    //         types.emplace_back(TypeRegistry::primitive(paramType));
    //     }

    //     TypeNode typeNode {
    //         TypeKind::Invocable,
    //         NodeValueType::Method,
    //         method->getCallable()->getName(),
    //         types,
    //     };

    //     auto mid = classScope->localTypes.idOf(typeNode);
    //     methodIds.emplace_back(mid);
    // } 
    
    // Vector<TypeId> clsParams;
    // for (auto paramType : parameters.clone().getParameterTypes()) {
    //     clsParams.emplace_back(TypeRegistry::primitive(paramType));
    // }

    // TypeNode classType {
    //     TypeKind::Named,
    //     NodeValueType::Class,
    //     name,
    //     clsParams,
    //     "",
    //     methodIds
    // };

    // defScope->localTypes.idOf(classType);
    
    DEBUG_LOG(LogLevel::TRACE, "ClassDef::classScope created: ");
    auto classNode = ClassNode(cls);
    
    DEBUG_FLOW_EXIT();
    return classNode;
} 

Node ClassBody::evaluate(SharedPtr<Scope> classScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    // setScope(classScope);
    return Evaluator::evaluateClassBody(classCapturedScope, classScope, getScope(), accessor, getMutableChildren(), instanceNode);
}

Node ClassCall::evaluate(SharedPtr<Scope> callScope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!callScope) {throw MerkError("Initial Scope Failed in ClassCall::evaluate()");}
    // if (!getScope()) {throw MerkError("ClassCall::evaluate(): getScope() is null");}

    auto argValues = handleArgs(callScope, instanceNode);

    DEBUG_FLOW_EXIT();

    return Evaluator::evaluateClassCall(callScope, name, argValues, instanceNode);
}

Node Accessor::evaluate(SharedPtr<Scope> evalScope, SharedPtr<ClassInstanceNode> instanceNode) const {
    MARK_UNUSED_MULTI(evalScope);
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!instanceNode) {throw MerkError("The passed instanceNode is null");}
    
    if (!instanceNode->isInstance()) throw MerkError("No instance provided to Accessor, but a " + nodeTypeToString(DynamicNode::getTypeFromValue(instanceNode->getCallable())));


    ClassInstanceNode instance = instanceNode->toInstance(); 

    // throw MerkError("Evaluating Some other structure, the current Value of instanceNode is: " + nodeTypeToString(DynamicNode::getTypeFromValue(instanceNode)));

    
    DEBUG_FLOW_EXIT();
    return instance;
}


ClassDef::~ClassDef() {if (getBody()) { getBody().reset(); }}

ClassCall::~ClassCall() {
    if (getScope()) { getScope()->clear();}
    if (arguments) {
        arguments.reset();
    }
}

ClassBody::~ClassBody() {clear();}



