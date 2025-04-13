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
#include "ast/exceptions.h"



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
    // varRef.release();
}


ClassBody::ClassBody(UniquePtr<CodeBlock>&& block)
    : CallableBody(block->getScope()) {
    this->children = std::move(block->children);
    block.reset();
}
ClassBody::ClassBody(SharedPtr<Scope> scope)
    : CallableBody(scope) {}

ClassBody::~ClassBody() {
    DEBUG_LOG(LogLevel::DEBUG, "Destroying Class Body");
}

UniquePtr<BaseAST> ClassBody::clone() const {
    UniquePtr<ClassBody> newBlock = makeUnique<ClassBody>(getScope());
    DEBUG_LOG(LogLevel::ERROR, "ClassBody::clone()", "Created a Unique ClassBody");
     
    for (const auto &child : children) {
        DEBUG_LOG(LogLevel::ERROR, "ClassBody::clone()", "current child", child->toString());

        if (!child){
            DEBUG_LOG(LogLevel::ERROR, "ClassBody::clone()", "Null child encountered in ClassBody::clone()");
        }

        newBlock->addChild(child->clone());
        DEBUG_LOG(LogLevel::ERROR, "ClassBody::clone()", "Added Child to ClassBody", child->getAstTypeAsString());

    }
    DEBUG_LOG(LogLevel::ERROR, "ClassBody::clone()", "Returning newClassBody");
    return newBlock;
}


MethodBody::MethodBody(UniquePtr<CodeBlock>&& body) : CallableBody(std::move(body)){};
MethodBody::MethodBody(SharedPtr<Scope> scope) : CallableBody(scope) {}
MethodBody::MethodBody(UniquePtr<CallableBody>* body) : CallableBody(std::move(body)) {}
Node MethodBody::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);

    if (!scope) {
        throw MerkError("MethodBody::evaluate: scope is null");
    }

    // Execute the children in the given scope
    Node last;
    try {
        for (const auto& child : children) {
            if (!child) continue;
            last = child->evaluate(scope);
        }
    } catch (const ReturnException& e) {
        DEBUG_FLOW_EXIT();
        return e.getValue();
    }

    DEBUG_FLOW_EXIT();
    return last;
}

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
    
// MethodDef::MethodDef(String name, ParamList parameters, UniquePtr<MethodBody> body, SharedPtr<Scope> scope)
// : CallableDef(name, std::move(parameters), std::move(body), CallableType::METHOD, scope) {}
MethodDef::MethodDef(String name, ParamList parameters, UniquePtr<MethodBody> body, CallableType methodType, SharedPtr<Scope> scope)
: CallableDef(name, std::move(parameters), std::move(body), methodType, scope) {
    // this->methodType = ;
}
MethodDef::MethodDef(UniquePtr<FunctionDef> funcDef)
    : CallableDef(funcDef->getName(), funcDef->getParameters(), makeUnique<MethodBody>(std::move(funcDef->getBody())), CallableType::METHOD, funcDef->getScope()) 
{
    funcDef.release();
}


MethodCall::MethodCall(String name, Vector<UniquePtr<ASTStatement>> arguments, SharedPtr<Scope> scope)
    : CallableCall(name, std::move(arguments), scope) {
}

void MethodDef::setMethodAccessor(String& accessorName) {accessor = accessorName;}
String MethodDef::getMethodAccessor() const {return accessor;}



