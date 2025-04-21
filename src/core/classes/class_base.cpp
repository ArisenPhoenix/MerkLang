// class_base.cpp
#include "core/types.h"
#include "core/errors.h"
#include "utilities/debugger.h"
#include "core/node.h"
#include "ast/ast_class.h"
#include "ast/ast_chain.h"
#include "core/classes/method.h"
#include "core/classes/class_base.h"
#include "core/scope.h"
#include "ast/ast_validate.h"
#include <cassert>





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

ClassBase::ClassBase(String otherName, String otherAccessor, UniquePtr<ClassBody> otherBody, SharedPtr<Scope> parentScope)
    : Callable(otherName, ParamList(), CallableType::CLASS) {
    if (!otherBody){
        throw MerkError("ClassBase::ClassBase: newBody is null");
    }
    // name = otherName;
    accessor = otherAccessor;
    // classScope = OtherClassScope;
    scope = parentScope;
    // UniquePtr<ClassBody> newBody = static_unique_ptr_cast<ClassBody>(body->clone());
    body = std::move(otherBody);

    if (!body){
        throw MerkError("ClassBase::ClassBase: body is null");
    }
    
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
    if (getScope()){
        // DEBUG_LOG(LogLevel::ERROR, highlight("Still Holding Onto original scope", Colors::yellow));
        getScope().reset();
    }
    body.reset();

    // body.release();
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
    return classScope->getFunction(name).value()->getCallable();
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
    capturedScope->owner = "ClassBaseCaptured(" + name + ")";
}

void ClassBase::setClassScope(SharedPtr<Scope> scope) {
    if (!scope){
        throw MerkError("New ClassScope provided to ClassBase is null");
    }
    classScope = scope;
    classScope->owner = generateScopeOwner("ClassBase", name);

}

SharedPtr<Scope> ClassBase::getCapturedScope() const {return capturedScope;}
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
    // scope = newScope;
    if (!newScope){
        throw MerkError("New Scope provided to ClassBase is null");
    }
    scope->owner = "ClassBase(" + name + ")";
    getBody()->setScope(newScope);
}


void ClassBase::setBody(UniquePtr<ClassBody> updatedBody) {body = std::move(updatedBody);}
UniquePtr<ClassBody>& ClassBase::getBody() {return body;}
ClassBody* ClassBase::getBody() const {return body.get();};

SharedPtr<CallableSignature> ClassBase::toCallableSignature() {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);

    if (!body) {
        throw MerkError("The Body in ClassBase::toCallableSignature() is null");
    }
    if (!body->getScope()){
        throw MerkError("The Body in ClassBase::toCallableSignature() has a null scope");
    }

    if (!getCapturedScope()) {
        throw MerkError("Captured Scope in ClassBase::toCallableSignature is null");
    }

    if (!getClassScope()) {
        throw MerkError("Class Scope is null in ClassBase::toCallableSignature");
    }

    // Clone the body
    auto clonedBodyBase = body->clone();
    auto clonedClassBody = static_unique_ptr_cast<ClassBody>(std::move(clonedBodyBase));

    // Clone captured scope (the one with free variables)
    SharedPtr<Scope> clonedCapturedScope = getCapturedScope()->clone();
    clonedCapturedScope->owner = "ClassBaseCaptured(" + name + ")";
    clonedClassBody->setScope(clonedCapturedScope);

    // Create ClassBase using the cloned captured scope as parent
    SharedPtr<ClassBase> classBase = makeShared<ClassBase>(
        getName(),
        accessor,
        std::move(clonedClassBody),
        clonedCapturedScope // parentScope
    );

    // Clone parameters
    ParamList clonedParams;
    for (const auto& param : parameters) {
        clonedParams.addParameter(param.copy());
    }
    classBase->setParameters(clonedParams);

    if (!getClassScope()){
        throw MerkError("ClassScope was not set before toCallableSignature()");

    }

    // Clone the classScope (member scope)
    SharedPtr<Scope> clonedClassScope = getClassScope()->clone();
    clonedClassScope->owner = "Class(" + name + ")";

    // Attach classScope to clonedCapturedScope
    clonedCapturedScope->appendChildScope(clonedClassScope, "ClassBase::toCallableSignature");

    // Set both scopes on class
    classBase->setCapturedScope(clonedCapturedScope);

    classBase->setClassScope(getClassScope()->clone());


    // DEBUG_LOG(LogLevel::ERROR, "ClassBase::toCallableSignature()", "Scopes cloned and linked");

    // Wrap into ClassSignature
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
    auto scope = capturedScope.lock();
    if (!scope) {
        throw MerkError("ClassInstance::getCapturedScope: scope is null");
    }
    // if (auto scope = capturedScope.lock()) {
    return scope;
    // }
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

    if (!getInstanceScope()->hasFunction("construct")) {
        throw MerkError("A construct method must be implemented in class: " + getName());
    }
    auto sigOpt = getInstanceScope()->getFunction("construct", args);
    if (!sigOpt || !sigOpt.value()){
        throw FunctionNotFoundError("construct");
    }
    // auto sigOpt = getInstanceScope()->getFunction("construct", args);
    getInstanceScope()->debugPrint();
    getInstanceScope()->owner = "ClassInstance(" + name + ")";
    if (!sigOpt.has_value()) {
        throw MerkError("Constructor for '" + getName() + "' does not match provided arguments.");
    }

    sigOpt.value()->call(args, getInstanceScope());
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
    if (!sigOpt.has_value()) {
        throw MerkError("No matching '__call__' overload found.");
    }
    DEBUG_FLOW_EXIT();
    return sigOpt.value()->call(args, classScope);
}






