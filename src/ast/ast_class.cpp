#include <iostream>
// #include <regex>
#include "core/types.h"
#include "utilities/debugger.h"
#include "utilities/helper_functions.h"
#include "core/scope.h"

#include "core/evaluator.h"
#include "ast/ast.h"

#include "ast/ast_callable.h"
#include "core/classes/method.h" 
#include "ast/ast_class.h"




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

void AttributeHandler::setFullName(String fullNameVal) {
    fullName = fullNameVal;
    size_t start = 0;

    auto [pos, delim] = findDelimiter(fullNameVal.substr(start));
    if (pos != String::npos){
        setAccessor(fullNameVal.substr(start, pos));
        setDelimiter(delim);
        setVarName(fullNameVal.substr(pos+delim.size()));
    }

    else {
        throw MerkError("There is no matching syntax for " + fullNameVal);
    }
}  
void AttributeHandler::setAccessor(String accessorVal) {accessor = accessorVal;}
void AttributeHandler::setDelimiter(String delimiterVal) {delimeter = delimiterVal;}
void AttributeHandler::setVarName(String varNameVal) {varName = varNameVal;}

String AttributeHandler::getFullName() const {return fullName;}
String AttributeHandler::getDelimiter() const {return delimeter;}
String AttributeHandler::getAccessor() const {return accessor;}
String AttributeHandler::getVarName() const {return varName;}    


AttributeDeclaration::AttributeDeclaration(UniquePtr<VariableDeclaration> varDec)
    : VariableDeclaration(std::move(varDec)) {
    setFullName(getName());
    varDec.release();
}
AttributeAssignment::AttributeAssignment(UniquePtr<VariableAssignment> varAssign)
    : VariableAssignment(std::move(varAssign)) {
    setFullName(getName());
    varAssign.release();
}
AttributeReference::AttributeReference(UniquePtr<VariableReference> varRef)
    : VariableReference(std::move(varRef)) {
    setFullName(getName());
    varRef.release();
}

ClassCall::ClassCall(String name, Vector<UniquePtr<ASTStatement>> arguments, SharedPtr<Scope> scope)
    : CallableCall(name, std::move(arguments), scope) {
    branch = "Classical";
}
ClassDef::ClassDef(String name, ParamList parameters, UniquePtr<ClassBody> body, String accessor, SharedPtr<Scope> scope)
    : CallableDef(name, std::move(parameters), std::move(body), CallableType::CLASS, scope), accessor(accessor) {}

void ClassDef::setClassAccessor(String accessorName){accessor = accessorName;}
String ClassDef::getClassAccessor() {return accessor;}
    
MethodDef::MethodDef(String name, ParamList parameters, UniquePtr<MethodBody> body, SharedPtr<Scope> scope)
: CallableDef(name, std::move(parameters), std::move(body), CallableType::METHOD, scope) {
}
MethodDef::MethodDef(String name, ParamList parameters, UniquePtr<MethodBody> body, CallableType methodType, SharedPtr<Scope> scope)
: CallableDef(name, std::move(parameters), std::move(body), CallableType::METHOD, scope) {
    methodType = methodType;
}




String MethodDef::toString() const {return astTypeToString(getAstType());}
MethodDef::MethodDef(UniquePtr<FunctionDef> funcDef)
    : CallableDef(funcDef->getName(), funcDef->getParameters(), makeUnique<MethodBody>(std::move(funcDef->getBody())), CallableType::METHOD, funcDef->getScope()) 
{
    funcDef.release();
}
MethodCall::MethodCall(String name, Vector<UniquePtr<ASTStatement>> arguments, SharedPtr<Scope> scope)
    : CallableCall(name, std::move(arguments), scope) {
}
void MethodDef::setMethodAccessor(String& accessorName) {accessor = accessorName;}
String MethodDef::getMethodAccessor() {return accessor;}



