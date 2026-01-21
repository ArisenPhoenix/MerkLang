#include "core/evaluators/Evaluator.hpp"
#include "core/errors.h"
#include "utilities/debugger.h"
#include "ast/Ast.hpp"
#include "ast/AstChain.hpp"
#include "ast/AstCallable.hpp"
#include "ast/AstClass.hpp"
#include "ast/AstMethod.hpp"
#include "ast/AstChain.hpp"
#include "core/Scope.hpp"
#include "ast/helpers.h"
#include "ast/ast_validate.h"
#include "core/callables/Callable.hpp"
#include "core/callables/classes/Method.hpp"
#include "core/callables/functions/Function.hpp"
#include "core/node/BoundArgs.hpp"
#include "core/node/ArgumentNode.hpp"

namespace Evaluator {
Node evaluateMethodBody(Vector<UniquePtr<BaseAST>>& children, SharedPtr<Scope> methodScope, SharedPtr<ClassInstanceNode> instanceNode){
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!instanceNode){throw MerkError("Evaluator::evaluateMethod has no instanceNode");}
    Node lastValue;
    for (const auto& child : children) {
        
        if (child->getAstType() == AstType::ChainOperation) {
            auto chain = static_unique_ptr_cast<ChainOperation>(child->clone());
            for (auto& elem: chain->getLeft()->getElements()) {
                if (elem.object->getAstType() == AstType::ClassMethodCall) {
                    DEBUG_LOG(LogLevel::TRACE, highlight("Evaluating a nested method call ========================================================================================", Colors::bg_bright_red));

                    auto current = static_unique_ptr_cast<MethodCall>(elem.object->clone());                        
                }
            }

        }

        lastValue = child->evaluate(methodScope, instanceNode);
        
    }

    DEBUG_FLOW_EXIT();
    return lastValue; // Return the last evaluated value
}

Node evaluateClassBody(SharedPtr<Scope> classCapturedScope, SharedPtr<Scope> classScope, SharedPtr<Scope> generatedScope, String accessor, Vector<UniquePtr<BaseAST>>& children, SharedPtr<ClassInstanceNode> instanceNode) {
    MARK_UNUSED_MULTI(classCapturedScope, instanceNode);
    DEBUG_FLOW(FlowLevel::PERMISSIVE);

    if (!classCapturedScope) {throw MerkError("Class Captured Scope Was Not Set On Body");}

    if (!classScope) {throw MerkError("Class Scope Was Not Set On Body");}

    if (accessor.empty()) {throw MerkError("Accessor Was Not Set On Body");}


    generatedScope->owner = generateScopeOwner("Class", "Body");
    DEBUG_LOG(LogLevel::TRACE, highlight("Moving to Apply Accessor Scope Fix", Colors::yellow));
    Vector<String> methods;
    for (const auto& child : children) {
        switch (child->getAstType()) {
            case AstType::VariableDeclaration:                // protected variables stored in captured scope, 
                child->evaluate(classScope->getParent());
                break;
            case AstType::ClassMethodDef:
                if (child->getAstType() == AstType::ClassMethodDef){
                    auto* methodDef = static_cast<MethodDef*>(child.get());
                    DEBUG_LOG(LogLevel::TRACE, "CREATING METHOD CALL SCOPE");
                    auto methodScope = classScope->createChildScope();
                    DEBUG_LOG(LogLevel::TRACE, "CREATED METHOD CALL SCOPE");
                    if (!methodScope){throw MerkError("generated methodscope is null for method in ClassDef::evaluate");} 

                    methodDef->setClassScope(classScope);
                    if (methodDef->getClassScope().get() != classScope.get()) {throw MerkError("method class Scope is not the same as cls->classScope");}
 
                    Vector<Chain*> nonStaticElements = applyAccessorScopeFix(methodDef, classScope, accessor);
                    
                    stripImplicitAccessor(methodDef, accessor);
                    methods.emplace_back(methodDef->getName());
                    // methodDef->setNonStaticElements(nonStaticElements);
                    methodDef->evaluate(classScope, instanceNode);
                } 
                break;

            // case AstType::VariableAssignment:
            // case AstType::ParameterAssignment:
            case AstType::ClassDefinition:
                // Will be handled later
                break;

            default:
                // child->evaluate(classScope);
                DEBUG_LOG(LogLevel::ERROR, "Unhandled AST type in ClassBody:", child->getAstTypeAsString());
                throw MerkError("Unexpected AST statement in ClassBody: " + child->getAstTypeAsString());
        }
    }

    DEBUG_LOG(LogLevel::TRACE, highlight("Finished Applying Accessor Scope Fix", Colors::yellow));
    DEBUG_FLOW_EXIT();
    return Node();
}

