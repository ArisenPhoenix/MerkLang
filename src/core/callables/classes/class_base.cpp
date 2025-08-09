// class_base.cpp

#include "core/node/node.h"
#include "core/node/argument_node.h"
#include "core/node/node_structures.h"

#include "core/types.h"
#include "core/errors.h"
#include "utilities/helper_functions.h"
#include "utilities/debugger.h"
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
    (void)name;
    throw MerkError("Shouldn't be using this right now");
    auto methodOpts = classScope->getFunction(name);
    if (methodOpts.has_value()) {
        auto method = methodOpts.value().front();
    if (method) {return method->getCallable();}
    }
    
    throw FunctionNotFoundError(name);
}

// Update (or set) a member variable's value in the class scope.
void ClassBase::addMember(const String& name, UniquePtr<VarNode> value) {
    classScope->declareVariable(name, std::move(value));
    DEBUG_LOG(LogLevel::DEBUG, "Member added to ClassBase: ", name);
}

// Retrieve a member variable's value from the class scope.
Node ClassBase::getMember(const String& name) {
    return classScope->getVariable(name).getValueNode();
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

Node ClassBase::execute(ArgResultType args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);

    (void) args;
    (void) scope;
    DEBUG_FLOW_EXIT();
    return Node();
};


void ClassBase::setScope(SharedPtr<Scope> newScope) const {
    MARK_UNUSED_MULTI(newScope);
    // (void)newScope;
}

SharedPtr<CallableSignature> ClassBase::toCallableSignature() {
    DEBUG_FLOW(FlowLevel::HIGH);

    if (!getCapturedScope()) {throw MerkError("Captured Scope in ClassBase::toCallableSignature is null");}

    if (!getClassScope()) {throw MerkError("Class Scope is null in ClassBase::toCallableSignature");}

    // if (!getCapturedScope()->has(getClassScope())){throw MerkError("CallBase::toCallableSignature -> Captured Scope Does Not Own Class Scope");}


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
    // auto val = std::static_pointer_cast<Callable>(shared_from_this());
    auto classBase = std::static_pointer_cast<ClassBase>(asCallable(shared_from_this()));
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

        // if (name == "Dict") {
        //     throw MerkError("Dict Instance Made by name and Scope etc " + toString());
        // }
        
}


ClassInstance::ClassInstance(SharedPtr<ClassBase> cls, SharedPtr<Scope> captScope, SharedPtr<Scope> instScope)
    : Callable(cls->name, cls->parameters.clone(), CallableType::INSTANCE), capturedScope(captScope), instanceScope(instScope), accessor(cls->getAccessor()) {
    subType = cls->getSubType();
    instanceScope->owner = generateScopeOwner("ClassInstance", name);
    auto startingScopeCheck = getCapturedScope()->getParent();
    if (!startingScopeCheck) {throw MerkError("Could Not Get Defining Scope For Class Instance");}
    // if (name == "Dict") { throw MerkError("Dict Instance Made by ClassBase " + toString()); }
}

SharedPtr<Scope> ClassInstance::getCapturedScope() const {
    if (capturedScope) {return capturedScope;}

    throw MerkError("CapturedScope No Longer Exists in ClassInstance " + name);
}

void ClassInstance::setCapturedScope(SharedPtr<Scope> scope) {
    if (!scope) { throw MerkError("ClassInstance new Captured Scope is null"); }
    capturedScope = scope;
    
}

void ClassInstance::setScope(SharedPtr<Scope> newScope) const {
    MARK_UNUSED_MULTI(newScope);
    instanceScope = newScope;
}

