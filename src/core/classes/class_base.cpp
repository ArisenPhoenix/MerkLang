// class_base.cpp
#include "core/types.h"
#include "core/errors.h"
#include "utilities/helper_functions.h"
#include "utilities/debugger.h"
#include "core/node.h"
#include "ast/ast_class.h"
#include "ast/ast_chain.h"
#include "core/classes/method.h"
#include "core/classes/class_base.h"
#include "core/scope.h"
#include "ast/ast_validate.h"
#include <cassert>







// SharedPtr<Scope> ClassBase::getScope() {return scope;}
String ClassBase::getAccessor() {return accessor;}
String& ClassBase::getQualifiedAccessor() {return accessor;}
void ClassBase::setParameters(ParamList params) {parameters = params;}

ClassBase::ClassBase(String otherName, String otherAccessor, SharedPtr<Scope> templateScope)
    : Callable(otherName, ParamList(), CallableType::CLASS), accessor(otherAccessor), classScope(templateScope){    
}

ClassBase::~ClassBase() {
    // DEBUG_LOG(LogLevel::ERROR, "Destorying ClassBase");
    if (getClassScope()){
        // DEBUG_LOG(LogLevel::ERROR, highlight("Still Holding Onto classScope", Colors::yellow));
        getClassScope().reset();
    }
    if (getCapturedScope()){
        // DEBUG_LOG(LogLevel::ERROR, highlight("Still Holding Onto capturedScope", Colors::yellow));
        getCapturedScope().reset();

    }
}
// Add a method to the class by storing its signature in the class scope.
// Here we assume that setVariable will store a Node that wraps the method signature.
void ClassBase::addMethod(const String& name, SharedPtr<Method> method) {
    // Wrap the method signature into a Node.
    classScope->registerFunction(name, method->toCallableSignature());
    DEBUG_LOG(LogLevel::DEBUG, "Method added to ClassBase: ", name);
}

// Retrieve a method's signature from the class scope.
SharedPtr<Callable> ClassBase::getMethod(const String& name) {
    auto methods = classScope->getFunction(name);
    auto method = methods.front();
    if (method){
        return method->getCallable();
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
    return classScope->getVariable(name);
}

void ClassBase::setCapturedScope(SharedPtr<Scope> scope) {
    if (!scope){
        throw MerkError("New Scope provided to ClassBase is null");
    }
    capturedScope = scope;
    initialCapturedScope = scope;
    // getCapturedScope()->owner = "ClassBaseCaptured(" + name + ")";
}

void ClassBase::setClassScope(SharedPtr<Scope> scope) {
    if (!scope){
        throw MerkError("New ClassScope provided to ClassBase is null");
    }
    classScope = scope;
    // classScope->owner = generateScopeOwner("ClassBase", name);

}

SharedPtr<Scope> ClassBase::getCapturedScope() const {
    

    // else if (capturedScope.expired()){
    //     return nullptr;
    // }

    if (auto captured = capturedScope.lock()) {
        return captured;
    }
    if (initialCapturedScope) {
        return initialCapturedScope;
    }
    return nullptr;
}
SharedPtr<Scope> ClassBase::getClassScope() const {return classScope;}

String ClassBase::toString() const {
    return "ClassBase()";
};

Node ClassBase::execute(Vector<Node> args, SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);

    (void) args;
    (void) scope;
    DEBUG_FLOW_EXIT();
    return Node();
};


void ClassBase::setScope(SharedPtr<Scope> newScope) const {
    (void)newScope;
    // scope = newScope;
    // if (!newScope){
    //     throw MerkError("New Scope provided to ClassBase is null");
    // }
    // scope->owner = "ClassBase(" + name + ")";
    // getBody()->setScope(newScope);

}


// void ClassBase::setBody(UniquePtr<ClassBody> updatedBody) {body = std::move(updatedBody);}
// UniquePtr<ClassBody>& ClassBase::getBody() {return body;}
// ClassBody* ClassBase::getBody() const {return body.get();};

