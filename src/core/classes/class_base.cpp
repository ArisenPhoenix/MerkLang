// class_base.cpp
#include "core/types.h"
#include "core/errors.h"
#include "utilities/debugger.h"
#include "core/node.h"

#include "core/classes/method.h"
#include "core/classes/class_base.h"
#include "core/scope.h"

#include <cassert>




// Constructor: Given a class name and a parent scope,
// create a new child scope to serve as the class's own scope.
ClassBase::ClassBase(String name, SharedPtr<Scope> parentScope)
    : Callable(name, ParamList(), CallableType::CLASS)  // Assuming no parameters for a class definition.
{
    if (!parentScope) {
        throw MerkError("Parent scope cannot be null for ClassBase.");
    }
    // Create a new child scope from the parent for this class.
    classScope = parentScope->createChildScope();
    DEBUG_LOG(LogLevel::DEBUG, "ClassBase created with name: ", name, " in parent scope: ", parentScope.get());
}

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


// Add a method to the class by storing its signature in the class scope.
// Here we assume that setVariable will store a Node that wraps the method signature.
void ClassBase::addMethod(const String& name, SharedPtr<Method> method) {
    // Wrap the method signature into a Node.
    classScope->registerFunction(name, method->toCallableSignature());
    DEBUG_LOG(LogLevel::DEBUG, "Method added to ClassBase: ", name);
}

// Retrieve a method's signature from the class scope.
SharedPtr<Callable> ClassBase::getMethod(const String& name) {
    return classScope->getFunction(name)->get().getCallable();
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
    capturedScope = scope;
}

void ClassBase::setClassScope(SharedPtr<Scope> scope) {
    classScope = scope;
}

SharedPtr<Scope> ClassBase::getCapturedScope() const {return capturedScope;}
SharedPtr<Scope> ClassBase::getClassScope() const {return classScope;}

// Returns the class's scope.
// SharedPtr<Scope> ClassBase::getScope() const {return scope;}

String ClassBase::toString() const {
    return "ClassBase()";
};

Node ClassBase::execute(Vector<Node> args, SharedPtr<Scope> scope) const {
    (void) args;
    (void) scope;
    return Node();
};

void ClassBase::setBody(UniquePtr<ClassBody> updatedBody) {body = std::move(updatedBody);}
UniquePtr<ClassBody>& ClassBase::getBody() {return body;}


