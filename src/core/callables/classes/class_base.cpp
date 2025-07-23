// class_base.cpp
#include "core/types.h"
#include "core/errors.h"
#include "utilities/helper_functions.h"
#include "utilities/debugger.h"
#include "core/node.h"
#include "ast/ast_class.h"
#include "ast/ast_chain.h"
#include "core/callables/classes/method.h"
#include "core/callables/classes/class_base.h"
#include "core/scope.h"
#include "ast/ast_validate.h"
#include <cassert>







// SharedPtr<Scope> ClassBase::getScope() {return scope;}
String ClassBase::getAccessor() {return accessor;}
String& ClassBase::getQualifiedAccessor() {return accessor;}
void ClassBase::setParameters(ParamList params) {parameters = params;}

ClassBase::ClassBase(String otherName, String otherAccessor, SharedPtr<Scope> templateScope)
    : Callable(otherName, ParamList(), CallableType::CLASS), accessor(otherAccessor), classScope(templateScope){
        setSubType(CallableType::CLASS);    
}

ClassBase::~ClassBase() {
    if (getClassScope()){getClassScope().reset();}
    if (getCapturedScope()){getCapturedScope().reset();}
}

// Add a method to the class by storing its signature in the class scope.
// Here it's assumed that setVariable will store a Node that wraps the method signature.
void ClassBase::addMethod(const String& name, SharedPtr<Method> method) {
    classScope->registerFunction(name, method->toCallableSignature());
    DEBUG_LOG(LogLevel::DEBUG, "Method added to ClassBase: ", name);
}

// Retrieve a method's signature from the class scope.
SharedPtr<Callable> ClassBase::getMethod(const String& name) {
    auto methods = classScope->getFunction(name);
    auto method = methods.front();
    if (method) {return method->getCallable();}
    throw FunctionNotFoundError(name);
}

// Update (or set) a member variable's value in the class scope.
void ClassBase::addMember(const String& name, UniquePtr<VarNode> value) {
    classScope->declareVariable(name, std::move(value));
    DEBUG_LOG(LogLevel::DEBUG, "Member added to ClassBase: ", name);
}

// Retrieve a member variable's value from the class scope.
Node ClassBase::getMember(const String& name) {
    return classScope->getVariable(name);
}

void ClassBase::setCapturedScope(SharedPtr<Scope> scope) {
    if (!scope){throw MerkError("New Scope provided to ClassBase is null");}
    capturedScope = scope;
    initialCapturedScope = scope;
}

void ClassBase::setClassScope(SharedPtr<Scope> scope) {
    if (!scope){throw MerkError("New ClassScope provided to ClassBase is null");}
    classScope = scope;
    capturedScope = scope;
    classScope->owner = generateScopeOwner("ClassBase", name);

}

SharedPtr<Scope> ClassBase::getCapturedScope() const {
    if (auto captured = capturedScope.lock()) {return captured;}

    if (initialCapturedScope) {return initialCapturedScope;}

    return nullptr;
}
SharedPtr<Scope> ClassBase::getClassScope() const {return classScope;}

String ClassBase::toString() const {return "Class(" + name + ") <Params>" + parameters.toShortString();}

Node ClassBase::execute(Vector<Node> args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);

    (void) args;
    (void) scope;
    DEBUG_FLOW_EXIT();
    return Node();
};


void ClassBase::setScope(SharedPtr<Scope> newScope) const {
    (void)newScope;
}

SharedPtr<CallableSignature> ClassBase::toCallableSignature() {
    DEBUG_FLOW(FlowLevel::HIGH);

    if (!getCapturedScope()) {throw MerkError("Captured Scope in ClassBase::toCallableSignature is null");}

    if (!getClassScope()) {throw MerkError("Class Scope is null in ClassBase::toCallableSignature");}

    if (!getCapturedScope()->has(getClassScope())){throw MerkError("CallBase::toCallableSignature -> Captured Scope Does Not Own Class Scope");}


    // Clone captured scope (the one with free variables)
    SharedPtr<Scope> clonedCapturedScope = getCapturedScope();

    // Clone parameters
    ParamList clonedParams;
    for (const auto& param : parameters) {clonedParams.addParameter(param.copy());}

    this->setParameters(clonedParams);
    // setCapturedScope
    if (!initialCapturedScope){
        if (getCapturedScope()){
            initialCapturedScope = getCapturedScope();
        }
    }

    getClassScope()->owner = generateScopeOwner("ClassBase", name);
    auto classBase = std::static_pointer_cast<ClassBase>(shared_from_this());
    SharedPtr<CallableSignature> classSig = makeShared<ClassSignature>(classBase);
    classSig->setParameters(parameters.clone());
    DEBUG_FLOW_EXIT();
    return classSig;
}
 