SharedPtr<CallableSignature> ClassBase::toCallableSignature() {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);

    // if (!body) {
    //     throw MerkError("The Body in ClassBase::toCallableSignature() is null");
    // }
    // if (!body->getScope()){
    //     throw MerkError("The Body in ClassBase::toCallableSignature() has a null scope");
    // }

    if (!getCapturedScope()) {
        throw MerkError("Captured Scope in ClassBase::toCallableSignature is null");
    }

    if (!getClassScope()) {
        throw MerkError("Class Scope is null in ClassBase::toCallableSignature");
    }

    // Clone the body
    // auto clonedBodyBase = body->clone();
    // auto clonedClassBody = static_unique_ptr_cast<ClassBody>(std::move(clonedBodyBase));

    // Clone captured scope (the one with free variables)
    SharedPtr<Scope> clonedCapturedScope = getCapturedScope();
    // clonedCapturedScope->owner = generateScopeOwner("ClassBaseCaptured", name);
    // clonedClassBody->setScope(clonedCapturedScope);

    // Create ClassBase using the cloned captured scope as parent
    // SharedPtr<ClassBase> classBase = makeShared<ClassBase>(
    //     getName(),
    //     accessor,
    //     // std::move(clonedClassBody),
    //     clonedCapturedScope // parentScope
    // );



    // Clone parameters
    ParamList clonedParams;
    for (const auto& param : parameters) {
        clonedParams.addParameter(param.copy());
    }

    this->setParameters(clonedParams);
    // setCapturedScope
    if (!initialCapturedScope){
        if (getCapturedScope()){
            initialCapturedScope = getCapturedScope();
        }
    }

    // Clone the classScope (member scope)
    // SharedPtr<Scope> clonedClassScope = ;
    getClassScope()->owner = "Class(" + name + ")";

    // Attach classScope to clonedCapturedScope
    // clonedCapturedScope->appendChildScope(getClassScope(), "ClassBase::toCallableSignature");

    // Set both scopes on class
    // classBase->setCapturedScope(clonedCapturedScope);

    // classBase->setClassScope(getClassScope());


    // DEBUG_LOG(LogLevel::ERROR, "ClassBase::toCallableSignature()", "Scopes cloned and linked");
    // ClassSignature()
    // Wrap into ClassSignature
    // ClassSignature(shared_from_this());
    auto classBase = std::static_pointer_cast<ClassBase>(shared_from_this());
    // auto sig = makeShared<ClassSignature>(classBase);
    // SharedPtr<CallableSignature> classSig = makeShared<ClassSignature>(shared_from_this());
    SharedPtr<CallableSignature> classSig = makeShared<ClassSignature>(classBase);
    DEBUG_FLOW_EXIT();
    return classSig;
}


ClassInstance::ClassInstance(String& name, SharedPtr<Scope> capturedScope, SharedPtr<Scope> instanceScope, ParamList params, String& accessor)
    : Callable(name, params, CallableType::INSTANCE), capturedScope(capturedScope), instanceScope(instanceScope), accessor(accessor) {
        auto startingScopeCheck = getCapturedScope()->getParent();
        if (!startingScopeCheck) {
            throw MerkError("Could Not Get Defining Scope For Class Instance");
        }
}


SharedPtr<Scope> ClassInstance::getCapturedScope() const {
    // return capturedScope;
    // auto scope = capturedScope.lock();
    // if (!scope) {
    //     if (!getInstanceScope()){
    //         throw MerkError("While Retriveing capturedScope from ClassInstance " + name + ", InstanceScope Was ALSO No longer available");
    //     } else {
    //         auto parent = getInstanceScope()->getParent();
    //         if (parent){
    //             return parent;
    //         } else {
    //             throw MerkError("While Retrieving capturedScope from ClassInstance " + name + ", InstanceScope's parent was no longer available");
    //         }
    //     }
    //     throw MerkError("ClassInstance::getCapturedScope: scope is null");
    // }
    // // if (auto scope = capturedScope.lock()) {
    // return scope;
    // }

    if (capturedScope) {
        return capturedScope;
    }

    throw MerkError("CapturedScope No Longer Exists in ClassInstance " + name);
}