// Attributes
String AttributeDeclaration::toString() const {
    return getAstTypeAsString() + "(accessor=" + getAccessor() + ", name=" + getVarName() + ", delimiter" + getDelimiter() + 
        ", variable=" + variable.toString() +
        ", scope=" + std::to_string(getScope()->getScopeLevel()) + ")";
}
String AttributeAssignment::toString() const {
    return getAstTypeAsString() + "(accessor=" + getAccessor() + ", name=" + getVarName() + ", delimiter" + getDelimiter();

}
String AttributeReference::toString() const {
    return getAstTypeAsString() + "(accessor=" + getAccessor() + ", name=" + getVarName() + ", delimiter" + getDelimiter() +
    ", scope=" + std::to_string(getScope()->getScopeLevel()) + ")";
}


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
void MethodDef::printAST(std::ostream& os, int indent) const {
    indent = printIndent(os, indent);
    debugLog(true, "MethodDef: ", name);
    body->printAST(os, indent);
}
void AttributeDeclaration::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    indent = printIndent(os, indent);
    debugLog(true,  highlight(getAstTypeAsString(), Colors::cyan), "(Chain =", getFullName(), ", scope =", getScope()->getScopeLevel(), ")");
    valueExpression->printAST(os, indent);

    DEBUG_FLOW_EXIT();
};
void AttributeAssignment::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    indent = printIndent(os, indent);
    debugLog(true,  highlight(getAstTypeAsString(), Colors::cyan), "(Chain =", getFullName(), ", scope =", getScope()->getScopeLevel(), ")");

    if (getExpression()) {
        getExpression()->printAST(os, indent);
    }

    DEBUG_FLOW_EXIT();
};
void AttributeReference::printAST(std::ostream& os, int indent) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    printIndent(os, indent);
    debugLog(true,  highlight(getAstTypeAsString(), Colors::cyan), "(Chain =", getFullName(), ", scope =", getScope()->getScopeLevel(), ")");

    DEBUG_FLOW_EXIT();
};


Node ClassDef::evaluate(SharedPtr<Scope> scope) const{
    
    DEBUG_FLOW(FlowLevel::HIGH);
    if (!scope){
        throw MerkError("No Scope Was Found in ClassDef::evaluate()");
    }
    // Collect free variables from the class body (attributes and methods)
    FreeVars freeVarNames = body->collectFreeVariables();


    DEBUG_LOG(LogLevel::ERROR, "ClassDef::evaluate(), got freeVarNames");

    // This scope will capture any free variables from the class body.
    SharedPtr<Scope> classDefScope = scope->detachScope(freeVarNames);

    if (!classDefScope){
        throw MerkError("Failed to create detachedScope in ClassDef::evaluate");
    }

    DEBUG_LOG(LogLevel::ERROR, "ClassDef::evaluate(), detached scope");

    classDefScope->isCallableScope = true;

    // Clone the class body AST.
    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    DEBUG_LOG(LogLevel::ERROR, "ClassDef::evaluate(), cloned the body");

    // Cast it to a ClassBody type
    auto clonedBody = static_unique_ptr_cast<ClassBody>(std::move(clonedBodyBase));
    

    if (!clonedBody){
        throw MerkError("ClassDef::evaluate(): clonedBody is null");
    }
    DEBUG_LOG(LogLevel::ERROR, "ClassDef::evaluate(), casted the cloned body");

    if (!clonedBody->getScope()){
        DEBUG_FLOW_EXIT();
        throw MerkError("Scope not present in ClassDef::evaluate");
    }


    
    // Create a new Class object.
    SharedPtr<ClassBase> cls = std::make_shared<ClassBase>(
        name,           // The class name.
        accessor,
        std::move(clonedBody),
        scope
    );
    // ClassBase(name, accessor, )

    cls->setParameters(parameters.clone());

    
    DEBUG_LOG(LogLevel::ERROR, "ClassDef::evaluate(), created Shared ClassBase scope");
    
    // Set the captured scope on the class so that methods later can refer to it.
    cls->setCapturedScope(classDefScope);
    cls->setClassScope(classDefScope->createChildScope());
    if (!cls->getCapturedScope()){
        throw MerkError("CapturedScope was not set correctly on the ClassBase.");
    }
    // Determine Method Details and makde decisions about handling.
    // for (auto& method : body->getMutableChildren()) {
    //     if (method->getAstType() == AstType::ClassMethodDef){
    //         auto castedChild = static_unique_ptr_cast<MethodDef>(std::move(method));
    //         castedChild->getName();
    //         method = castedChild;
    //     }
    // }
    // for (auto method :)
    // Register the class in the current scope's class registry.


    
    for (const auto& child : cls->getBody()->getChildren()) {
        if (child->getAstType() == AstType::ClassMethodDef) {
            child->evaluate(cls->getClassScope());  // Ensure it gets registered
        }
        // You might want to evaluate attributes or nested logic as well
    }
    scope->registerClass(name, cls);

    auto classTest = scope->getClass(name);
    if (!classTest){
        throw MerkError("Class Was Not Registered in ClassDef::evaluate()");
    }
     
    DEBUG_LOG(LogLevel::ERROR, "ClassDef::evaluate(), registered class in scope and ClassRegistry");

    
    DEBUG_LOG(LogLevel::ERROR, "ClassDef::evaluate(), set the captured Scope");
    if (!cls->getCapturedScope()){
        throw MerkError("Captured Scope is null in ClassDef::evaluate()");
    }

    // Wrap the class object in a ClassNode (so that it can be treated as a value)
    ClassNode classNode(cls);
    DEBUG_FLOW_EXIT();
    return classNode;
}