Node evaluateMethodDef(
    SharedPtr<Scope> passedScope, 
    SharedPtr<Scope> ownScope, 
    SharedPtr<Scope> classScope, 
    String methodName, 
    MethodBody* body, 
    ParamList parameters, 
    CallableType callType, 
    SharedPtr<ClassInstanceNode> instanceNode) {
    
    MARK_UNUSED_MULTI(instanceNode);
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!passedScope){throw MerkError("Provided Scope to MethodDef::evaluate is null");}
    if (!ownScope){throw MerkError("MethodDef::evaluate, scope is null");}
    if (!classScope) {throw MerkError("Class Scope wargValuesas not supplied to Method: " + methodName);}

    auto freeVarNames = body->collectFreeVariables();

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
            throw MerkError("The Following Vars: " + highlight(joinUnorderedSetStrings(tempFreeVars, ", "), Colors::yellow) + "; were defined outside of function defined using function");
        }
    } 
    
    SharedPtr<Scope> defScope = passedScope->buildFunctionDefScope(freeVarNames, methodName);
    if (!defScope){DEBUG_FLOW_EXIT();throw MerkError("Defining Scope for FunctionDef::evaluate is null");}
    if (!defScope){throw MerkError("defScope created in MethodDef::evaluate is null");}

    defScope->owner = generateScopeOwner("MethodDef", methodName); 
    UniquePtr<BaseAST> clonedBodyBase = body->clone();
    auto clonedBody = static_unique_ptr_cast<MethodBody>(std::move(clonedBodyBase));

    // auto nonStaticEls = body->getNonStaticElements();

    // Vector<Chain*> nonStaticElements;
    // if (nonStaticEls.data()) {
    //     nonStaticElements = nonStaticEls;
    // } else {
    //     nonStaticElements = {};
    // }

    // clonedBody->setScope(defScope);
    // clonedBody->setNonStaticElements(nonStaticElements);
    
    if (!clonedBody){throw MerkError("ClonedBody in MethodDef::evaluate is null");}

    if (!clonedBody->getScope()){throw MerkError("Scope not present in FunctionDef::evaluate(scope) of clonedBody");}

    DEBUG_LOG(LogLevel::TRACE, "FunctionDef Defining Scope: ", passedScope->getScopeLevel());
    
    SharedPtr<Method> method = makeShared<UserMethod>(methodName, parameters, std::move(clonedBody), ownScope, callType);
    
    if (!method){throw MerkError("Method created in MethodDef::evaluate is null");}
    
    method->setCapturedScope(defScope);
    if (!method->getCapturedScope()) {throw MerkError("No Captured Scope for method: " + methodName);}
    

    auto methodSig = method->toCallableSignature();
    DEBUG_LOG(LogLevel::TRACE, "SETTING METHOD SIG PARAMS FOR ", methodName);

    methodSig->setParameters(parameters.clone());


    if (method->getCallableType() != CallableType::METHOD && method->getSubType() == CallableType::METHOD) {
        DEBUG_LOG(LogLevel::ERROR, "The Types TO Provided To MethodDef::Evaluate -> CallType: ", callableTypeAsString(callType));
        DEBUG_LOG(LogLevel::ERROR, "Method 'get_area_offset' is not being constructed properly with Type: ", callableTypeAsString(method->getCallableType()), "And SubType: ", callableTypeAsString(method->getSubType()));
        throw MerkError("Evaluator:: evaluateMethodDef: See Above Error 1");
    }

    classScope->registerFunction(methodName, methodSig);

    DEBUG_LOG(LogLevel::TRACE, highlight("Registered Method: " + methodName, Colors::yellow), "into: ", classScope.get(), "Owner: ", classScope->owner);

    DEBUG_LOG(LogLevel::TRACE, "Method", methodName, "registered to ", classScope->owner);

    

    if (!method->getBody()){DEBUG_FLOW_EXIT();throw MerkError("Function body is null in FunctionDef::evaluate");}

    // Wrap in a FunctionNode (for returning it as a value)
    MethodNode methodNode(method);
    
    DEBUG_FLOW_EXIT();
    return methodNode;
}