String ClassInstance::toString() const {
    const auto& vars = getInstanceScope()->getContext().getVariables();
    if (!vars.size()) {
        if (value) {
            return value->toString();
        }
    }
    

    String out = name;
    if (vars.size() > 0) {
        out = "( ";
        for (const auto& [varName, var] : vars) {
            out += varName + ": " + var->varString() + ", ";
        }
    }

    out += ")";

    if (out.size() > 2) {
        out.replace(out.size() - 1, out.size(), " )");
    }
    return "<ClassInstance> " + out;
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



void ClassInstance::construct(const ArgResultType& args, SharedPtr<ClassInstance> self) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
   
    if (!getInstanceScope()->hasFunction("construct")) {throw MerkError("A construct method must be implemented in class: " + getName());}
    bool instanceMatch = self->getInstanceScope() == getInstanceScope();
    if (!instanceMatch) {
        if (self->getNativeData()) {
            auto oldSelf = self;
            auto nativeData = self->getNativeData();

            self = std::static_pointer_cast<ClassInstance>(self->clone());
            self->setNativeData(nativeData);
            // if (!std::holds_alternative<SharedPtr<Callable>>(self->getValue())) {
            //     throw MerkError("self in ClassInstance::construct is a " + nodeTypeToString(DynamicNode::getTypeFromValue(self)) + " and holds a " + nodeTypeToString(DynamicNode::getTypeFromValue(self->getValue()))); 
            // }
        }
    }


    auto methodOpt = getInstanceScope()->getFunction("construct", args);
    if (!methodOpt) {throw MerkError("Constructor for '" + getName() + "' does not match provided arguments.");}
    
    
    SharedPtr<CallableSignature> methodSig;
    if (methodOpt.has_value()) {
        methodSig = methodOpt.value();
    } else {
        throw FunctionNotFoundError("construct");
    }


    auto method = std::static_pointer_cast<Method>(methodSig->getCallable());
    if (!method) {throw MerkError("Class " + name + " is not valid");}
    SharedPtr<Scope> methodCallScope = self->getInstanceScope()->buildMethodCallScope(method, method->getName());
    auto params = parameters.clone();

    SharedPtr<ClassInstanceNode> instanceNode = makeShared<ClassInstanceNode>(self);

    SharedPtr<Scope> instanceScope = self->getInstanceScope();

    ClassMembers vars = {};
    if (method->getSubType() == CallableType::CALLABLE) {throw MerkError("Incorrect Callable subType in NativeMethod");}

    instanceScope->setClassMembers(vars);
    method->execute(args, methodCallScope, instanceNode);
    instanceScope->removeChildScope(methodCallScope);
    
    isConstructed = true;
    if (name == "Types") {
        // instanceScope->debugPrint();
        // instanceScope->printChildScopes();
        // throw MerkError("Constructed Types");
    }
    DEBUG_FLOW_EXIT();
}
    
Node ClassInstance::execute(const ArgResultType args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    (void)args;
    (void)scope;
    return Node("Null");
}







ClassSignature::ClassSignature(SharedPtr<ClassBase> classBaseData)
: CallableSignature(asCallable(classBaseData), CallableType::CLASS), accessor(classBaseData->getAccessor())
 {
    // if (callableTypeAsString(classBaseData->getSubType()) == "Unknown") { throw MerkError("SubType is Unknown for ClassBase to ClassSignature");}
    setSubType(classBaseData->getSubType());
    // DEBUG_LOG(LogLevel::TRACE, "ClassSignature::ClassSignature -> classBase:", "CallableType: ", callableTypeAsString(classBaseData->getCallableType()), "SubType: ", callableTypeAsString(classBaseData->getSubType()));
    if (getCallableType() != CallableType::CLASS) {throw MerkError("ClassSignature callableType is not CLASS");}
    if (callableTypeAsString(getSubType()) == "Unknown") {throw MerkError("ClassSignature subType is unknown at construction");}
    // throw MerkError("See Above");
    // if (getSubType() != CallableType::NATIVE) {throw MerkError("SubType is not Native");}
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    DEBUG_FLOW_EXIT();
}


ClassSignature::~ClassSignature() = default;

String ClassSignature::getAccessor() const { return accessor; }

SharedPtr<ClassBase> ClassSignature::getClassDef() const {return std::dynamic_pointer_cast<ClassBase>(getCallable());}

String ClassInstance::getAccessor() {return accessor;} 
SharedPtr<Scope> ClassInstance::getInstanceScope() {return instanceScope;}
SharedPtr<Scope> ClassInstance::getInstanceScope() const { return instanceScope; }
void ClassInstance::setInstanceScope(SharedPtr<Scope> scope) {instanceScope = scope;};







