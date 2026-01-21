#include "core/node/Node.hpp"
#include "core/node/ArgumentNode.hpp"
#include "core/node/NodeStructures.hpp"
#include "core/TypesFWD.hpp"
#include "core/errors.h"
#include "utilities/helper_functions.h"
#include "utilities/debugger.h"
#include "ast/AstClass.hpp"
#include "ast/AstChain.hpp"
#include "core/callables/classes/Method.hpp"
#include "core/callables/classes/ClassBase.hpp"
#include "core/Scope.hpp"
#include "ast/ast_validate.h"
#include <cassert>
#include "core/evaluators/Evaluator.hpp"


ClassNode::ClassNode(SharedPtr<ClassBase> callable) : CallableNode(callable, "Class") {
    getFlags().type = NodeValueType::Class;
}

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


void ClassBase::addMethod(const String& name, SharedPtr<Method> method) {
    classScope->registerFunction(name, method->toCallableSignature());
    DEBUG_LOG(LogLevel::DEBUG, "Method added to ClassBase: ", name);
}

SharedPtr<Callable> ClassBase::getMethod(const String& name) {
    MARK_UNUSED_MULTI(name);
    throw MerkError("Shouldn't be using this right now");
    auto methodOpts = classScope->getFunction(name);
    if (methodOpts.has_value()) {
        auto method = methodOpts.value().front();
        if (method) { 
            return method->getCallable();
        } 
    }
    throw FunctionNotFoundError(name);
}

