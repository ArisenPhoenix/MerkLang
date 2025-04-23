#include <iostream>
// #include <regex>
#include "core/types.h"
#include "utilities/debugger.h"
#include "utilities/helper_functions.h"
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




std::pair<size_t, String> findDelimiter(const String& s) {
    size_t posDelimeter = s.find(".");
    if (posDelimeter != String::npos) {
        return { posDelimeter, "." };
    }

    // If the delimiter is not found, return npos and an empty string.
    return { String::npos, "" };
}

Vector<String> splitIdentifierChain(const String& s) {
    Vector<String> parts;
    size_t start = 0;
    while (start < s.size()) {
        // Find the next delimiter starting from 'start'
        auto [pos, delim] = findDelimiter(s.substr(start));
        if (pos == String::npos) {
            // No more delimiters found; add the remainder of the string
            parts.push_back(s.substr(start));
            break;
        } else {
            // 'pos' is relative to s.substr(start), so the absolute position is start + pos.
            parts.push_back(s.substr(start, pos));
            // Move start past the delimiter
            start += pos + delim.size();
        }
    }
    return parts;
}

ClassBody::ClassBody(UniquePtr<CodeBlock>&& block)
    : CallableBody(block->getScope()) {
    this->children = std::move(block->children);
    block.reset();
}
ClassBody::ClassBody(SharedPtr<Scope> scope)
    : CallableBody(scope) {}

UniquePtr<BaseAST> ClassBody::clone() const {
    if (!getScope()){
        throw MerkError("No Scope Present in ClassBody::clone()");
    }
    UniquePtr<ClassBody> newBlock = makeUnique<ClassBody>(getScope());
    std::unordered_map<String, SharedPtr<Scope>> methodScopes;

    for (const auto &child : children) {
        DEBUG_LOG(LogLevel::TRACE, "ClassBody::clone()", "current child", child->toString());

        if (!child){
            DEBUG_LOG(LogLevel::WARNING, "ClassBody::clone()", "Null child encountered in ClassBody::clone()");
        }

        newBlock->addChild(child->clone());
        
        DEBUG_LOG(LogLevel::TRACE, "ClassBody::clone()", "Added Child to ClassBody", child->getAstTypeAsString());

    }
    return newBlock;
}


MethodBody::MethodBody(UniquePtr<CodeBlock>&& body) : CallableBody(std::move(body)){};
MethodBody::MethodBody(SharedPtr<Scope> scope) : CallableBody(scope) {}
MethodBody::MethodBody(UniquePtr<CallableBody>* body) : CallableBody(std::move(body)) {}

UniquePtr<BaseAST> MethodBody::clone() const {
    UniquePtr<MethodBody> newBlock = makeUnique<MethodBody>(getScope());

    for (const auto &child : children) {
        newBlock->addChild(child->clone());
    }
    return newBlock;
}

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
    funcDef.release();
}



ParamList& ClassDef::getParameters() {return parameters;}
ParamList& MethodDef::getParameters() {return parameters;}
const ParamList& MethodDef::getParameters() const {return parameters;}

MethodCall::MethodCall(String name, Vector<UniquePtr<ASTStatement>> arguments, SharedPtr<Scope> scope)
    : CallableCall(name, std::move(arguments), scope) {
}

void MethodDef::setMethodAccessor(String& accessorName) {accessor = accessorName;}
String MethodDef::getMethodAccessor() const {return accessor;}

Vector<String> ClassDef::removeMethodAccessors(UniquePtr<ClassBody>& clonedBody) const {
    auto& methods = clonedBody->collectChildrenOfType(clonedBody->getChildren(), {AstType::ClassMethodDef});
    Vector<String> removed;
    // Step 2: Analyze method parameters for potential accessor matching
    for (auto& methodNode : methods) {
        auto* methodDef = static_cast<MethodDef*>(methodNode);
        auto allNodes = methodDef->getBody()->getAllAst(true);
        // for (const auto* node : allNodes) {
        //     DEBUG_LOG(LogLevel::TRACE, highlight("Node type: " + astTypeToString(node->getAstType()), Colors::red));
        // }
        ParamList& params = methodDef->getParameters();  // you might need a mutable accessor here
        String methodName = methodDef->getName();
        // DEBUG_LOG(LogLevel::DEBUG, "Params for", methodName, "=", params.toString());
        bool hasAccessor = false;
        if (!params.empty()) {
            const String& firstParam = params[0].getName();
            if (firstParam == accessor) {
                hasAccessor = true;
            }
        }
         // Validate improper accessor usage if not explicitly provided
         if (!hasAccessor) {
            validateNoImplicitAccessor(Vector<const BaseAST*>{methodDef->getBody().get()}, accessor, methodName);
        }

        // If explicit, now remove it
        else {
            params.eraseByName(accessor);
            DEBUG_LOG(LogLevel::DEBUG, "Removed implicit accessor from method:", methodName);
            removed.emplace_back(methodName);
        }

    }
    return removed;
}