// Optional: override call() to auto-instantiate when the class is "called"
Node ClassSignature::call(const ArgResultType& args, SharedPtr<Scope> scope, SharedPtr<Scope> instanceScope) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    (void)args;

    if (!scope){throw MerkError("Scope passed is no longer valid");}
    if (!instanceScope) {throw MerkError("Class Scope passed is no longer valid");}

    SharedPtr<ClassBase> classBase = std::static_pointer_cast<ClassBase>(getCallable());
    if (!classBase) { throw MerkError("Classbase Created Unsuccessfully");}

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
    return Node(ClassInstanceNode(instance));
}


Node ClassInstance::call(String name, ArgResultType args) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    auto methodOpt = getInstanceScope()->getFunction(name, args);
    SharedPtr<CallableSignature> methodSig;
    if (methodOpt.has_value()) {
        methodSig = methodOpt.value();
    }
    auto method = methodSig->getCallable();

    SharedPtr<Scope> methodCallScope = getInstanceScope()->makeCallScope();
    auto instance = std::static_pointer_cast<ClassInstance>(shared_from_this());
    auto instanceNode = makeShared<ClassInstanceNode>(instance);

    Node val = method->execute(args, methodCallScope, instanceNode);
    throw MerkError("Cannot Call ClassInstance Yet");
    DEBUG_FLOW_EXIT();
    return val;
}



Node ClassInstance::getField(const String& fieldName, TokenType type) const {    // specific to what kind of member i.e var/method
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (fieldName == "var") {throw MerkError("fieldName is Var: ClassInstance::getField");}

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
    DEBUG_FLOW_EXIT();
    throw MerkError("Field or method '" + fieldName + "' not found in class instance. If a call was made that should take place in the ChainOperation");
}

Node ClassInstance::getField(const String& fieldName) const {                    // assumes a variable
    // if (fieldName == "x") { throw MerkError("Attempted to get var x");}
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (fieldName == "var") {throw MerkError("fieldName is Var: ClassInstance::getField");}


    DEBUG_FLOW_EXIT();
    return getInstanceScope()->getVariable(fieldName).getValueNode();
}                     




void ClassInstance::declareField(const String& fieldName, const Node& var) {             // probably only used in dynamic construction of a class
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (var.isValid()){ 
        UniquePtr<VarNode> newVar = makeUnique<VarNode>(var);
        instanceScope->declareVariable(fieldName, std::move(newVar));
    } else {
        throw MerkError("Variable not valid when declaring in ClassInstance::declareField");
    }

    DEBUG_FLOW_EXIT();
};


void ClassInstance::declareField(const String& fieldName, const VarNode& var) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!instanceScope) { throw MerkError("Cannot declare field: instanceScope is missing"); }
    UniquePtr<VarNode> newVar = makeUnique<VarNode>(var);
    instanceScope->declareVariable(fieldName, std::move(newVar));
    DEBUG_FLOW_EXIT();
}


void ClassInstance::updateField(const String& fieldName, Node val) const {                // most commonly used
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (fieldName == "var") {throw MerkError("fieldName is Var: ClassInstance::getField");}

    if (!instanceScope) { throw MerkError("Cannot declare field: instanceScope is missing"); }
    instanceScope->updateVariable(fieldName, val);
    // if (fieldName == "list") {
    //     auto& var = instanceScope->getVariable(fieldName);
    //     auto val = var.getValueNode();
    //     throw MerkError("LIST DATA ORIGINAL: " + val.toString() + "     LIST DATA CLONED " + val.toString());
    // }
    

    DEBUG_FLOW_EXIT();
}                 

void ClassInstance::setNativeData(SharedPtr<NativeNode> incoming) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    value = incoming;
    // if (!nativeData) {throw MerkError("Native Data Wasn't Set");}
    DEBUG_FLOW_EXIT();

}


// ClassInstance::ClassInstance(SharedPtr<ClassBase> cls, SharedPtr<Scope> captScope, SharedPtr<Scope> instScope)
//     : Callable(cls->name, cls->parameters.clone(), CallableType::INSTANCE), capturedScope(captScope), instanceScope(instScope), accessor(cls->getAccessor()) {
//     subType = cls->getSubType();
//     instanceScope->owner = generateScopeOwner("ClassInstance", name);
//     auto startingScopeCheck = getCapturedScope()->getParent();
//     if (!startingScopeCheck) {throw MerkError("Could Not Get Defining Scope For Class Instance");}
//     // if (name == "Dict") { throw MerkError("Dict Instance Made by ClassBase " + toString()); }
// }


