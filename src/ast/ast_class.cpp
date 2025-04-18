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

ClassBody::~ClassBody() {
    DEBUG_LOG(LogLevel::DEBUG, "Destroying Class Body");
}

UniquePtr<BaseAST> ClassBody::clone() const {
    UniquePtr<ClassBody> newBlock = makeUnique<ClassBody>(getScope());
    DEBUG_LOG(LogLevel::ERROR, "ClassBody::clone()", "Created a Unique ClassBody");
    std::unordered_map<String, SharedPtr<Scope>> methodScopes;

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
    
MethodDef::MethodDef(String name, ParamList parameters, UniquePtr<MethodBody> body, CallableType methodType, SharedPtr<Scope> scope)
: CallableDef(name, std::move(parameters), std::move(body), methodType, scope) {}

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
        for (const auto* node : allNodes) {
            std::cout << highlight("Node type: " + astTypeToString(node->getAstType()), Colors::red) << "\n";
        }
        ParamList& params = methodDef->getParameters();  // you might need a mutable accessor here
        String methodName = methodDef->getName();
        DEBUG_LOG(LogLevel::ERROR, "Params for", methodName, "=", params.toString());
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
            DEBUG_LOG(LogLevel::ERROR, "Removed implicit accessor from method:", methodName);
            removed.emplace_back(methodName);
        }

    }
    return removed;
}

void applyAccessorScopeFix(MethodDef* methodDef, SharedPtr<Scope> classScope, const String& accessor) {
    const auto& bodyChildren = methodDef->getBody()->getChildren();
    ASTUtils::traverse(bodyChildren, [&](BaseAST* node) {
        if (node->getAstType() == AstType::Chain) {
            Chain* chain = static_cast<Chain*>(node);
            const auto& elems = chain->getElements();
            if (!elems.empty() && elems[0].name == accessor) {
                chain->setSecondaryScope(classScope);
                chain->setResolutionStartIndex(1);
                chain->setResolutionMode(ResolutionMode::ClassInstance);
            }
        }
    }, true, false);
}


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


    

    removeMethodAccessors(clonedBody);
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
    for (const auto& methodNode : getBody()->getChildren()) {
        if (methodNode->getAstType() == AstType::ClassMethodDef){
            auto* methodDef = static_cast<MethodDef*>(methodNode.get());
            applyAccessorScopeFix(methodDef, cls->getClassScope(), accessor);
            DEBUG_LOG(LogLevel::ERROR, "Temporarily Skipping Accessor Scope Fix");
        }
        
    }
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

    // Will need to update Methods here to have a clean scope ==============================================================
    // for (auto& child : )
    SharedPtr<ClassInstance> instance = classSig->instantiate(argValues);

    // Create the instance
    // for (const auto& method : instance->getBody()->getChildren()) {
    //     if (method->getAstType() == AstType::ClassMethodDef) {
    //         auto* original = static_cast<MethodDef*>(method.get());
    
    //         // Clone method and scope
    //         UniquePtr<MethodDef> clonedMethod = original->cloneWithScope(instanceScope);
    
    //         // Register the method
    //         SharedPtr<CallableSignature> signature = clonedMethod->evaluate(instanceScope).asCallableSignature();
    //         instanceScope->registerFunction(clonedMethod->getName(), signature);
    //     }
    // }
    
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

    SharedPtr<Scope> defScope = scope->detachScope(freeVarNames); //creates a clean scope with cloned variables.
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
    auto methodDef = makeUnique<MethodDef>(name, parameters.clone(), std::move(clonedBody), callType, getScope()->clone());
    String access = getMethodAccessor();

    methodDef->setMethodAccessor(access);

    DEBUG_FLOW_EXIT();
    return methodDef;
}

void MethodDef::setClassScope(SharedPtr<Scope> scope) {classScope = scope;}



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

























// void AttributeHandler::setFullName(String fullNameVal) {
//     fullName = fullNameVal;
//     size_t start = 0;

//     auto [pos, delim] = findDelimiter(fullNameVal.substr(start));
//     if (pos != String::npos){
//         setAccessor(fullNameVal.substr(start, pos));
//         setDelimiter(delim);
//         setVarName(fullNameVal.substr(pos+delim.size()));
//     }