ClassInstance::ClassInstance(const String& name, SharedPtr<Scope> captScope, SharedPtr<Scope> instScope, ParamList params, const String& access)
    : Callable(name, params, CallableType::INSTANCE), capturedScope(captScope), instanceScope(instScope), accessor(access) {
        instanceScope->owner = generateScopeOwner("ClassInstance", name);
        auto startingScopeCheck = getCapturedScope()->getParent();
        if (!startingScopeCheck) {throw MerkError("Could Not Get Defining Scope For Class Instance");} 
}


ClassInstance::ClassInstance(SharedPtr<ClassBase> cls, SharedPtr<Scope> captScope, SharedPtr<Scope> instScope)
    : Callable(cls->name, cls->parameters.clone(), CallableType::INSTANCE), capturedScope(captScope), instanceScope(instScope), accessor(cls->getAccessor()) {
    subType = cls->getSubType();
    instanceScope->owner = generateScopeOwner("ClassInstance", name);
    auto startingScopeCheck = getCapturedScope()->getParent();
    if (!startingScopeCheck) {throw MerkError("Could Not Get Defining Scope For Class Instance");} 
        // classTemplate->getQualifiedName(), capturedScope, instanceScope, params, classTemplate->getQualifiedAccessor()
}

SharedPtr<Scope> ClassInstance::getCapturedScope() const {
    if (capturedScope) {return capturedScope;}

    throw MerkError("CapturedScope No Longer Exists in ClassInstance " + name);
}

void ClassInstance::setCapturedScope(SharedPtr<Scope> scope) {
    if (!scope) {
        throw MerkError("ClassInstance new Captured Scope is null");
    }
    capturedScope = scope;
    
}

void ClassInstance::setScope(SharedPtr<Scope> newScope) const {
    (void)newScope;
}

String ClassInstance::toString() const {
    if (getSubType() == CallableType::NATIVE) {
        if (nativeData) {
            return nativeData->toString();
        } else {
            throw MerkError("No NATIVE DATA AVAILABLE FOR: " + name);
        }
    }
    

    // if (name == "Array" && !nativeData) {
    //     throw MerkError("None");
    // }
    return "<ClassInstance>" + name;
}

SharedPtr<CallableSignature> ClassInstance::toCallableSignature() {throw MerkError("Instances are not directly callable unless '__call__' is defined.");}

ClassMembers ClassInstance::getInstanceVarsFromConstructor(SharedPtr<Method> method) {
    auto methodBody = method->getBody();
    auto ast = methodBody->getAllAst();
    auto instanceMembers = ASTUtils::collectMatching(ast, 
    [](const BaseAST* node) {
            return node->getAstType() == AstType::VariableDeclaration;
        }, false, false);
    

    ClassMembers vars;
    for (auto* member : instanceMembers) {
        auto decMember = static_cast<const VariableDeclaration*>(member);
        auto varName = decMember->getName();
        vars.emplace(varName, varName);
    }


    return vars;

}

SharedPtr<ClassInstanceNode> ClassInstance::getInstanceNode() {
    auto instance = std::static_pointer_cast<ClassInstance>(shared_from_this());
    return makeShared<ClassInstanceNode>(instance);
}



void ClassInstance::construct(const Vector<Node>& args, SharedPtr<ClassInstance> self) {
    DEBUG_FLOW(FlowLevel::HIGH);
    if (!getInstanceScope()->hasFunction("construct")) {throw MerkError("A construct method must be implemented in class: " + getName());}

    auto methodOpt = getInstanceScope()->getFunction("construct", args);
    if (!methodOpt) {throw MerkError("Constructor for '" + getName() + "' does not match provided arguments.");}
    DEBUG_LOG(LogLevel::PERMISSIVE, "sigCallType: ", callableTypeAsString(methodOpt->getCallableType()), "sigSubType: ", callableTypeAsString(methodOpt->getSubType()));
    // throw MerkError("Got sig Types");
    auto method = std::static_pointer_cast<Method>(methodOpt->getCallable());
    if (!method) {throw MerkError("Class " + name + " is not valid");}
    SharedPtr<Scope> methodCallScope = self->getInstanceScope()->buildMethodCallScope(method, method->getName());

    auto params = parameters.clone();

    SharedPtr<ClassInstanceNode> instanceNode = makeShared<ClassInstanceNode>(self);
    SharedPtr<Scope> instanceScope = self->getInstanceScope();
    
    DEBUG_LOG(LogLevel::PERMISSIVE, "ClassInstance::contruct -> methodCallScope");
    ClassMembers vars = {};
    DEBUG_LOG(LogLevel::PERMISSIVE, "callType: ", callableTypeAsString(method->getCallableType()), " subType: ", callableTypeAsString(method->getSubType()));
    if (method->getSubType() == CallableType::CALLABLE) {throw MerkError("Incorrect Callable subType in NativeMethod");}

    DEBUG_LOG(LogLevel::PERMISSIVE, "got instance vars");
    instanceScope->setClassMembers(vars);
    method->execute(args, methodCallScope, instanceNode);
    instanceScope->removeChildScope(methodCallScope);
    
    isConstructed = true;
    DEBUG_FLOW_EXIT();
}
    