SharedPtr<ClassInstance> ClassInstance::cloneInstance() const {
    auto clonedInstanceScope = getInstanceScope()->clone();
    auto clonedCapturedscope = clonedInstanceScope->getParent();
    
    auto clonedInstance = makeShared<ClassInstance>(name, clonedCapturedscope, clonedInstanceScope, parameters.clone(), accessor);

    const auto& vars = instanceScope->getContext().getVariables();

    if (name.empty()) { throw MerkError("ClassInstance::clone has no name"); }
    if (!value) {
        String repr = "";
        for (auto& var : vars) {
            repr += var.first + " " + var.second->toString() + ", ";
        }
        throw MerkError("Cloning Variables from instance, " + repr);
    }
    if (value) {
        clonedInstance->setNativeData(std::static_pointer_cast<DataStructure>(value->clone()));
        DEBUG_LOG(LogLevel::TRACE, highlight("++++++++++++++++++++++++++++++++++++++++++++++", Colors::bg_bright_green), "has value of " + value->toString());
        // throw MerkError("Cloned NativeData from " + value->toString()  + " to " + clonedInstance->getNativeData()->toString());
    }
    if (clonedInstance.get() == this || clonedInstanceScope == getInstanceScope()) {throw MerkError("Instances are still the same");}

    DEBUG_LOG(LogLevel::TRACE, highlight("ORIGINAL INSTACE DATA START: ==========================================================================", Colors::orange));
    // if (value) {debugLog(true, value->toString());}


    DEBUG_LOG(LogLevel::TRACE, highlight("ORIGINAL INSTACE DATA END: ==========================================================================", Colors::orange));



    DEBUG_LOG(LogLevel::TRACE, highlight("CLONED INSTACE DATA START: ==========================================================================", Colors::purple));
    clonedInstanceScope->debugPrint();
    // if (clonedInstance->value) {debugLog(true, clonedInstance->value->toString());}

    DEBUG_LOG(LogLevel::TRACE, highlight("CLONED INSTACE DATA END: ==========================================================================", Colors::purple));
    // throw MerkError("See Above");
    // else {throw MerkError("Was Unable to get Class Signature");}

    // throw MerkError("End Instance clone");
    
    if (vars.size() > 0 && value) { throw MerkError("cloned an instance holding both variables and native data");}


    // else throw MerkError("Cloned Instance from " + toString() + "    To " + clonedInstance->toString());
    // if (clonedInstanceScope->getContext().getVariables().size()) {
    //     clonedInstanceScope->getContext().debugPrint();
    //     throw MerkError("Instance Context Above");
    // } else {
    //     String val = (value ? " Holds NativeData" : " No NativeData");
    //     throw MerkError("No Vars in Context && " + val);
    // }
    return clonedInstance;
}

SharedPtr<NodeBase> ClassInstance::clone() const {
    return cloneInstance();
}

void ClassInstance::setValue(const VariantType& v) {
    if (std::holds_alternative<SharedPtr<NativeNode>>(v)) {
        auto ds = std::get<SharedPtr<NativeNode>>(v);
        value = ds;
    }
    throw MerkError("Cannot set type " + nodeTypeToString(DynamicNode::getTypeFromValue(v)) + " as native class data");
}

void ClassInstance::setValue(SharedPtr<NativeNode> v) {
    value = v;
}



SharedPtr<NativeNode> ClassInstance::getNativeData() {return value;}
VariantType ClassInstance::getValue() const { return value; }
NodeValueType ClassInstance::getType() const {return NodeValueType::ClassInstance; }
std::size_t ClassInstance::hash() const {
    // std::size_t h1 = std::hash<int>()(static_cast<int>(nativeData->getType()));
    std::size_t finalHash = 0;
    if (value) {
        finalHash += value->hash();
    }
    return finalHash;
}

ClassInstance::~ClassInstance() {
    DEBUG_LOG(LogLevel::DEBUG, "~ClassInstance() destructor triggered");
    getInstanceScope()->clear();
    getCapturedScope()->clear();
    value.reset();
}