//     else {
//         throw MerkError("There is no matching syntax for " + fullNameVal);
//     }
// }  
// void AttributeHandler::setAccessor(String accessorVal) {accessor = accessorVal;}
// void AttributeHandler::setDelimiter(String delimiterVal) {delimeter = delimiterVal;}
// void AttributeHandler::setVarName(String varNameVal) {varName = varNameVal;}

// String AttributeHandler::getFullName() const {return fullName;}
// String AttributeHandler::getDelimiter() const {return delimeter;}
// String AttributeHandler::getAccessor() const {return accessor;}
// String AttributeHandler::getVarName() const {return varName;}    


// AttributeDeclaration::AttributeDeclaration(UniquePtr<VariableDeclaration> varDec)
//     : VariableDeclaration(std::move(varDec)) {
//     setFullName(getName());
//     varDec.release();
// }
// AttributeAssignment::AttributeAssignment(UniquePtr<VariableAssignment> varAssign)
//     : VariableAssignment(std::move(varAssign)) {
//     setFullName(getName());
//     varAssign.release();
// }
// AttributeReference::AttributeReference(UniquePtr<VariableReference> varRef)
//     : VariableReference(std::move(varRef)) {
//     setFullName(getName());
//     // varRef.release();
// }




// Node AttributeDeclaration::evaluate(SharedPtr<Scope> scope) const {
//     DEBUG_FLOW(FlowLevel::HIGH);

//     validateScope(scope, "AttributeDeclaration");

//     Node value = getExpression()->evaluate(scope);
//     DEBUG_LOG(LogLevel::ERROR, "Value for AttributeDeclaration::evaluate is: ", value);
//     // DEBUG_LOG(LogLevel::TRACE, highlight("[AttributeDeclaration::evaluate]", Colors::orange), valueExpression->toString());

//     // Node val = Evaluator::evaluateVariableDeclaration(valueExpression.get(), variable, scope);

//     // DEBUG_FLOW_EXIT();
//     // return val;

//     // DEBUG_FLOW(FlowLevel::HIGH);
    
//     // validateScope(scope, "AttributeDeclaration");

//     // Node value = getExpression()->evaluate(scope);  // evaluate right-hand side

//     // // Resolve left-hand chain (inherited from VariableDeclaration)
//     // const String& fullAttrName = getFullName();  // e.g., "self.x"
//     // auto chain = std::make_unique<Chain>(scope);  // Create chain manually or reuse parser output

//     // // Option 1: You stored chain somewhere? Great, call assign:
//     // // chain->assign(scope, value);

//     // // Option 2: Reconstruct from your `AttributeHandler` metadata
//     // ChainElement accessorElement = { getAccessor(), IdentifierType::Variable, "." };
//     // ChainElement propertyElement = { getVarName(), IdentifierType::Variable, "" };

//     // chain->addElement(accessorElement);
//     // chain->addElement(propertyElement);
//     // chain->setSecondaryScope(scope);  // or use clsScope if needed
//     // chain->setResolutionStartIndex(1);  // skip "self" during assignment
//     // chain->setResolutionMode(ResolutionMode::ClassInstance);

//     // chain->assign(scope, value);

//     // DEBUG_FLOW_EXIT();
//     // return value;

//     // DEBUG_FLOW(FlowLevel::HIGH);
//     // validateScope(scope, "AttributeDeclaration");

//     // // 1. Evaluate the value expression (RHS)
//     // Node val = valueExpression->evaluate(scope);

//     // // 2. Extract the attribute chain from the `variable` (inherited from VariableDeclaration)
//     // const Chain* chain = dynamic_cast<const Chain*>(variable.get());
//     // if (!chain) {
//     //     throw MerkError("AttributeDeclaration expects a Chain (e.g., self.x) on the left-hand side.");
//     // }

//     // // 3. Assign to chain
//     // chain->assign(scope, val);

//     // DEBUG_LOG(LogLevel::TRACE, "[AttributeDeclaration::evaluate] Assigned:", chain->toString());

//     // DEBUG_FLOW_EXIT();
//     // return val;
//     return value;
// }
// Node AttributeAssignment::evaluate(SharedPtr<Scope> scope) const {
//     DEBUG_FLOW(FlowLevel::HIGH);

//     validateScope(scope, "VariableAssignment::evaluate", getVarName());