void ClassInstance::setCapturedScope(SharedPtr<Scope> scope) {
    if (!scope) {
        throw MerkError("ClassInstance new Captured Scope is null");
    }
    capturedScope = scope;
    
    // capturedScope->owner = "ClassInstanceScope(" + name + ")";
}

void ClassInstance::setScope(SharedPtr<Scope> newScope) const {
    // startingScope = newScope;
    (void)newScope;
    // startingScope = newScope;
}

String ClassInstance::toString() const {
    return "<Instance of " + getName() + ">";
}

SharedPtr<CallableSignature> ClassInstance::toCallableSignature() {
    // stub for later implementation
    throw MerkError("Instances are not directly callable unless '__call__' is defined.");
}

void ClassInstance::construct(const Vector<Node>& args) {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    // if (!getInstanceScope()->hasFunction("construct")) {
    //     getInstanceScope()->debugPrint();
    //     getInstanceScope()->printChildScopes();
    //     throw MerkError("A construct method must be implemented in class: " + getName());
    // }
    auto sigOpt = getInstanceScope()->getFunction("construct", args);
    if (!sigOpt) {
        throw MerkError("Constructor for '" + getName() + "' does not match provided arguments.");
    }
    
    // Now safe to use
    auto method = std::static_pointer_cast<Method>(sigOpt->getCallable());
    auto body = method->getBody();
    body->getScope()->owner = generateScopeOwner("ClassInstanceMethod", name);
    String accessor = method->getAccessor();
    // DEBUG_LOG(LogLevel::PERMISSIVE, "Attempting to update methodScope");
    
    ASTUtils::traverse(body->getChildren(), [&](BaseAST* node) {
        if (node->getAstType() == AstType::ChainOperation) {
            // DEBUG_LOG(LogLevel::PERMISSIVE, "Updating Method...");
            auto* chainOp = static_cast<ChainOperation*>(node);
            chainOp->setResolutionMethod(1, ResolutionMode::ClassInstance, instanceScope, accessor);
        }
    }, true, false);
    
    sigOpt->call(args, getInstanceScope());
    DEBUG_FLOW_EXIT();
}


class InstanceNode : public CallableNode {
    public:
        InstanceNode(SharedPtr<ClassInstance> instance)
            : CallableNode(instance, "Instance") {
            data.type = NodeValueType::ClassInstance;
        }
    
        SharedPtr<ClassInstance> getInstance() const {
            return std::get<SharedPtr<ClassInstance>>(data.value);
        }
    };
    
Node ClassInstance::execute(const Vector<Node> args, SharedPtr<Scope> scope) const {
    (void)scope;
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    auto classScope = getCapturedScope();
    if (!classScope->hasFunction("__call__")) {
        throw MerkError("This class instance is not callable.");
    }

    auto sigOpt = classScope->getFunction("__call__", args);
    if (!sigOpt) {
        throw MerkError("No matching '__call__' overload found.");
    }
    DEBUG_FLOW_EXIT();
    return sigOpt->call(args, classScope);
}






ClassSignature::ClassSignature(SharedPtr<ClassBase> classBaseData)
: CallableSignature(classBaseData, CallableType::CLASS), accessor(classBaseData->getAccessor())
// , classBody(static_unique_ptr_cast<ClassBody>(classBaseData->getBody()->clone()))
 {
    // , classBody(std::move(classDef->getBody()->clone()))
    // classBody = classBodyData->getBody()->clone();
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    DEBUG_FLOW_EXIT();
}