Node ClassInstance::execute(const Vector<Node> args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    (void)args;
    (void)scope;
    return Node("Null");
}







ClassSignature::ClassSignature(SharedPtr<ClassBase> classBaseData)
: CallableSignature(classBaseData, CallableType::CLASS), accessor(classBaseData->getAccessor())
 {
    // if (callableTypeAsString(classBaseData->getSubType()) == "Unknown") { throw MerkError("SubType is Unknown for ClassBase to ClassSignature");}
    setSubType(classBaseData->getSubType());
    DEBUG_LOG(LogLevel::PERMISSIVE, "ClassSignature::ClassSignature -> classBase:", "CallableType: ", callableTypeAsString(classBaseData->getCallableType()), "SubType: ", callableTypeAsString(classBaseData->getSubType()));
    DEBUG_LOG(LogLevel::PERMISSIVE, "ClassSignature::ClassSignature -> classSig:", "CallableType: ", callableTypeAsString(getCallableType()), "SubType: ", callableTypeAsString(getSubType()));
    // throw MerkError("See Above");
    // if (getSubType() != CallableType::NATIVE) {throw MerkError("SubType is not Native");}
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    DEBUG_FLOW_EXIT();
}


ClassSignature::~ClassSignature() = default;

String ClassSignature::getAccessor() const { return accessor; }

SharedPtr<ClassBase> ClassSignature::getClassDef() const {return std::dynamic_pointer_cast<ClassBase>(getCallable());}

SharedPtr<Scope> ClassInstance::getInstanceScope() {return instanceScope;}
void ClassInstance::setInstanceScope(SharedPtr<Scope> scope) {instanceScope = scope;};







// Optional: override call() to auto-instantiate when the class is "called"
Node ClassSignature::call(const Vector<Node>& args, SharedPtr<Scope> scope, SharedPtr<Scope> instanceScope) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    (void)args;

    if (!scope){throw MerkError("Scope passed is no longer valid");}
    if (!instanceScope) {throw MerkError("Class Scope passed is no longer valid");}

    SharedPtr<ClassBase> classBase = std::static_pointer_cast<ClassBase>(getCallable());
    if (!classBase) {
        throw MerkError("Classbase Created Unsuccessfully");
    }

    auto captured = instanceScope->getParent();
    captured->owner = generateScopeOwner("InstanceCaptured", classBase->getName());
    if (!captured){
        throw MerkError("Captured Scope Does Not Exist When Instantiating class: " + classBase->getName());
    }
    if (!captured->has(instanceScope)){
        captured->printChildScopes();
        instanceScope->printChildScopes();
        throw MerkError("Instance Scope does not live in captured Scope");
    } 
    else {DEBUG_LOG(LogLevel::DEBUG, "Instance Scope Lives in Captured Scope");}
    auto params = classBase->getParameters().clone();


    instanceScope->owner = generateScopeOwner("ClassInstance", classBase->getName());
    SharedPtr<ClassInstance> instance = makeShared<ClassInstance>(classBase->getQualifiedName(), captured, instanceScope, params, classBase->getQualifiedAccessor());

    // instance->construct(args, ins);
    DEBUG_FLOW_EXIT();
    return ClassInstanceNode(instance);
}


Node ClassInstance::call(String name, Vector<Node> args) {
    DEBUG_FLOW(FlowLevel::NONE);
    auto methodSig = getInstanceScope()->getFunction(name, args);
    auto method = methodSig->getCallable();

    SharedPtr<Scope> methodCallScope = getInstanceScope()->makeCallScope();
    auto instance = std::static_pointer_cast<ClassInstance>(shared_from_this());
    auto instanceNode = makeShared<ClassInstanceNode>(instance);

    Node val = method->execute(args, methodCallScope, instanceNode);

    DEBUG_FLOW_EXIT();
    return val;
}