void applyAccessorScopeFix(MethodDef* methodDef, SharedPtr<Scope> classScope, const String& accessor) {
    if (!classScope) {
        throw MerkError("ApplyAccessorFix classScope is null");
    }

    const auto& bodyChildren = methodDef->getBody()->getChildren();

    ASTUtils::traverse(bodyChildren, [&](BaseAST* node) {
        if (node->getAstType() != AstType::Chain) return;
        auto& params = methodDef->getParameters();
        Chain* chain = static_cast<Chain*>(node);
        const auto& elems = chain->getElements();
        bool isAccessorDeclared = !params.empty() && params[0].getName() == accessor;
        if (!elems.empty() && elems[0].name == accessor) {
            if (!isAccessorDeclared){
                // Validate: is accessor declared explicitly as a parameter

                throw MerkError("Method '" + methodDef->getName() + "' references '" + accessor +
                "' via a chain but does not declare it as a parameter.");
            }
            chain->setResolutionStartIndex(1);
            chain->setResolutionMode(ResolutionMode::ClassInstance);

        }
    }, true, false);
}


void fixupClassChains(SharedPtr<Scope> classScope) {
    for (const auto& [name, signatures] : classScope->getFunctionRegistry()->getFunctions()) {
        for (const auto& sigPtr : signatures) {
            auto callable = sigPtr->getCallable();
            if (!callable || callable->getCallableType() != CallableType::METHOD) continue;

            auto method = std::dynamic_pointer_cast<Method>(callable);
            if (!method) continue;

            auto* body = method->getBody();
            if (!body) continue;

            ASTUtils::traverse(body->getChildren(), [&](BaseAST* node) {
                if (node->getAstType() == AstType::Chain) {
                    auto* chain = static_cast<Chain*>(node);
                    if (chain->getResolutionMode() == ResolutionMode::ClassInstance) {
                        chain->setSecondaryScope(classScope);
                    }
                }
            }, true, false);
        }
    }
}

void stripImplicitAccessor(MethodDef* methodDef, const String& accessor) {
    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    ParamList& params = methodDef->getParameters();

    if (!params.empty() && params[0].getName() == accessor) {
        params.eraseByName(accessor);
        DEBUG_LOG(LogLevel::ERROR, highlight("Stripped implicit accessor from method: " + methodDef->getName(), Colors::orange));
        methodDef->parameters = params;

        return;
    }

    DEBUG_LOG(LogLevel::ERROR, highlight("Could Not Stript the implicit acccessor from method: " + methodDef->getName(), Colors::orange));
    DEBUG_FLOW_EXIT();
    
}