Node evaluateFunctionCall(String name, SharedPtr<Scope> scope, Arguments* arguments, SharedPtr<ClassInstanceNode> instanceNode) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE); 
    if (!scope) {throw MerkError("scope passed to FunctionCall::evaluate is null");}
    if (name == "showScope") {scope->debugPrint(); return Node(Null);}
    auto callArgs = arguments->evaluateAll(scope, instanceNode);
    SharedPtr<CallableSignature> optSig;

    auto sigOpt = scope->getFunction(name, callArgs);
    BoundArgs evaluatedArgs;

    if (sigOpt.has_value()) {
        optSig = sigOpt.value();
        evaluatedArgs = callArgs.bindToBound(optSig->getParameters(), /*allowDefaults=*/true);
    } else {
        scope->debugPrint();
        
        auto& var = scope->getVariable(name);
        if (var.isFunctionNode()) {
            auto funcNode = var.toFunctionNode();
            auto opt = funcNode.getFunction(name, callArgs, scope);
            if (opt.has_value()) {
                optSig = opt.value();
            } else {
                throw MerkError("Could Not Determine Overload for function " + name);
            }
        }        
        if (var.getValueNode().getType() == NodeValueType::Function) {
            throw MerkError("IS A FUNCTION>>>>YAAAAAAY");
        }
    }


    SharedPtr<Function> func = std::static_pointer_cast<Function>(optSig->getCallable());

    if (func->getSubType() == CallableType::NATIVE) {
        // func->parameters.verifyArguments(evaluatedArgs); // as opposed to placing them within the callScope
        auto m = scope->localTypes.matchCall(optSig->getTypeSignature(), callArgs);
        if (!m.ok) {}
        
        return func->execute(callArgs, scope, instanceNode);
    }

    
    SharedPtr<Scope> callScope;
    auto captured = func->getCapturedScope();
    if (!captured) { throw MerkError("Function has No CapturedScope 1"); }

    
    if (captured->getContext().getVariables().size() == 0) {
        callScope = captured;
        scope->appendChildScope(callScope, false);
        
    } else {
        callScope = scope->buildFunctionCallScope(func, func->getName());
    }
    

    // Node value;
    // try {
    //     value = func->execute(evaluatedArgs, callScope);
    //     if (func->getRequiresReturn()) { throw MerkError("Function did not return a value."); }
        
    // } catch (const ReturnException& e) {
    //     DEBUG_FLOW_EXIT();
    //     value = e.getValue();  // Extract and return function's result
    // }

    Node value = func->execute(callArgs, callScope);
    scope->removeChildScope(callScope);
    return value;
}

Node evaluateClassCall(SharedPtr<Scope> callScope, String className, ArgumentList argValues, SharedPtr<ClassInstanceNode> instanceNode) {
    MARK_UNUSED_MULTI(instanceNode);
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    auto classOpt = callScope->getClass(className);
    if (!classOpt.has_value()) {throw MerkError("Class not found: " + className);}

    auto classSig = classOpt.value();
    if (!classSig) {throw MerkError("Class signature is null for class: " + className);}
    
    auto classTemplate = classSig->getClassDef();
    
    SharedPtr<Scope> instanceScope = callScope->buildInstanceScope(classTemplate, classTemplate->getName());

    if (!instanceScope){throw MerkError("InstanceScope creation failed in ClassCall::evaluate()");}

    auto capturedScope = instanceScope->getParent();

    if (!capturedScope){throw MerkError("Captured Scope Does Not Exist When Instantiating class: " + classTemplate->getName());}

    auto params = classTemplate->getParameters().clone();
    

    SharedPtr<ClassInstance> instance = makeShared<ClassInstance>(classTemplate, capturedScope, instanceScope);
    
    instance->construct(argValues, instance);
    
    
    
    auto inst = ClassInstanceNode(instance);
    inst.getFlags().isInstance = true;
    inst.getFlags().isCallable = true;                  // if you store instances as Callable
    inst.getFlags().type = NodeValueType::ClassInstance; // or Callable, but be consistent
    inst.getFlags().fullType.setBaseType(className);     // "Types"
    inst.getFlags().inferredSig = callScope->localTypes.classType(className);
    DEBUG_FLOW_EXIT();
    return inst;
}