//     Node val = Evaluator::evaluateVariableAssignment(getVarName(), getExpression().get(), scope);

//     DEBUG_FLOW_EXIT();
//     return val;
// }
// Node AttributeReference::evaluate(SharedPtr<Scope> scope) const {
//     DEBUG_FLOW(FlowLevel::HIGH);

//     validateScope(scope, "VariableReference::evluate", "Name = " + getVarName());

//     VarNode& varRef = Evaluator::evaluateVariableReference(getVarName(), scope); // ✅ Preserve reference
//     Node val = Node(varRef);
//     DEBUG_FLOW_EXIT();
//     return val;
// };

// UniquePtr<BaseAST> AttributeDeclaration::clone() const {
//     UniquePtr<ASTStatement> valueInfo = nullptr;
//     if (valueExpression){
//         auto valueInfoBase = valueExpression->clone();
//         valueInfo = static_unique_ptr_cast<ASTStatement>(std::move(valueInfoBase));
//     }
//     auto varDec = makeUnique<VariableDeclaration>(name, variable, getScope(), typeTag, std::move(valueInfo));
//     return makeUnique<AttributeDeclaration>(std::move(varDec));
// }
// UniquePtr<BaseAST> AttributeReference::clone() const {
//     auto varRef = makeUnique<VariableReference>(getName(), getScope());
//     return makeUnique<AttributeReference>(std::move(varRef));
// } 
// UniquePtr<BaseAST> AttributeAssignment::clone() const {
//     UniquePtr<BaseAST> clonedValBase = getExpression()->clone();
//     auto clonedVal = static_unique_ptr_cast<ASTStatement>(std::move(clonedValBase));
//     auto varAssign = makeUnique<VariableAssignment>(getName(), std::move(clonedVal), getScope());
//     return makeUnique<AttributeAssignment>(std::move(varAssign));
// } 









// // Attributes
// String AttributeDeclaration::toString() const {
//     return getAstTypeAsString() + "(accessor=" + getAccessor() + ", name=" + getVarName() + ", delimiter" + getDelimiter() + 
//         ", variable=" + variable.toString() +
//         ", scope=" + std::to_string(getScope()->getScopeLevel()) + ")";
// }
// String AttributeAssignment::toString() const {
//     return getAstTypeAsString() + "(accessor=" + getAccessor() + ", name=" + getVarName() + ", delimiter" + getDelimiter();

// }
// String AttributeReference::toString() const {
//     return getAstTypeAsString() + "(accessor=" + getAccessor() + ", name=" + getVarName() + ", delimiter" + getDelimiter() +
//     ", scope=" + std::to_string(getScope()->getScopeLevel()) + ")";
// }


// void AttributeDeclaration::printAST(std::ostream& os, int indent) const {
//     DEBUG_FLOW(FlowLevel::VERY_LOW);

//     indent = printIndent(os, indent);
//     debugLog(true,  highlight(getAstTypeAsString(), Colors::cyan));
//     printIndent(os, indent);
//     debugLog(true, "(Attribute =", getFullName(), ", scope =", getScope()->getScopeLevel(), ")");

//     valueExpression->printAST(os, indent);

//     DEBUG_FLOW_EXIT();
// };
// void AttributeAssignment::printAST(std::ostream& os, int indent) const {
//     DEBUG_FLOW(FlowLevel::VERY_LOW);

//     indent = printIndent(os, indent);
//     debugLog(true,  highlight(getAstTypeAsString(), Colors::cyan));
//     printIndent(os, indent);
//     debugLog(true, "(Attribute =", getFullName(), ", scope =", getScope()->getScopeLevel(), ")");

//     if (getExpression()) {
//         getExpression()->printAST(os, indent);
//     }

//     DEBUG_FLOW_EXIT();
// };
// void AttributeReference::printAST(std::ostream& os, int indent) const {
//     DEBUG_FLOW(FlowLevel::VERY_LOW);

//     indent = printIndent(os, indent);
//     debugLog(true,  highlight(getAstTypeAsString(), Colors::cyan));
//     printIndent(os, indent);
//     debugLog(true, "(Attribute =", getFullName(), ", scope =", getScope()->getScopeLevel(), ")");


//     DEBUG_FLOW_EXIT();
// };