Vector<SharedPtr<CallableSignature>> ClassBase::getAllMethodSignatures() {
    Vector<SharedPtr<CallableSignature>> methodSigs;
    for (auto& [methodName, methodSig] : classScope->localFunctions.getFunctions()) {
        for (const auto& method : methodSig) {
            methodSigs.emplace_back(method);
        }
    }
    return methodSigs;
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
    if (!scope) { throw MerkError("New Scope provided to ClassBase is null"); }
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

Node ClassBase::execute(ArgumentList args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    MARK_UNUSED_MULTI(args, scope);
    DEBUG_FLOW_EXIT();
    return Node();
};


void ClassBase::setScope(SharedPtr<Scope> newScope) const {
    MARK_UNUSED_MULTI(newScope);
}

SharedPtr<CallableSignature> ClassBase::toCallableSignature() {
    DEBUG_FLOW(FlowLevel::HIGH);

    if (!getCapturedScope()) {throw MerkError("Captured Scope in ClassBase::toCallableSignature is null");}

    if (!getClassScope()) {throw MerkError("Class Scope is null in ClassBase::toCallableSignature");}

    SharedPtr<Scope> clonedCapturedScope = getCapturedScope();

    // Clone parameters
    ParamList clonedParams;
    for (const auto& param : parameters) {clonedParams.addParameter(param.copy());}

    this->setParameters(clonedParams);

    if (!initialCapturedScope){
        if (getCapturedScope()){
            initialCapturedScope = getCapturedScope();
        }
    }

    getClassScope()->owner = generateScopeOwner("ClassBase", name);
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
}


ClassInstance::ClassInstance(SharedPtr<ClassBase> cls, SharedPtr<Scope> captScope, SharedPtr<Scope> instScope)
    : Callable(cls->name, cls->parameters.clone(), CallableType::INSTANCE), capturedScope(captScope), instanceScope(instScope), accessor(cls->getAccessor()) {
    subType = cls->getSubType();
    instanceScope->owner = generateScopeOwner("ClassInstance", name);
    auto startingScopeCheck = getCapturedScope()->getParent();
    if (!startingScopeCheck) {throw MerkError("Could Not Get Defining Scope For Class Instance");}
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
    if (vars.empty()) {
        if (value) return value->toString();
        return "<ClassInstance> ( )";
    }

    auto& cfg = debugStringifyConfig();
    String out = "( ";
    for (const auto& [varName, var] : vars) {
        String v = var ? var->varString() : "<null>";
        cfg.handle(v);

        out += varName + ": " + v + ", ";
    }

    // Close nicely
    if (out.size() >= 2 && out.back() == ' ' && out[out.size() - 2] == ',') {
        out.erase(out.size() - 2); // drop trailing ", "
        out += " )";
    } else if (out.size() >= 2 && out.substr(out.size() - 2) == ", ") {
        out.erase(out.size() - 2);
        out += " )";
    } else {
        out += ")";
    }

    return "<ClassInstance> " + name + " " + out;
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





void ClassInstance::construct(const ArgumentList& args, SharedPtr<ClassInstance> self) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
   
    if (!getInstanceScope()->hasFunction("construct")) {throw MerkError("A construct method must be implemented in class: " + getName());}
    bool instanceMatch = self->getInstanceScope() == getInstanceScope();
    if (!instanceMatch) {
        if (self->getNativeData()) {
            auto oldSelf = self;
            auto nativeData = self->getNativeData();

            self = std::static_pointer_cast<ClassInstance>(self->clone());
            self->setNativeData(nativeData);
        }
    }

    auto methodOpt = getInstanceScope()->getFunction("construct", args);
    if (!methodOpt) {throw MerkError("Constructor for '" + getName() + "' does not match provided arguments.");}
    
    SharedPtr<CallableSignature> methodSig;
    if (methodOpt.has_value()) { methodSig = methodOpt.value(); } 
    else { throw FunctionNotFoundError("construct"); }


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
    DEBUG_FLOW_EXIT();
}
    
Node ClassInstance::execute(const ArgumentList args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    MARK_UNUSED_MULTI(args, scope);
    return Node("Null");
}







ClassSignature::ClassSignature(SharedPtr<ClassBase> classBaseData)
: CallableSignature(asCallable(classBaseData), CallableType::CLASS), accessor(classBaseData->getAccessor())
 {
    setSubType(classBaseData->getSubType());
    if (getCallableType() != CallableType::CLASS) {throw MerkError("ClassSignature callableType is not CLASS");}
    if (callableTypeAsString(getSubType()) == "Unknown") {throw MerkError("ClassSignature subType is unknown at construction");}
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
Node ClassSignature::call(const ArgumentList& args, SharedPtr<Scope> scope, SharedPtr<Scope> instanceScope) const {
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
    else {DEBUG_LOG(LogLevel::DEBUG, "Instance Scope Lives in Captured Scope");}
    auto params = classBase->getParameters().clone();


    instanceScope->owner = generateScopeOwner("ClassInstance", classBase->getName());
    SharedPtr<ClassInstance> instance = makeShared<ClassInstance>(classBase->getQualifiedName(), captured, instanceScope, params, classBase->getQualifiedAccessor());

    DEBUG_FLOW_EXIT();
    return Node(ClassInstanceNode(instance));
}


Node ClassInstance::call(String name, ArgumentList args) {
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



Node ClassInstance::getField(const String& fieldName, TokenType type) const {   
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


    DEBUG_FLOW_EXIT();
    throw MerkError("Field or method '" + fieldName + "' not found in class instance. If a call was made that should take place in the ChainOperation");
}

Node ClassInstance::getField(const String& fieldName) const { 
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (fieldName == "var") {throw MerkError("fieldName is Var: ClassInstance::getField");}


    DEBUG_FLOW_EXIT();
    return getInstanceScope()->getVariable(fieldName).getValueNode();
}                     

void ClassInstance::declareField(const String& fieldName, DataTypeFlags varMeta, const ASTStatement *valueNode) {
    MARK_UNUSED_MULTI(fieldName, varMeta, valueNode);
    // String& name, const ASTStatement* valueNode, DataTypeFlags varMeta, SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode
    // String varName = fieldName;
    // Evaluator::evaluateVariableDeclaration(varName, valueNode, varMeta, getInstanceScope(), getInstanceNode());

}

void ClassInstance::declareField(const String& fieldName, const Node& var) {
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


void ClassInstance::updateField(const String& fieldName, Node val) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (fieldName == "var") {throw MerkError("fieldName is Var: ClassInstance::getField");}

    if (!instanceScope) { throw MerkError("Cannot declare field: instanceScope is missing"); }
    instanceScope->updateVariable(fieldName, val);
    DEBUG_FLOW_EXIT();
}                 

void ClassInstance::setNativeData(SharedPtr<NativeNode> incoming) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    value = incoming;
    DEBUG_FLOW_EXIT();

}

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
    }
    if (clonedInstance.get() == this || clonedInstanceScope == getInstanceScope()) {throw MerkError("Instances are still the same");}

    DEBUG_LOG(LogLevel::TRACE, highlight("ORIGINAL INSTACE DATA START: ==========================================================================", Colors::orange));

    DEBUG_LOG(LogLevel::TRACE, highlight("ORIGINAL INSTACE DATA END: ==========================================================================", Colors::orange));



    DEBUG_LOG(LogLevel::TRACE, highlight("CLONED INSTACE DATA START: ==========================================================================", Colors::purple));

    DEBUG_LOG(LogLevel::TRACE, highlight("CLONED INSTACE DATA END: ==========================================================================", Colors::purple));
    
    if (vars.size() > 0 && value) { throw MerkError("cloned an instance holding both variables and native data");}

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
    throw MerkError("Cannot set 'some' type as native class data");
}