SharedPtr<CallableSignature> ClassBase::toCallableSignature() {
    DEBUG_FLOW(FlowLevel::LOW);

    if (!body) {
        throw MerkError("The Body in ClassBase::toCallableSignature() is null");
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

    // Create ClassBase using the cloned captured scope as parent
    SharedPtr<ClassBase> classBase = std::make_shared<ClassBase>(
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

    // Clone the classScope (member scope)
    SharedPtr<Scope> clonedClassScope = getClassScope()->clone();

    // Attach classScope to clonedCapturedScope
    clonedCapturedScope->appendChildScope(clonedClassScope);

    // Set both scopes on class
    classBase->setCapturedScope(clonedCapturedScope);
    if (!this->getClassScope()) {
        throw MerkError("ClassScope was not set before toCallableSignature()");
    }
    classBase->setClassScope(this->getClassScope()->clone());

    DEBUG_LOG(LogLevel::ERROR, "ClassBase::toCallableSignature()", "Scopes cloned and linked");

    // Wrap into ClassSignature
    SharedPtr<CallableSignature> classSig = std::make_shared<ClassSignature>(classBase);
    DEBUG_FLOW_EXIT();
    return classSig;
}


// Produce a ClassSignature for this class definition.
// SharedPtr<CallableSignature> ClassBase::toCallableSignature() const {
//     DEBUG_FLOW(FlowLevel::LOW);

//     if (!body){
//         throw MerkError("The Body in ClassBase::toCallableSignature() is null");
//     }

//     if (!scope){
//         throw MerkError("scope is null in ClassBase::toCallableSignature()");
//     }
//     // getBody(); 
//     auto clonedBodyBase = body->clone();

//     DEBUG_LOG(LogLevel::ERROR, "ClassBase::toCallableSignature()", "Class Body Was cloned");
//     auto clonedClassBody = static_unique_ptr_cast<ClassBody>(std::move(clonedBodyBase));
//     DEBUG_LOG(LogLevel::ERROR, "ClassBase::toCallableSignature()", "Class Body casted");


//     ParamList clonedParams;
//     for (const auto& param : parameters) {
//         clonedParams.addParameter(param.copy());  // Create a copy
//     }
//     DEBUG_LOG(LogLevel::ERROR, "ClassBase::toCallableSignature()", "Parameters Were Copied");

//     if (!getCapturedScope()){
//         throw MerkError("Captured Scope in ClassBase::toCallableSignature is null");
//     }

//     // Create a new runtime Method with the cloned body.
//     SharedPtr<ClassBase> classBase = std::make_shared<ClassBase>(getName(), accessor, std::move(clonedClassBody), getCapturedScope()->clone());
//     if (!classScope){
//         throw MerkError("Class Scope is null in ClassBase::toCallableSignature");
//     }
//     classBase->setClassScope(classScope);
//     DEBUG_LOG(LogLevel::ERROR, "ClassBase::toCallableSignature()", "Class was created and made shared");
//     DEBUG_LOG(LogLevel::ERROR, "Attempting To Create a Detached Scope in ClassBase::toCallableSignature");
//     auto classDefScope = scope->detachScope({});
//     DEBUG_LOG(LogLevel::ERROR, "Detached Scope was successfully created in ClassBase::toCallableSignature");

//     DEBUG_LOG(LogLevel::ERROR, "Attempting To Set Detached Scope in ClassBase::toCallableSignature");

//     classBase->setClassScope(classDefScope);
//     DEBUG_LOG(LogLevel::ERROR, "ClassBase::toCallableSignature()", "ClassScope Was Set");
//     if (!classBase->getClassScope()) {
//         throw MerkError("Class Base is null after setting in ClassBase::toCallableSignature");
//     }
    
//     // Wrap the Method in a FunctionSignature and return it.
//     // SharedPtr<CallableSignature> classSig = std::make_shared<CallableSignature>(classBase, callType);
//     SharedPtr<CallableSignature> classSig = std::make_shared<ClassSignature>(classBase);
//     assert(classSig && "Cast to ClassSignature should never fail now.");

//     DEBUG_LOG(LogLevel::ERROR, "ClassBase::toCallableSignature()", "Class Signature Was Created");
//     DEBUG_FLOW_EXIT();
//     return classSig;

// }







ClassInstance::ClassInstance(SharedPtr<ClassBase> base)
    : Callable(base->getName(), base->getParameters(), CallableType::INSTANCE),
      baseClass(base),
      instanceScope(base->getClassScope())
{
    // Deep copy the captured scope from base
    accessor = base->getAccessor();
    capturedScope = base->getCapturedScope()->clone();
}

SharedPtr<Scope> ClassInstance::getCapturedScope() const {
    return capturedScope;
}

void ClassInstance::setCapturedScope(SharedPtr<Scope> scope) {
    capturedScope = scope;
}

String ClassInstance::toString() const {
    return "<Instance of " + getName() + ">";
}

SharedPtr<CallableSignature> ClassInstance::toCallableSignature() {
    throw MerkError("Instances are not directly callable unless '__call__' is defined.");
}

// Called only if someone explicitly invokes the instance (not common unless __call__ exists)
// Node ClassInstance::execute(Vector<Node>& args, SharedPtr<Scope> callerScope) const override {


// This is the key initializer you're asking for
// void ClassInstance::construct(const Vector<Node>& args) {
//     if (!capturedScope->hasFunction("construct")) {
//         return; // No constructor defined
//     }

//     auto sigOpt = capturedScope->getFunction("construct", args);
//     if (!sigOpt.has_value()) {
//         throw MerkError("Constructor for '" + getName() + "' does not match provided arguments.");
//     }

//     sigOpt->get().call(args, capturedScope);
// }

void ClassInstance::construct(const Vector<Node>& args) {
    if (!instanceScope->hasFunction("construct")) {
        // return; // No constructor defined
        throw MerkError("A construct method must be implemented in class" + getName());
    }

    auto sigOpt = instanceScope->getFunction("construct", args);
    if (!sigOpt.has_value()) {
        throw MerkError("Constructor for '" + getName() + "' does not match provided arguments.");
    }

    sigOpt->get().call(args, instanceScope);
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
    
    auto classScope = getCapturedScope();
    if (!classScope->hasFunction("__call__")) {
        throw MerkError("This class instance is not callable.");
    }

    auto sigOpt = classScope->getFunction("__call__", args);
    if (!sigOpt.has_value()) {
        throw MerkError("No matching '__call__' overload found.");
    }

    return sigOpt->get().call(args, classScope);
}

// Node ClassInstance::execute(const Vector<Node>& args, SharedPtr<Scope> callingScope) const {
//     // Step 1: Check if the instance has a 'call' method
//     // auto method = methodRegistry->get("call");  // or "construct" if you're mirroring __init__-style behavior
//     auto methodSig = getInstanceScope()->getFunction("__call__", args);
//     if (!methodSig) {
//         throw std::runtime_error("Instance is not callable.");
//     }

//     // Step 2: Create a new scope for the method call
//     SharedPtr<Scope> methodScope = std::make_shared<Scope>(getInstanceScope()); // use the instance’s internal scope as parent
    
//     // Step 3: Push the accessor (like 'self' or 'doggo') into the methodScope
//     // methodScope->defineVariable(getAccessor(), shared_from_this());  // if ClassInstance is enable_shared_from_this
//     auto method = methodSig.value().get().call(args, methodScope);

//     // Step 4: Execute the method with arguments
//     // return method->call(args, methodScope);
//     return method;
// }





ClassSignature::ClassSignature(SharedPtr<ClassBase> classDef)
: CallableSignature(classDef, CallableType::CLASS),
    accessor(classDef->getAccessor()) {}

String ClassSignature::getAccessor() const { return accessor; }

SharedPtr<ClassBase> ClassSignature::getClassDef() const {
    return std::dynamic_pointer_cast<ClassBase>(getCallable());
}

SharedPtr<ClassInstance> ClassSignature::instantiate(const Vector<Node>& args) const {
    auto classBase = getClassDef();

    if (!classBase) {
        throw MerkError("Failed to cast Callable to ClassBase in ClassSignature.");
    }

    SharedPtr<Scope> instanceScope;
    if (classBase->getClassScope()) {
        instanceScope = classBase->getClassScope()->clone();
    } else {
        throw MerkError("ClassScope was null during instantiation.");
    }

    SharedPtr<Scope> capturedScope;
    if (classBase->getCapturedScope()) {
        capturedScope = classBase->getCapturedScope()->clone();
    } else {
        throw MerkError("CapturedScope was null during instantiation.");
    }

    // Clone class scope for instance
    // SharedPtr<Scope> instanceScope = classBase->getClassScope()->clone();
    // SharedPtr<Scope> capturedScope = classBase->getCapturedScope()->clone();

    // Create instance
    SharedPtr<ClassInstance> instance = std::make_shared<ClassInstance>(classBase);
    instance->setCapturedScope(capturedScope);
    

    // Run constructor if defined
    instance->construct(args);
    capturedScope->appendChildScope(instanceScope);

    return instance;
}

// Optional: override call() to auto-instantiate when the class is "called"
Node ClassSignature::call(const Vector<Node>& args, SharedPtr<Scope> scope) const {
    (void)scope;
    return Node(ClassInstanceNode(instantiate(args)));
}


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

//         method = std::make_shared<Method>(
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