Node ClassDef::evaluate(SharedPtr<Scope> defScope) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    
    if (!defScope) {
        throw MerkError("No Scope Was Found in ClassDef::evaluate()");
    }

    if (!body->getScope()){
        throw MerkError("scope not present in ClassDef::evaluate in body");
    }
    // body->getScope()->owner = generateScopeOwner("ClassDefBody", name);

    // scope->owner = "ClassDef::evaluate(" + name + ")";

    FreeVars freeVarNames = body->collectFreeVariables();
    if (freeVarNames.size() == 0) {
        DEBUG_LOG(LogLevel::ERROR, "There Are No Free Variables in classdef: ", name);
    }

    SharedPtr<Scope> classDefCapturedScope = defScope->detachScope(freeVarNames);
    if (!classDefCapturedScope) {
        throw MerkError("Failed to create detachedScope in ClassDef::evaluate");
    }
    classDefCapturedScope->isCallableScope = true;
    classDefCapturedScope->owner = generateScopeOwner("ClassDef", name); //"ClassDef(" + name + ")";

    // auto classScope = classDefCapturedScope->createChildScope();
    auto classScope = classDefCapturedScope->makeCallScope();
    if (!classScope){
        throw MerkError("classScope was not created correctly on the ClassBase.");
    }
    classScope->owner = generateScopeOwner("ClassMainScopeClass", name);
    classScope->isCallableScope = true;
    classScope->isDetached = true;
    DEBUG_LOG(LogLevel::ERROR, highlight("Attempting captured setting on cls", Colors::yellow));
    classScope->debugPrint();

    SharedPtr<ClassBase> cls = makeShared<ClassBase>(name, accessor, classScope);


    DEBUG_LOG(LogLevel::ERROR, "Created cls");

    cls->setParameters(parameters.clone());
    DEBUG_LOG(LogLevel::ERROR, "Parameters set on cls");

    DEBUG_LOG(LogLevel::ERROR, highlight("Created classScope", Colors::yellow));


    cls->setCapturedScope(classDefCapturedScope);
    if (!cls->getCapturedScope()) {
        throw MerkError("CapturedScope was not set correctly on the ClassBase.");
    }

    DEBUG_LOG(LogLevel::ERROR, highlight("Attempting classScope setting on cls", Colors::yellow));

    cls->setClassScope(classScope);
    if (!cls->getClassScope()){
        throw MerkError("ClassDef::evlaute classScope is null");
    }
    DEBUG_LOG(LogLevel::ERROR, "ClassScope Below: ");
    cls->getClassScope()->debugPrint();

    auto classBody = static_cast<ClassBody*>(getBody());
    String bodyAccess = accessor;
    classBody->setAccessor(bodyAccess);
    classBody->setCapturedScope(classDefCapturedScope);
    classBody->setClassScope(cls->getClassScope());
    // getBody()->evaluate(cls->getClassScope());
    classBody->evaluate(cls->getClassScope());
    // Ensure the class has a construct method
    if (!cls->getClassScope()->hasFunction("construct")) {
        cls->getClassScope()->debugPrint();
        cls->getClassScope()->printChildScopes();
        throw MerkError("Class '" + name + "' must implement a 'construct' method.");
    }
    classBody->setCapturedScope(nullptr);
    classBody->setClassScope(nullptr);

    defScope->appendChildScope(classDefCapturedScope, "ClassDef::evaluate");
    defScope->registerClass(name, cls);
    
    DEBUG_LOG(LogLevel::ERROR, "ClassDef::classScope created: ");

    cls->getClassScope()->debugPrint();
    cls->getClassScope()->printChildScopes();
    classDefCapturedScope->appendChildScope(cls->getClassScope());
    
    DEBUG_FLOW_EXIT();
    return ClassNode(cls);
}

SharedPtr<Scope> MethodDef::getClassScope() const {return classScope;}


void ClassBody::setClassScope(SharedPtr<Scope> newScope) {classScope = newScope;}
void ClassBody::setAccessor(String& classAccessor) {accessor = classAccessor;}
void ClassBody::setCapturedScope(SharedPtr<Scope> capturedScope) {classCapturedScope = capturedScope;}