void ClassInstance::setValue(SharedPtr<NativeNode> v) {
    value = v;
}



SharedPtr<NativeNode> ClassInstance::getNativeData() {return value;}
VariantType ClassInstance::getValue() const { return value; }
NodeValueType ClassInstance::getType() const {return NodeValueType::ClassInstance; }
std::size_t ClassInstance::hash() const {
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

    const String cls = callable ? callable->getName() : "<?>";

    // concrete identity
    setFlags(getFlags().merge({
        {"isCallable", "true"},               
        {"isInstance", "true"},
        {"type", "ClassInstance"},  
        {"fullType", cls},       
        {"name", "ClassInstance(" + cls + ")"}
    }));

    getFlags().inferredSig = getScope()->localTypes.classType(cls);

    DEBUG_FLOW_EXIT();
    

}


ClassInstanceNode::ClassInstanceNode(SharedPtr<CallableNode> callableNode)
    : CallableNode(callableNode) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    auto call = getCallable();
    const String cls = call ? call->getName() : "<?>";

    auto f = getFlags();
    f.isCallable  = true;
    f.isInstance  = true;
    f.type        = NodeValueType::ClassInstance;
    f.fullType.setBaseType(cls);
    f.inferredSig = getFlags().inferredSig = getScope()->localTypes.classType(cls);

    f.name        = "ClassInstance(" + cls + ")";

    setFlags(f);

    DEBUG_FLOW_EXIT();

}


SharedPtr<Callable> ClassInstanceNode::getCallable() const {
    DEBUG_FLOW(FlowLevel::NONE);
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

SharedPtr<ClassInstance> ClassInstanceNode::getInstance() const {
    DEBUG_FLOW(FlowLevel::NONE);
    auto call = getCallable();
    if (call->getType() == NodeValueType::Callable || call->getType() == NodeValueType::ClassInstance) {
        auto instance = std::static_pointer_cast<ClassInstance>(call);
        if (instance) {
            return instance;
        }
        else {
            throw MerkError("There Was No Instance");
        }
        
    }
    else {
        throw MerkError("The InstanceNode is holding " + call->toString() + call->flags.toString() + " With type of " + nodeTypeToString(call->getType()) + " And Full Type of " + call->flags.fullType.toString());
    }

    if (std::holds_alternative<int>(getValue())) {
        int val = std::get<int>(getValue());
        throw MerkError("The Instance Holds " + std::to_string(val) + " as its value") ;
    }



    throw MerkError("ClassInstanceNode does not hold a ClassInstance, but a " + nodeTypeToString(call->getType()));
}

ClassInstanceNode ClassInstanceNode::getInstanceNode() const {
    DEBUG_FLOW(FlowLevel::NONE);

    SharedPtr<ClassInstance> raw = getInstance();
    auto val = ClassInstanceNode(raw);
    DEBUG_FLOW_EXIT(); 
    return val;
}

String ClassInstanceNode::toString() const { return getInstance()->toString(); }


void ClassInstanceNode::setValue(const VariantType& v) { getInstance()->setValue(v); }

void ClassInstanceNode::setValue(SharedPtr<NativeNode> v) { getInstance()->setValue(v); }

NodeValueType ClassInstanceNode::getType() {return NodeValueType::ClassInstance;}
SharedPtr<CallableNode> ClassInstanceNode::clone() const { return cloneInstance(); }
void ClassInstanceNode::clear() { getInstance()->clear(); } 

SharedPtr<ClassInstanceNode> ClassInstanceNode::cloneInstance() const {
    auto copy = std::make_shared<ClassInstanceNode>(*this);
    copy->getFlags() = this->getFlags();
    return copy;
}