ClassInstanceNode::ClassInstanceNode(SharedPtr<ClassInstance> callable) : CallableNode(callable, "ClassInstance") {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    // (void)callable;
    // data.type = NodeValueType::ClassInstance;
    // auto val = std::static_pointer_cast<ClassInstance>(callable);
    // setValue(val);
    // getFlags().type = NodeValueType::ClassInstance;
    // getFlags().fullType.setBaseType("ClassInstance");
    // getFlags().isInstance = true;

    // getFlags().name = callableType + "(" + callable->name + ")";
    // setValue(callable);
    setFlags(getFlags().merge({{"isInstance", "true"}, {"type", "ClassInstance"}, {"fullType", "ClassInstance"}, {"name", callable->getName()}}));
    
    // data.value = callable;
    // name = callable->getName();

    DEBUG_FLOW_EXIT();
    // throw MerkError("HIT ClassInstanceNode::ClassInstanceNode(SharedPtr<Callable> WITH META: " + getFlags().toString() + " DETERMINED TYPE: " + nodeTypeToString(DynamicNode::getTypeFromValue(callable)));    
}


ClassInstanceNode::ClassInstanceNode(SharedPtr<CallableNode> callableNode)
    : CallableNode(callableNode) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    // auto instance = std::get<SharedPtr<Callable>>(data.value);
    // auto instance = getInstance();
    // if (!instance) {throw MerkError("ClassInstanceNode: expected ClassInstance in CallableNode");}
    // setValue(instance);
    // data.value = instance; 
    // data.type = NodeValueType::ClassInstance;
    // name = callableNode->getCallable()->getName();
    setFlags(getFlags().merge({{"isInstance", "true"}, {"type", "ClassInstance"}, {"fullType", "ClassInstance"}}));

    DEBUG_FLOW_EXIT();

}