Node ClassBody::evaluate(SharedPtr<Scope> classScope) const {
    DEBUG_FLOW();
    if (!classCapturedScope) {
        throw MerkError("Class Captured Scope Was Not Set On Body");
    }

    if (!classScope) {
        throw MerkError("Class Scope Was Not Set On Body");
    }

    if (accessor.empty()) {
        throw MerkError("Accessor Was Not Set On Body");
    }


    getScope()->owner = generateScopeOwner("Class", "Body");
    DEBUG_LOG(LogLevel::ERROR, highlight("Moving to Apply Accessor Scope Fix", Colors::yellow));
    Vector<String> methods;
    for (const auto& child : getMutableChildren()) {
        DEBUG_LOG(LogLevel::ERROR, "Evaluating AST Child: ", child->getAstTypeAsString());

        switch (child->getAstType()) {
            case AstType::VariableDeclaration:                // protected variables stored in captured scope, 
                child->evaluate(classScope);
            break;
            case AstType::ClassMethodDef:
                if (child->getAstType() == AstType::ClassMethodDef){
                    auto* methodDef = static_cast<MethodDef*>(child.get());
                    DEBUG_LOG(LogLevel::ERROR, highlight("Handling Method: " + methodDef->getName(), Colors::yellow));

                    auto methodScope = classScope->createChildScope();
                    if (!methodScope){
                        throw MerkError("generated methodscope is null for method in ClassDef::evaluate");
                    }
                                    // for providing its own classScope from which to work with
                    methodDef->setClassScope(classScope); 

                    // methodClassScope->owner = generateScopeOwner("Method", methodDef->getName());
                    // methodDef->setScope(methodscope);
                    // methodDef->setClassScope(classScope);        // for registering itself into the class registry
                    if (methodDef->getClassScope().get() != classScope.get()) {
                        throw MerkError("method class Scope is not the same as cls->classScope");
                    } else {
                        DEBUG_LOG(LogLevel::ERROR, highlight("Method class scope and class's scope match", Colors::yellow));
                    }
                    applyAccessorScopeFix(methodDef, classScope, accessor);
                    stripImplicitAccessor(methodDef, accessor);
                    methods.emplace_back(methodDef->getName());
                    methodDef->evaluate(methodScope);
                    // methodDef->getBody()->getScope()->owner = generateScopeOwner("Method", methodDef->getName());
                    // methodDef->getClassScope()->owner = generateScopeOwner("METHOD", methodDef->getName());
                    // cls->getClassScope()->appendChildScope(methodscope);     // for ensuring continuity
                    // auto methodNode = val.getValue();
                    
                    DEBUG_LOG(LogLevel::ERROR, highlight("Setting methodDef's classScope: ", Colors::red));                    
                    
                } 
                break;

            // case AstType::VariableAssignment:
            // case AstType::ParameterAssignment:
            case AstType::ClassDefinition:
                break;

            default:
                // child->evaluate(classScope);
                DEBUG_LOG(LogLevel::ERROR, "Unhandled AST type in ClassBody:", child->getAstTypeAsString());
                throw MerkError("Unexpected AST statement in ClassBody: " + child->getAstTypeAsString());
        }
    }


    for (auto& method : methods){
        DEBUG_LOG(LogLevel::ERROR, "Assuming Method ", highlight(method, Colors::blue), "Was Registered");
        if (!classScope->hasFunction(method)){
            throw MerkError("Method: " + method + " Was Not Registered To Class Scope");
        }
    }

    DEBUG_LOG(LogLevel::ERROR, highlight("Finished Applying Accessor Scope Fix", Colors::yellow));
    // Possibly validate the structure but do nothing
    DEBUG_FLOW_EXIT();
    return Node();  // or nullptr
}