ClassSignature::ClassSignature(SharedPtr<ClassBase> classBaseData)
: CallableSignature(classBaseData, CallableType::CLASS), accessor(classBaseData->getAccessor()), classBody(static_unique_ptr_cast<ClassBody>(classBaseData->getBody()->clone())) {
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


    // Pass the classBase directly into the ClassInstance constructor (it clones internally)
    // String& className = ;
    // ClassInstance(String& name, SharedPtr<Scope> capturedScope, SharedPtr<Scope> instanceScope, ParamList params, String& accessor)
    // ClassInstance(classBase->getQualifiedName(), classBase->getCapturedScope(), classBase->getClassScope(), classBase->getParameters(), classBase->getAccessor());

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
Node ClassSignature::call(const Vector<Node>& args, SharedPtr<Scope> scope, SharedPtr<Scope> classScope) const {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);


    if (!scope){
        throw MerkError("Scope passed is no longer valid");
    }
    if (!classScope) {
        throw MerkError("Class Scope passed is no longer valid");
    }

    SharedPtr<ClassBase> classBase = std::static_pointer_cast<ClassBase>(getCallable());
    if (!classBase) {
        throw MerkError("Classbase Created Unsuccessfully");
    } 
    
    // DEBUG_LOG(LogLevel::ERROR, "ClassBase created");
    // Clone body and attach scope
    if (!classBody){
        throw MerkError("ClassBody doesn't exist");
    }
    auto body = classBody->clone();
    if (!body) {
        throw MerkError("body Created Unsuccessfully");
    }
    // DEBUG_LOG(LogLevel::ERROR, "body created");

    auto clonedBody = static_unique_ptr_cast<ClassBody>(std::move(body));
    if (!clonedBody) {
        throw MerkError("clonedBody Created Unsuccessfully");
    }

    // DEBUG_LOG(LogLevel::ERROR, "clonedBody created");
    // scope->owner = "ClassSig(" + classBase->getName() + ")";
    clonedBody->setScope(scope);
    // classScope->owner = "ClassInstance(" + classBase->getName() + ")";
    // Fix classScope for methods
    auto& children = clonedBody->getChildren();
    for (auto& child : children) {
        if (child->getAstType() == AstType::ClassMethodDef) {
            auto* methodDef = static_cast<MethodDef*>(child.get());

            methodDef->setClassScope(classScope);
            methodDef->setScope(scope);
            // methodDef->getScope()->owner = "Method(" + methodDef->getName() + ")";

            applyMethodAccessorScopeFix(methodDef, classScope, getAccessor());
        }
    }

    // DEBUG_LOG(LogLevel::ERROR, "ChildScopes updated");

    // Inject updated body back into base
    classBase->setBody(std::move(clonedBody));
    // classBody->setScope(scope);
    // classBase->setCapturedScope()`
    // DEBUG_LOG(LogLevel::ERROR, "Body reset");

    // Instantiate
    // SharedPtr<ClassInstance> instance = makeShared<ClassInstance>(classBase);
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

    // DEBUG_LOG(LogLevel::ERROR, "Instance Created");
    // instance->getScope()->owner = "ClassInstance(" + instance->getName() + ")";
    instance->construct(args);
    DEBUG_FLOW_EXIT();
    return ClassInstanceNode(instance);
}



Node ClassInstance::getField(const String& name, TokenType type) const {
    switch (type) {
        case TokenType::Variable:
            if (instanceScope->hasVariable(name)) {
                return instanceScope->getVariable(name);
            }
            break;

        case TokenType::ClassMethodCall:
        case TokenType::FunctionCall:
        if (instanceScope->hasFunction(name)) {
            auto sig = instanceScope->getFunction(name);


            return MethodNode(sig.value()->getCallable());
        }
            break;

        default:
            throw MerkError("Unsupported field type for '" + name + "'");
    }

    throw MerkError("Field or method '" + name + "' not found in class instance.");
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