SharedPtr<Callable> ClassInstanceNode::getCallable() const {
    DEBUG_FLOW(FlowLevel::NONE);
    // auto val = std::static_pointer_cast<ClassInstance>(std::get<SharedPtr<ClassInstance>>(data.value));
    auto val = toCallable();
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


// VariantType ClassInstanceNode::getValue() const { 
//     return getInstance()->getValue();
// }

SharedPtr<ClassInstance> ClassInstanceNode::getInstance() const {
    DEBUG_FLOW(FlowLevel::NONE);
    
    // DEBUG_LOG(LogLevel::NONE, "Current Instance Type: ", this);
    // auto val = std::get<SharedPtr<ClassInstance>>(data.value); 
    // auto val = getValue();
    // std::get<ClassInstance>(val);
    // ClassInstance(val);
    // auto instance = std::static_pointer_cast<ClassInstance>(val);
    // ClassInstance(Node());
    auto call = getCallable();
    if (call->getType() == NodeValueType::Callable || call->getType() == NodeValueType::ClassInstance) {
        auto instance = std::static_pointer_cast<ClassInstance>(call);
        if (instance) {
            // auto type = DynamicNode::getTypeFromValue(getValue());
            return instance;
        }
        else {
            throw MerkError("There Was No Instance");
        }
        
    // if (std::holds_alternative<SharedPtr<Callable>>(getInner()) || (std::holds_alternative<int>(getValue()) && std::get<int>(getValue()) == 0)) {
    //     DEBUG_LOG(LogLevel::TRACE, "ABOUT TO GET CLASS INSTANCE FROM DETERMINED TYPE: ", nodeTypeToString(type));
    //     auto callable = std::get<SharedPtr<Callable>>(getValue());
    //     // DEBUG_LOG(LogLevel::TRACE, "CALLABLE DATA: ", callable->toString());
    //     auto instance = std::static_pointer_cast<ClassInstance>(callable);
    //     return instance;
    //     // throw MerkError("GOT CALLABLE");
    //     // return std::get<SharedPtr<Callable>>(getValue());
    // }
    }
    else {
        throw MerkError("The InstanceNode is holding " + call->toString() + call->flags.toString() + " With type of " + nodeTypeToString(call->getType()) + " And Full Type of " + call->flags.fullType.toString());
    }
    

    // else if (std::holds_alternative<SharedPtr<ClassInstance>>(getValue())) {
    //     auto instance = std::get<SharedPtr<ClassInstance>>(getValue());
    //     return instance;
    // }

    if (std::holds_alternative<int>(getValue())) {
        int val = std::get<int>(getValue());
        throw MerkError("The Instance Holds " + std::to_string(val) + " as its value") ;
    }
    


    throw MerkError("ClassInstanceNode does not hold a ClassInstance, but a " + nodeTypeToString(call->getType()));
    
    
    // auto instance = makeShared<ClassInstance>();
    // return instance;
}

ClassInstanceNode ClassInstanceNode::getInstanceNode() const {
    DEBUG_FLOW(FlowLevel::NONE);

    SharedPtr<ClassInstance> raw = getInstance();
    auto val = ClassInstanceNode(raw);
    DEBUG_FLOW_EXIT(); 
    return val;
}

String ClassInstanceNode::toString() const {
    return getInstance()->toString();
    // return "<ClassInstanceNode>" + getFlags().name;
    // return "<" + nodeType + ": " + getInstance()->toString() + ">";
}


void ClassInstanceNode::setValue(const VariantType& v) { getInstance()->setValue(v); }

void ClassInstanceNode::setValue(SharedPtr<NativeNode> v) { getInstance()->setValue(v); }

NodeValueType ClassInstanceNode::getType() {return NodeValueType::ClassInstance;}
SharedPtr<CallableNode> ClassInstanceNode::clone() const { return cloneInstance(); }
void ClassInstanceNode::clear() { getInstance()->clear(); } 

SharedPtr<ClassInstanceNode> ClassInstanceNode::cloneInstance() const {
    auto cloned = Node::clone();
    auto node = makeShared<ClassInstanceNode>(cloned.toInstance());
    return node;
    // auto clonedInstance = getInstance()->clone();
    // auto newInstanceNode = makeShared<ClassInstanceNode>(clonedInstance);
    // if (std::holds_alternative<SharedPtr<DataStructure>>(getValue())) {
    //     throw MerkError("ClassInstanceNode holds a data structure");
    // }

    // else if (std::holds_alternative<SharedPtr<ClassInstance>>(getValue())) {
    //     throw MerkError("ClassInstanceNode holds a ClassInstance");
    // }

    // else if (std::holds_alternative<SharedPtr<Callable>>(getValue())) {
    //     auto data = std::get<SharedPtr<Callable>>(getValue());
    //     auto inst = std::static_pointer_cast<ClassInstance>(data);
    //     if (inst->getNativeData()) {
    //         if (inst->getNativeData() == newInstanceNode->getInstance()->getNativeData()) {
    //             throw MerkError("The Native Data is the same");
    //         }
    //         if (!newInstanceNode->getInstance()->getNativeData()) {
    //             throw MerkError("NativeData doesn't match between instanceNode and cloned InstanceNode");
    //         }
    //         else if (inst == newInstanceNode->getInstance()) {
    //             throw MerkError("The Instances are the same");
    //         }
    //         // newInstanceNode->getInstance()->setNativeData(inst->getNativeData()->clone());
    //         // getInstance()->getInstanceScope()->clear();
            
    //         // newInstanceNode->clear();
    //         // clonedInstance.reset();
    //         // throw MerkError("Set the NativeData");
            
    //         // return newInstanceNode;
    //     } else {
    //         throw MerkError("Has Not Meta Data");
    //     }
    // }

    // else {
    //     auto val = getValue();
    //     DEBUG_LOG(LogLevel::TRACE, highlight("ClassInstanceNode holds: " + nodeTypeToString(DynamicNode::getTypeFromValue(val)), Colors::bg_red));
    //     // newInstanceNode->setFlags(getFlags());
    //     // throw MerkError();
    // }
    // // newInstanceNode->setValue(clonedInstance);
    // // newInstanceNode->getValue() = clonedInstance;
    
    
    // // newInstanceNode->setFlags(getFlags());
    // auto curFlags = getFlags();
    // newInstanceNode->setFlags(curFlags.merge({{"isInstance", "true"}, {"type", "ClassInstance"}, {"fullType", "ClassInstance"}}));

    // auto var = VarNode(newInstanceNode->getInstanceNode(), curFlags);
    // // node.setValue(newInstanceNode->getValue());

    // auto oldHeldData = getInstance()->getNativeData();
    // auto dataHeld = newInstanceNode->getInstance()->getNativeData();


    // throw MerkError("Cloned Instance of " + newInstanceNode->toString() + " Node Holding " + var.toString() + " Held Data of: " + dataHeld->toString() + " Old Held Data: " + oldHeldData->toString());
    // return newInstanceNode;
}