Node ClassCall::evaluate(SharedPtr<Scope> callScope) const {
    // The provided scope is the scope calling for the class instance

    DEBUG_FLOW(FlowLevel::VERY_HIGH);
    if (!callScope){
        throw MerkError("Initial Scope Failed in ClassCall::evaluate()");
    }
    DEBUG_LOG(LogLevel::ERROR, "Initial Scope Worked");
    if (!getScope()) {
        throw MerkError("CallCall::evaluate(): getScope() is null");
    }

    Vector<Node> argValues = handleArgs(callScope);
    DEBUG_LOG(LogLevel::ERROR, highlight("ClassCall Args:", Colors::yellow));

    for (auto& arg : argValues){
        DEBUG_LOG(LogLevel::ERROR, arg);
    }

    auto classOpt = callScope->getClass(name);
    if (!classOpt.has_value()) {
        throw MerkError("Class not found: " + name);
    }

    auto classSig = classOpt.value();

    DEBUG_LOG(LogLevel::ERROR, "Getting Class");
    auto classTemplate = classSig->getClassDef();
    DEBUG_LOG(LogLevel::ERROR, "Got Class");
    auto params = classTemplate->getParameters();
    // auto params = classSig->getParameterTypes();
    if (!classTemplate){
        throw MerkError("ClassBase <classTemplate> could not be pulled from ClassSignature for Class: " + name);
    }
    auto capturedScope = classTemplate->getCapturedScope();
    if (!capturedScope){
        throw MerkError("capturedScope Failed in ClassCall::evaluate()");
    } 
    DEBUG_LOG(LogLevel::ERROR, "Class capturedScope Below:");
    capturedScope->debugPrint();

    auto classScope = classTemplate->getClassScope()->makeCallScope();
    // classScope->owner = generateScopeOwner("ClassCall", name);

    if (!classScope){
        throw MerkError("classScope Failed in ClassCall::evaluate()");
    }

    DEBUG_LOG(LogLevel::ERROR, "ClassCallScope Below:");
    classScope->debugPrint();
    fixupClassChains(classScope);

    SharedPtr<Scope> clonedCaptured = classTemplate->getCapturedScope()->clone();
    // clonedCaptured->owner = generateScopeOwner("CapturedClone", name);
    // Use shared classScope for method access
    SharedPtr<Scope> sharedClassScope = classTemplate->getClassScope();

    // Create private instance scope for 'self.x', 'self.y', etc.
    SharedPtr<Scope> privateInstanceScope = sharedClassScope->createChildScope();
    privateInstanceScope->owner = generateScopeOwner("PrivateInstanceData", classTemplate->getName());

    // Create the resolution stack
    clonedCaptured->appendChildScope(sharedClassScope, "ClassMethods");
    sharedClassScope->appendChildScope(privateInstanceScope, "InstancePrivateVars");

    if (!privateInstanceScope){
        throw MerkError("originalDefScope Failed in ClassCall::evaluate()");
    }

    DEBUG_LOG(LogLevel::ERROR, "Class originalDefScope Below:");
    privateInstanceScope->debugPrint();

    SharedPtr<ClassInstance> instance = makeShared<ClassInstance>(
        classTemplate->getQualifiedName(),
        clonedCaptured,
        privateInstanceScope,
        // classTemplate->getParameters(),
        params,
        classTemplate->getQualifiedAccessor()
    );

    instance->getInstanceScope()->owner = generateScopeOwner("ClassInstance", name);
    
    instance->construct(argValues);

    // Node instanceNode = classSig->call(argValues, callScope, classScope);
    DEBUG_LOG(LogLevel::ERROR, highlight("Instance CallScope Owner", Colors::bold_green), highlight(callScope->owner, Colors::yellow));
    // InstanceNode()
    Node instanceNode = classSig->call(argValues, callScope, classScope);
    DEBUG_FLOW_EXIT();
    return instanceNode;
}

bool MethodDef::isConstructor() const {
    return name == "construct";
}