Node ClassInstance::getField(const String& fieldName, TokenType type) const {    // specific to what kind of member i.e var/method
    DEBUG_FLOW(FlowLevel::NONE);

    if (!isConstructed) {
        throw MerkError("Attempted to access field '" + fieldName + "' before construct() completed.");

    }
    switch (type) {
        case TokenType::Variable:
            DEBUG_FLOW_EXIT();
            return getField(fieldName);
            break;

        default:
            throw MerkError("Unsupported field type for '" + fieldName + "'");
    }


    // return Node();
    throw MerkError("Field or method '" + fieldName + "' not found in class instance. If a call was made that should take place in the ChainOperation");
}

Node ClassInstance::getField(const String& fieldName) const {                    // assumes a variable
    DEBUG_FLOW(FlowLevel::NONE);


    DEBUG_FLOW_EXIT();
    return getInstanceScope()->getVariable(fieldName);
}                     




void ClassInstance::declareField(const String& fieldName, const Node& var) {             // probably only used in dynamic construction of a class
    if (var.isValid()){ 
        UniquePtr<VarNode> newVar = makeUnique<VarNode>(var);
        instanceScope->declareVariable(fieldName, std::move(newVar));
    }
};


void ClassInstance::declareField(const String& fieldName, const VarNode& var) {
    if (!instanceScope) { throw MerkError("Cannot declare field: instanceScope is missing"); }
    UniquePtr<VarNode> newVar = makeUnique<VarNode>(var);
    instanceScope->declareVariable(fieldName, std::move(newVar));
}


void ClassInstance::updateField(const String& fieldName, Node val) const {                // most commonly used
    if (!instanceScope) { throw MerkError("Cannot declare field: instanceScope is missing"); }
    instanceScope->updateVariable(fieldName, val);
}                 

void ClassInstance::setNativeData(SharedPtr<DataStructure> incoming) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    nativeData = incoming;
    // if (!nativeData) {throw MerkError("Native Data Wasn't Set");}
    DEBUG_FLOW_EXIT();

}
SharedPtr<DataStructure> ClassInstance::getNativeData() {return nativeData;}


ClassInstance::~ClassInstance() {
    DEBUG_LOG(LogLevel::DEBUG, "~ClassInstance() destructor triggered");
    getInstanceScope()->clear();
    getCapturedScope()->clear();
    nativeData.reset();
}

ClassInstanceNode::ClassInstanceNode(SharedPtr<ClassInstance> callable) : CallableNode(callable, "ClassInstance") {
    data.type = NodeValueType::ClassInstance;
    data.value = callable;
    name = callable->getName();
    
}



ClassInstanceNode::ClassInstanceNode(SharedPtr<CallableNode> callableNode)
    : CallableNode(callableNode) {
    DEBUG_FLOW(FlowLevel::NONE);
    // auto instance = std::get<SharedPtr<Callable>>(data.value);
    auto instance = getInstance();
    if (!instance) {throw MerkError("ClassInstanceNode: expected ClassInstance in CallableNode");}

    data.value = instance; 
    data.type = NodeValueType::ClassInstance;
    name = callableNode->getCallable()->getName();

}


SharedPtr<Callable> ClassInstanceNode::getCallable() const {
    DEBUG_FLOW(FlowLevel::NONE);
    // auto val = std::static_pointer_cast<ClassInstance>(std::get<SharedPtr<ClassInstance>>(data.value));
    auto val = getInstance();
    DEBUG_FLOW_EXIT();
    return val;
}

SharedPtr<Scope> ClassInstanceNode::getScope() {
    DEBUG_FLOW(FlowLevel::NONE);
    auto instance = getInstance();
    DEBUG_FLOW_EXIT();
    return instance->getInstanceScope();
}

SharedPtr<Scope> ClassInstanceNode::getInstanceScope() {
    return getScope();
}

SharedPtr<ClassInstance> ClassInstanceNode::getInstance() const {
    DEBUG_FLOW(FlowLevel::NONE);
    
    // DEBUG_LOG(LogLevel::NONE, "Current Instance Type: ", this);
    auto val = std::get<SharedPtr<ClassInstance>>(data.value); 
    auto instance = std::static_pointer_cast<ClassInstance>(val);
    return instance;
}

ClassInstanceNode ClassInstanceNode::getInstanceNode() const {
    DEBUG_FLOW(FlowLevel::NONE);

    SharedPtr<ClassInstance> raw = getInstance();
    auto val = ClassInstanceNode(raw);
    DEBUG_FLOW_EXIT(); 
    return val;
}

String ClassInstanceNode::toString() const {
    return "<" + nodeType + ": " + getInstance()->toString() + ">";
}