Node ClassCall::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);

    Vector<Node> argValues = handleArgs(scope);

    auto classOpt = scope->getClass(name);
    if (!classOpt.has_value()) {
        throw MerkError("Class not found: " + name);
    }

    // auto classSig = std::dynamic_pointer_cast<ClassSignature>(classOpt->get());
    SharedPtr<CallableSignature> callableSig = classOpt.value().get();
    DEBUG_LOG(LogLevel::DEBUG, "The call is of type: ", callableTypeAsString(callableSig->getCallableType()));
    SharedPtr<ClassSignature> classSig = std::dynamic_pointer_cast<ClassSignature>(callableSig);

    // std::shared_ptr<ClassSignature> classSig = std::dynamic_pointer_cast<ClassSignature>(classOpt.value().get());

    if (!classSig) {
        throw MerkError("Could not cast CallableSignature to ClassSignature.");
    }

    // Create the instance
    SharedPtr<ClassInstance> instance = classSig->instantiate(argValues);
    SharedPtr<Scope> captured = instance->getCapturedScope();   // cloned captured scope
    SharedPtr<Scope> instanceScope = instance->getInstanceScope(); // cloned class scope

    scope->appendChildScope(captured);
    captured->appendChildScope(instanceScope);

    DEBUG_FLOW_EXIT();
    return ClassInstanceNode(instance);
}



Node MethodDef::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);

    // Clone the method body for this specific instantiation
    auto clonedBodyBase = body->clone();
    auto clonedBody = static_unique_ptr_cast<MethodBody>(std::move(clonedBodyBase));

    if (!clonedBody) {
        throw MerkError("MethodDef::evaluate: failed to clone method body");
    }

    // Construct the Method object
    SharedPtr<Method> method = std::make_shared<Method>(
        name,
        parameters.clone(),
        std::move(clonedBody),
        scope,
        false
    );
    
    // SharedPtr<Method> method = std::make_shared<Method>(
    //     getName(),
    //     clonedParams,
    //     std::move(clonedMethodBody),
    //     getCapturedScope(),
    //     requiresReturn
    // );

    method->setAccessor(accessor);

    // Register the method into the provided class scope
    scope->registerFunction(name, method->toCallableSignature());

    DEBUG_LOG(LogLevel::DEBUG, "Method registered: ", name);
    DEBUG_FLOW_EXIT();
    return Node();  // Evaluation doesn't produce a runtime value
}


Node AttributeDeclaration::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);

    validateScope(scope, "VariableDeclaration");
    DEBUG_LOG(LogLevel::TRACE, highlight("[VariableDeclaration::evaluate]", Colors::orange), valueExpression->toString());

    Node val = Evaluator::evaluateVariableDeclaration(valueExpression.get(), variable, scope);

    DEBUG_FLOW_EXIT();
    return val;
}
Node AttributeAssignment::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);

    validateScope(scope, "VariableAssignment::evaluate", getVarName());

    Node val = Evaluator::evaluateVariableAssignment(getVarName(), getExpression().get(), scope);

    DEBUG_FLOW_EXIT();
    return val;
}
Node AttributeReference::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);

    validateScope(scope, "VariableReference::evluate", "Name = " + getVarName());

    VarNode& varRef = Evaluator::evaluateVariableReference(getVarName(), scope); // ✅ Preserve reference
    Node val = Node(varRef);
    DEBUG_FLOW_EXIT();
    return val;
};

UniquePtr<BaseAST> AttributeDeclaration::clone() const {
    UniquePtr<ASTStatement> valueInfo = nullptr;
    if (valueExpression){
        auto valueInfoBase = valueExpression->clone();
        valueInfo = static_unique_ptr_cast<ASTStatement>(std::move(valueInfoBase));
    }
    auto varDec = makeUnique<VariableDeclaration>(name, variable, getScope(), typeTag, std::move(valueInfo));
    return makeUnique<AttributeDeclaration>(std::move(varDec));
}
UniquePtr<BaseAST> AttributeReference::clone() const {
    auto varRef = makeUnique<VariableReference>(getName(), getScope());
    return makeUnique<AttributeReference>(std::move(varRef));
} 
UniquePtr<BaseAST> AttributeAssignment::clone() const {
    UniquePtr<BaseAST> clonedValBase = getExpression()->clone();
    auto clonedVal = static_unique_ptr_cast<ASTStatement>(std::move(clonedValBase));
    auto varAssign = makeUnique<VariableAssignment>(getName(), std::move(clonedVal), getScope());
    return makeUnique<AttributeAssignment>(std::move(varAssign));
} 