ClassSignature::~ClassSignature() = default;
// , classBase(
//     std::move(static_unique_ptr_cast<ClassBody>(std::move(classBaseData->getBody()->clone())))
// )

String ClassSignature::getAccessor() const { return accessor; }

SharedPtr<ClassBase> ClassSignature::getClassDef() const {
    return std::dynamic_pointer_cast<ClassBase>(getCallable());
}

SharedPtr<Scope> ClassInstance::getInstanceScope() {return instanceScope;}
void ClassInstance::setInstanceScope(SharedPtr<Scope> scope) {instanceScope = scope;};


SharedPtr<ClassInstance> ClassSignature::instantiate(const Vector<Node>& args) const {
    // Clone the classBase from the callable stored in the signature
    DEBUG_FLOW(FlowLevel::VERY_HIGH);

    SharedPtr<ClassBase> classBase = std::static_pointer_cast<ClassBase>(getCallable());

    auto captured = classBase->getCapturedScope()->clone();
    if (!captured){
        throw MerkError("Captured Scope Does Not Exist When Instantiating class: " + classBase->getName());
    }
    auto instanced = classBase->getClassScope()->clone();
    if (!instanced){
        throw MerkError("Instance Scope Does Not Exist When Instantiating class: " + classBase->getName());

    }
    auto params = classBase->getParameters().clone();

    SharedPtr<ClassInstance> instance = makeShared<ClassInstance>(classBase->getQualifiedName(), captured, instanced, params, classBase->getQualifiedAccessor());

    // Run the constructor method if applicable
    instance->construct(args);
    DEBUG_FLOW_EXIT();
    return instance;
}


void applyMethodAccessorScopeFix(MethodDef* methodDef, SharedPtr<Scope> classScope, const String& accessor) {
    ASTUtils::traverse(
        methodDef->getBody()->getChildren(),
        [&](BaseAST* node) {
            if (node->getAstType() == AstType::Chain) {
                Chain* chain = static_cast<Chain*>(node);
                const auto& elems = chain->getElements();

                if (!elems.empty() && elems[0].name == accessor) {
                    chain->setSecondaryScope(classScope);
                    chain->setResolutionStartIndex(1);
                    chain->setResolutionMode(ResolutionMode::ClassInstance);

                    DEBUG_LOG(LogLevel::ERROR, "Applied Method Accessor fix to Chain starting with: ", accessor);
                }
            }
        },
        /* recursive = */ true,
        /* includeSelf = */ false
    );
}



// Optional: override call() to auto-instantiate when the class is "called"
Node ClassSignature::call(const Vector<Node>& args, SharedPtr<Scope> scope, SharedPtr<Scope> instanceScope) const {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);


    if (!scope){
        throw MerkError("Scope passed is no longer valid");
    }
    if (!instanceScope) {
        throw MerkError("Class Scope passed is no longer valid");
    }

    SharedPtr<ClassBase> classBase = std::static_pointer_cast<ClassBase>(getCallable());
    if (!classBase) {
        throw MerkError("Classbase Created Unsuccessfully");
    } 
    
    
    // auto captured = classBase->getCapturedScope()->clone();
    // auto captured = classBase->getCapturedScope()->clone();
    auto captured = instanceScope->getParent();
    if (!captured){
        throw MerkError("Captured Scope Does Not Exist When Instantiating class: " + classBase->getName());
    }

    auto params = classBase->getParameters().clone();

    captured->owner = generateScopeOwner("ClassInstanceCaptured", classBase->getName());
    instanceScope->owner = generateScopeOwner("ClassInstance", classBase->getName());
    SharedPtr<ClassInstance> instance = makeShared<ClassInstance>(classBase->getQualifiedName(), captured, instanceScope, params, classBase->getQualifiedAccessor());

    instance->construct(args);
    DEBUG_FLOW_EXIT();
    return ClassInstanceNode(instance);
}