Node evaluateChain(SharedPtr<Scope> currentScope, SharedPtr<Scope> methodScope, int resolutionStartIndex, const Vector<ChainElement>& elements, SharedPtr<ClassInstanceNode> instanceNode) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    MARK_UNUSED_MULTI(methodScope);
    
    if (!currentScope) {throw MerkError("Chain::evaluate: no valid scope");}
    int index = resolutionStartIndex;
    
    Node currentVal;

    auto& baseElem = elements[index];
    if (baseElem.object->getAstType() == AstType::Accessor){
        currentVal = baseElem.object->evaluate(currentScope, instanceNode); // should evaluate to a ClassInstanceNode
    } else {
        currentVal = baseElem.object->evaluate(currentScope, instanceNode);
    }
    index ++;

    if (baseElem.object->getAstType() == AstType::Unknown) {throw MerkError("Unknown AST Type Found In Chain...see above");}

    for (size_t i = index; i < elements.size(); ++i) {
        const auto& elem = elements[i];
        AstType objType = elem.object->getAstType();

        if (objType == AstType::Unknown) {throw MerkError("Unknown AST Type Found In Chain...see above");}
        if (!currentVal.isValid()) {throw MerkError("Cannot Chain off of a null return value");}

        if (currentVal.isInstance()) {
            auto instance = currentVal.toInstance();
            currentScope = instance->getInstanceScope();
            if (!currentScope) {throw MerkError("Scope Invalid During Chain Iteration");}
            instanceNode = instance->getInstanceNode();
            switch (objType)
            {
            case AstType::VariableDeclaration:
                {
                    auto varDec = static_cast<VariableDeclaration*>(elem.object.get());
                    auto varName = varDec->getName();
                    
                    // auto res = varDec->getExpression()->evaluate(methodScope, instanceNode);
                    auto res = Evaluator::evaluateVariableDeclaration(varName, varDec->getExpression().get(), varDec->getVariableMeta(), currentScope, instanceNode);

                    // instance->declareField(varName, res);
                    currentScope->addMember(varName);  // ensures downstream logic will correctly resolve

                    currentVal = res;

                    break;
                }
            case AstType::VariableAssignment:
                {
                    auto varAss = static_cast<VariableAssignment*>(elem.object.get());
                    auto varName = varAss->getName();
                    
                    // currentVal = varAss->getExpression()->evaluate(methodScope, instanceNode);
                    // instance->updateField(varName, currentVal);
                    currentVal = Evaluator::evaluateVariableAssignment(varName, varAss, methodScope, instanceNode);

                    break;
                }
            case AstType::VariableReference:
                {
                    
                    auto varRef = static_cast<VariableReference*>(elem.object.get());
                    auto varName = varRef->getName();
                    currentVal = instance->getField(varRef->getName());
                    break;
                }
            
            case AstType::ClassMethodCall:
                {
                    
                    if (!instanceNode) {throw MerkError("No InstanceNode in Chain::evaluate -> ClassMethodCall");}
                    currentVal = elem.object->evaluate(methodScope, instanceNode);
                    break;
                }
                
            default:
                currentVal = elem.object->evaluate(currentScope, instanceNode);
                currentScope = elem.object->getScope();
                
                break;
            }
            if (!currentScope) {throw MerkError("Scope Invalid After Chain Iteration");}

        } else {
            if (elem.object->getAstType() == AstType::ClassMethodCall) { // For Handling ad-hoc/virtual methods that are not actually on the object i.e (clone, etc.)
                DEBUG_LOG(LogLevel::PERMISSIVE, currentVal.toString(), currentVal.getFlags().toString());
                currentScope->debugPrint();
                currentScope->printChildScopes();
                auto methodCall = static_cast<MethodCall*>(elem.object.get());
                auto args = methodCall->cloneArgs();
                auto evaluatedArgs = args->evaluateAll(currentScope->createChildScope(), instanceNode);
                currentVal = handleVirtualMethod(currentVal, elem.name, evaluatedArgs.getPositional());                
            } else {
                    throw MerkError("Evaluating Some other structure, the current Value is: " + 
                    currentVal.getTypeAsString() + " META: " + 
                    currentVal.getFlags().toString() + " \n AST_INFO: " + elem.object->toString());            
            }
            
            DEBUG_LOG(LogLevel::DEBUG, highlight("Else OBJECTS AST TYPE: ", Colors::red), astTypeToString(elem.object->getAstType()));            
            currentScope = elem.object->getScope();
            
        }
        
        if (currentVal.isInstance()) {instanceNode = currentVal.toInstance()->getInstanceNode();}
    }

    DEBUG_FLOW_EXIT();
    return currentVal;
}


}