Node MethodDef::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);

    if (!scope){
        throw MerkError("Provided Scope to MethodDef::evaluate is null");
    }

    if (!getScope()){
        throw MerkError("MethodDef::evaluate, scope is null");
    }

    if (!getClassScope()) {
        throw MerkError("Class Scope was not supplied to Method: " + name);
    }


    auto freeVarNames = body->collectFreeVariables();
    DEBUG_LOG(LogLevel::ERROR,"MethodDef::evaluate | ", "CallType: ", callableTypeAsString(callType), "SubType: ", callableTypeAsString(methodType));
    
    if (callType == CallableType::FUNCTION){
        FreeVars tempFreeVars = freeVarNames;
        for (auto& param : parameters){
            auto it = tempFreeVars.find(param.getName()); // find a matching param name
            if (it != tempFreeVars.end()){                // indicates a match
                tempFreeVars.erase(it);
            }
        }

        if (tempFreeVars.size() > 0){
            std::ostringstream oss;
            for (auto& var : tempFreeVars){
                DEBUG_LOG(LogLevel::ERROR, highlight("'", Colors::yellow), highlight(var, Colors::purple), highlight("'", Colors::yellow), " ");
            }
            throw MerkError("The Following Vars: " + oss.str() + "; were defined outside of function defined using function");
        }
    } 
    

    SharedPtr<Scope> defScope = scope->detachScope(freeVarNames);

    if (!defScope){
        throw MerkError("defScope created in MethodDef::evaluate is null");
    }

    defScope->isCallableScope = true;
    defScope->owner = generateScopeOwner("MethodDef", name); 

    if (!body->getScope()){
        DEBUG_LOG(LogLevel::ERROR, "Body's Scope is null in FunctionDef::evaluate()");
        throw MerkError("Scope not present in FunctionDef::evaluate(scope)");
    }

    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    auto clonedBody = static_unique_ptr_cast<MethodBody>(std::move(clonedBodyBase));

    if (!clonedBody){
        throw MerkError("ClonedBody in MethodDef::evaluate is null");
    }
 
    if (!clonedBody->getScope()){
        DEBUG_LOG(LogLevel::ERROR, "Body's Scope is null in FunctionDef::evaluate()");
        throw MerkError("Scope not present in FunctionDef::evaluate(scope) of clonedBody");
    }

    DEBUG_LOG(LogLevel::DEBUG, "FunctionDef Defining Scope: ", scope->getScopeLevel());
    
    SharedPtr<Method> method = makeShared<Method>(name, parameters, std::move(clonedBody), getScope(), callType);
    if (!method){
        throw MerkError("Method created in MethodDef::evaluate is null");
    }
    method->setSubType(methodType);
    method->setScope(defScope);
    method->setCapturedScope(defScope);

    auto methodSig = method->toCallableSignature();

    getClassScope()->registerFunction(name, methodSig);

    DEBUG_LOG(LogLevel::ERROR, highlight("Registered Method: " + name, Colors::yellow), "into: ", getClassScope().get(), "Owner: ", getClassScope()->owner);

    DEBUG_LOG(LogLevel::ERROR, "Method", name, "registered to ", classScope->owner);
    // classScope->debugPrint();

    if (!defScope){
        DEBUG_FLOW_EXIT();
        throw MerkError("Defining Scope for FunctionDef::evaluate is null");
    }
    
    if (!method->getBody()){
        DEBUG_FLOW_EXIT();
        throw MerkError("Function body is null in FunctionDef::evaluate");
    }

    

    // Wrap it in a FunctionNode (for returning it as a value)
    MethodNode methodNode(method);

    DEBUG_FLOW_EXIT();
    return methodNode;
}

UniquePtr<BaseAST> MethodDef::clone() const {
    DEBUG_FLOW(FlowLevel::LOW);
    if (!body->getScope()){
        throw MerkError("No scope present in MethodDef::clone");
    }
    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    
    auto clonedBody = static_unique_ptr_cast<MethodBody>(std::move(clonedBodyBase));
    auto methodDef = makeUnique<MethodDef>(name, parameters.clone(), std::move(clonedBody), callType, getScope()->clone());
    String access = getMethodAccessor();

    methodDef->setMethodAccessor(access);

    DEBUG_FLOW_EXIT();
    return methodDef;
}

void MethodDef::setClassScope(SharedPtr<Scope> scope) {
    if (!scope) {
        DEBUG_LOG(LogLevel::ERROR, highlight("Setting MethodDef Scope Failed", Colors::yellow));

        throw MerkError("MethodDef new ClassScope is null");
    }
    classScope = scope;}



String MethodDef::toString() const {return astTypeToString(getAstType());}


void ClassDef::printAST(std::ostream& os, int indent) const {
    indent = printIndent(os, indent);
    debugLog(true, "ClassDef: ", name);
    body->printAST(os, indent);
}
void ClassCall::printAST(std::ostream& os, int indent) const {
    indent = printIndent(os, indent);
    for (const auto& arg : arguments){
        printIndent(os, indent);
        debugLog(true, arg->toString());
    }
}



ClassDef::~ClassDef() {
    // DEBUG_LOG(LogLevel::ERROR, "Destroying ClassDef:" + name);
}

MethodDef::~MethodDef() {
    // DEBUG_LOG(LogLevel::ERROR, "Destroying MethodDef:" + name);
}

ClassCall::~ClassCall() {
    // DEBUG_LOG(LogLevel::ERROR, "Destroying ClassCall:" + name);
}

MethodCall::~MethodCall() {
    DEBUG_LOG(LogLevel::ERROR, "Destroying MethodCall:" + name);
}

MethodBody::~MethodBody() {
    // DEBUG_LOG(LogLevel::ERROR, "Destroying MethodBody:");
}


ClassBody::~ClassBody() {
    // DEBUG_LOG(LogLevel::DEBUG, "Destroying Class Body");
}