Node ClassInstance::call(String name, Vector<Node> args) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    auto method = getInstanceScope()->getFunction(name, args);
    Node val = method->getCallable()->execute(args, getInstanceScope());
    DEBUG_FLOW_EXIT();
    return val;
}



Node ClassInstance::getField(const String& fieldName, TokenType type) const {    // specific to what kind of member i.e var/method
    switch (type) {
        case TokenType::Variable:
            return getField(fieldName);
            break;

        case TokenType::ClassMethodCall:
        case TokenType::FunctionCall:
            break;

        default:
            throw MerkError("Unsupported field type for '" + fieldName + "'");
    }


    // return Node();
    throw MerkError("Field or method '" + fieldName + "' not found in class instance. If a call was made that should take place in the ChainOperation");
}

Node ClassInstance::getField(const String& fieldName) const {                    // assumes a variable
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    // DEBUG_LOG(LogLevel::PERMISSIVE, "ENTERING ClassInstance::getField for Variable");
    // if (instanceScope->hasVariable(fieldName)) {
    //     return instanceScope->getVariable(fieldName);
    // } else {
    //     throw VariableNotFoundError(fieldName);
    // }
    getInstanceScope()->debugPrint();
    getInstanceScope()->printChildScopes();
    // DEBUG_LOG(LogLevel::PERMISSIVE, "++++++++++++++++++++++++++++++++++DONE WITH Instance::getField LOGGING+++++++++++++++++++++++++++++++++ ");
    // if (getInstanceScope()->getParent()){
    //     DEBUG_LOG(LogLevel::PERMISSIVE, "InstanceScope contains parent");
    // } else {
    //     DEBUG_LOG(LogLevel::PERMISSIVE, "InstanceScope does not contain parent");
    // }
    DEBUG_FLOW_EXIT();
    return getInstanceScope()->getVariable(fieldName);
}                     

void ClassInstance::declareField(const String& fieldName, const Node& var) {             // probably only used in dynamic construction of a class
    // (void)name;
    // DEBUG_LOG(LogLevel::PERMISSIVE, "Declaring Field Node&: ", fieldName);

    if (var.isValid()){ 
        // auto var = makeUnique<VarNode>(val);
        // auto val = VarNode(var);
        UniquePtr<VarNode> newVar = makeUnique<VarNode>(var);

        // auto variable = makeUnique<VarNode>(newVar, newVar->isConst, newVar->isMutable, newVar->isStatic);

        instanceScope->declareVariable(fieldName, std::move(newVar));
    }
};


void ClassInstance::declareField(const String& fieldName, const VarNode& var) {

    // DEBUG_LOG(LogLevel::PERMISSIVE, "Declaring Field VarNode&: ", fieldName);
    if (!instanceScope) {
        throw MerkError("Cannot declare field: instanceScope is missing");
    }
    UniquePtr<VarNode> newVar = makeUnique<VarNode>(var);
    instanceScope->declareVariable(fieldName, std::move(newVar));
}


void ClassInstance::updateField(const String& fieldName, Node val) const {                // most commonly used
    if (val.isValid()){
        instanceScope->updateVariable(fieldName, val);
    }
}                 




ClassInstance::~ClassInstance() {
    DEBUG_LOG(LogLevel::DEBUG, "~ClassInstance() destructor triggered");
    getInstanceScope().reset();
    getCapturedScope().reset();
}

ClassInstanceNode::ClassInstanceNode(SharedPtr<ClassInstance> callable) : CallableNode(callable, "ClassInstance") {
    data.type = NodeValueType::ClassInstance;
}

SharedPtr<Callable> ClassInstanceNode::getCallable() const {
    return std::static_pointer_cast<Callable>(std::get<SharedPtr<ClassInstance>>(data.value));
}