Node ClassDef::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);
    
    if (!scope) {
        throw MerkError("No Scope Was Found in ClassDef::evaluate()");
    }

    FreeVars freeVarNames = body->collectFreeVariables();
    DEBUG_LOG(LogLevel::ERROR, "ClassDef::evaluate(), got freeVarNames");

    // Clone the class body AST
    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    auto clonedBody = static_unique_ptr_cast<ClassBody>(std::move(clonedBodyBase));

    if (!clonedBody) {
        throw MerkError("ClassDef::evaluate(): clonedBody is null");
    }

    DEBUG_LOG(LogLevel::ERROR, "ClassDef::evaluate(), casted the cloned body");

    if (!clonedBody->getScope()) {
        DEBUG_FLOW_EXIT();
        throw MerkError("Scope not present in ClassDef::evaluate");
    }

    // Create the ClassBase object
    SharedPtr<ClassBase> cls = std::make_shared<ClassBase>(
        name,
        accessor,
        std::move(clonedBody),
        scope
    );

    cls->setParameters(parameters.clone());
    DEBUG_LOG(LogLevel::ERROR, "ClassDef::evaluate(), created Shared ClassBase scope");

    // Capture Free Variables
    SharedPtr<Scope> classDefScope = scope->detachScope(freeVarNames);
    if (!classDefScope) {
        throw MerkError("Failed to create detachedScope in ClassDef::evaluate");
    }

    DEBUG_LOG(LogLevel::ERROR, "ClassDef::evaluate(), detached scope");

    classDefScope->isCallableScope = true;
    cls->setCapturedScope(classDefScope);

    if (!cls->getCapturedScope()) {
        throw MerkError("CapturedScope was not set correctly on the ClassBase.");
    }

    // Create the inner class scope
    auto classScope = classDefScope->createChildScope();
    if (!classScope) {
        throw MerkError("createChildScope returned null");
    }

    cls->setClassScope(classScope);
    DEBUG_LOG(LogLevel::ERROR, "Class Scope level: ", cls->getClassScope()->getScopeLevel());
    DEBUG_LOG(LogLevel::ERROR, "ClassDef::evaluate(), set the class Scope");

    // Evaluate class body children (methods, attributes, etc.)
    for (const auto& child : cls->getBody()->getChildren()) {
        std::cerr << "Evaluating AST Child: " << child->getAstTypeAsString() << "\n";

        switch (child->getAstType()) {
            case AstType::ClassMethodDef:
            case AstType::VariableDeclaration:
            case AstType::VariableAssignment:
            case AstType::ParameterAssignment:
            case AstType::ClassDefinition:
                child->evaluate(cls->getClassScope());
                break;

            default:
                std::cerr << "Unhandled AST type in ClassBody: " << child->getAstTypeAsString() << std::endl;
                throw MerkError("Unexpected AST statement in ClassBody: " + child->getAstTypeAsString());
        }
    }

    // Ensure the class has a construct method
    if (!cls->getClassScope()->hasFunction("construct")) {
        throw MerkError("Class '" + name + "' must implement a 'construct' method.");
    }

    scope->registerClass(name, cls);

    DEBUG_FLOW_EXIT();
    return ClassNode(cls);
}


Node ClassBody::evaluate(SharedPtr<Scope> scope) const {
    (void)scope;
    DEBUG_FLOW();
    // Possibly validate the structure but do nothing
    DEBUG_FLOW_EXIT();
    return Node();  // or nullptr
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
    SharedPtr<Scope> captured = instance->getCapturedScope()->clone();   // cloned captured scope
    SharedPtr<Scope> instanceScope = instance->getInstanceScope()->clone(); // cloned class scope
    captured->appendChildScope(instanceScope);

    scope->appendChildScope(captured);


    DEBUG_FLOW_EXIT();
    return ClassInstanceNode(instance);
}

Node MethodDef::evaluate(SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::HIGH);
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
                oss << highlight("'", Colors::yellow) << highlight(var, Colors::purple) << highlight("'", Colors::yellow) << " ";
            }
            throw MerkError("The Following Vars: " + oss.str() + "; were defined outside of function defined using function");
        }
    } 

    SharedPtr<Scope> defScope = scope->detachScope(freeVarNames);
    defScope->isCallableScope = true;

    if (!body->getScope()){
        DEBUG_LOG(LogLevel::ERROR, "Body's Scope is null in FunctionDef::evaluate()");
        throw MerkError("Scope not present in FunctionDef::evaluate(scope)");
    }

    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    auto clonedBody = static_unique_ptr_cast<MethodBody>(std::move(clonedBodyBase));
 
    if (!clonedBody->getScope()){
        DEBUG_LOG(LogLevel::ERROR, "Body's Scope is null in FunctionDef::evaluate()");
        throw MerkError("Scope not present in FunctionDef::evaluate(scope) of clonedBody");
    }

    DEBUG_LOG(LogLevel::DEBUG, "FunctionDef Defining Scope: ", scope->getScopeLevel());
    
    
    SharedPtr<Method> method = std::make_shared<Method>(name, parameters, std::move(clonedBody), getScope(), callType);
    method->setSubType(methodType);
    
    auto methodSig = method->toCallableSignature();

    scope->registerFunction(name, methodSig);

    if (!defScope){
        DEBUG_FLOW_EXIT();
        throw MerkError("Defining Scope for FunctionDef::evaluate is null");
    }
    
    if (!method->getBody()){
        DEBUG_FLOW_EXIT();
        throw MerkError("Function body is null in FunctionDef::evaluate");
    }

    method->setCapturedScope(defScope);

    // Wrap it in a FunctionNode (for returning it as a value)
    MethodNode methodNode(method);

    DEBUG_FLOW_EXIT();
    return methodNode;
}


UniquePtr<BaseAST> MethodDef::clone() const {
    DEBUG_FLOW(FlowLevel::LOW);

    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    auto clonedBody = static_unique_ptr_cast<MethodBody>(std::move(clonedBodyBase));
    // MethodDef(name, parameters, std::move(clonedBody), callType, getScope());
    auto methodDef = makeUnique<MethodDef>(name, parameters.clone(), std::move(clonedBody), callType, getScope());
    String access = getMethodAccessor();

    methodDef->setMethodAccessor(access);

    DEBUG_FLOW_EXIT();
    return methodDef;
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