// Constructor: Given a class name and a parent scope,
// create a new child scope to serve as the class's own scope.
// ClassBase::ClassBase(String name, SharedPtr<Scope> parentScope)
//     : Callable(name, ParamList(), CallableType::CLASS)  // Assuming no parameters for a class definition.
// {
//     if (!parentScope) {
//         throw MerkError("Parent scope cannot be null for ClassBase.");
//     }
//     // Create a new child scope from the parent for this class.
//     classScope = parentScope->createChildScope();
//     DEBUG_LOG(LogLevel::DEBUG, "ClassBase created with name: ", name, " in parent scope: ", parentScope.get());
// }

// ClassBase::ClassBase(String otherName, String otherAccessor, UniquePtr<ClassBody> otherBody, SharedPtr<Scope> parentScope)
//     : Callable(otherName, ParamList(), CallableType::CLASS) {
//     if (!otherBody){
//         throw MerkError("ClassBase::ClassBase: newBody is null");
//     }
//     // name = otherName;
//     accessor = otherAccessor;
//     // classScope = OtherClassScope;
//     scope = parentScope;
//     // UniquePtr<ClassBody> newBody = static_unique_ptr_cast<ClassBody>(body->clone());
//     // body = std::move(otherBody);

//     // if (!body){
//     //     throw MerkError("ClassBase::ClassBase: body is null");
//     // }
    
// }



// Node ClassInstance::execute(const Vector<Node>& args, SharedPtr<Scope> callingScope) const {
//     // Step 1: Check if the instance has a 'call' method
//     // auto method = methodRegistry->get("call");  // or "construct" if you're mirroring __init__-style behavior
//     auto methodSig = getInstanceScope()->getFunction("__call__", args);
//     if (!methodSig) {
//         throw std::runtime_error("Instance is not callable.");
//     }

//     // Step 2: Create a new scope for the method call
//     SharedPtr<Scope> methodScope = makeShared<Scope>(getInstanceScope()); // use the instance’s internal scope as parent
    
//     // Step 3: Push the accessor (like 'self' or 'doggo') into the methodScope
//     // methodScope->defineVariable(getAccessor(), shared_from_this());  // if ClassInstance is enable_shared_from_this
//     auto method = methodSig.value().get().call(args, methodScope);

//     // Step 4: Execute the method with arguments
//     // return method->call(args, methodScope);
//     return method;
// }

// void ClassBase::setMethod(const String& name, SharedPtr<Callable> callable) {
//     if (!callable) {
//         throw MerkError("Attempted to assign null callable to method '" + name + "'");
//     }

//     // Check if this callable expects a 'self' (or construct-bound accessor)
//     bool isStatic = true;

//     const ParamList& params = callable->getParameters();
//     if (!params.empty()) {
//         const String& firstParam = params[0].getName();
//         if (firstParam == this->accessorName) {
//             isStatic = false;
//         }
//     }

//     // Wrap as Method if necessary
//     SharedPtr<Method> method;

//     if (callable->getCallableType() == CallableType::FUNCTION ||
//         callable->getCallableType() == CallableType::DEF) {

//         method = makeShared<Method>(
//             callable->getName(),
//             params.clone(),
//             static_unique_ptr_cast<MethodBody>(callable->getBody()->clone()),
//             callable->getCapturedScope(),
//             callable->requiresReturn()
//         );

//         method->setAccessor(this->accessorName);
//     }
//     else if (callable->getCallableType() == CallableType::METHOD) {
//         method = std::static_pointer_cast<Method>(callable);
//     } else {
//         throw MerkError("Unsupported callable type for method injection: " + callableTypeAsString(callable->getCallableType()));
//     }

//     // Tag as static if needed
//     if (isStatic) {
//         method->markStatic();
//     }

//     this->classScope->registerFunction(name, method->toCallableSignature());

//     DEBUG_LOG(LogLevel::INFO, "Injected method: ", name, " (Static: ", isStatic ? "true" : "false", ")");
// }