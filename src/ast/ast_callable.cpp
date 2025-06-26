#include <unordered_set>
#include <ostream>
#include "core/types.h"
#include "core/node.h"
#include "core/errors.h"
#include "utilities/debugger.h"
#include "utilities/helper_functions.h"
#include "core/scope.h"
#include "ast/ast_base.h"
#include "ast/ast.h"
#include "ast/ast_control.h"

#include "core/callables/functions/function.h"
#include "core/evaluator.h"
#include "ast/ast_function.h"
#include "ast/ast_method.h"
#include "ast/ast_class.h"  
#include "ast/ast_callable.h"

#include "core/callables/argument_node.h"



// For Calling Within A Method's Context
MethodBody* CallableBody::toMethodBody() {
    return static_cast<MethodBody*>(this);

}
    
// For Calling Within A Function's Context
FunctionBody* CallableBody::toFunctionBody() {
    return static_cast<FunctionBody*>(this);
}

CallableBody* CallableBody::toCallableBody() {
    return static_cast<CallableBody*>(this);
}

UniquePtr<BaseAST> CallableBody::clone() const {
    UniquePtr<CallableBody> newBlock = makeUnique<CallableBody>(getScope());

    for (const auto &child : children) {
        newBlock->addChild(child->clone());
    }
    
    return newBlock;

}


UniquePtr<BaseAST> CallableDef::clone() const {
    ParamList clonedParams;
    for (auto& param: parameters){
        clonedParams.addParameter(ParamNode(param));
    }

    UniquePtr<CallableBody> clonedBodyBase = static_unique_ptr_cast<CallableBody>(body->clone());
    

    UniquePtr<CallableDef> calDef = std::make_unique<CallableDef>(name, clonedParams, std::move(clonedBodyBase), callType, getScope());
    return calDef;
}

UniquePtr<BaseAST> CallableCall::clone() const {
    Vector<UniquePtr<ASTStatement>> clonedArgs;
    for (auto& arg : arguments){
        UniquePtr<ASTStatement> argBase = static_unique_ptr_cast<ASTStatement>(arg->clone());
        clonedArgs.emplace_back(std::move(argBase));
    }
    UniquePtr<CallableCall> calCall = std::make_unique<CallableCall>(name, std::move(clonedArgs), getScope());
    return calCall;
}

UniquePtr<BaseAST> CallableRef::clone() const {
    UniquePtr<CallableRef> calRef = std::make_unique<CallableRef>(name, getScope());
    return calRef;
}

CallableDef::~CallableDef() = default;
CallableCall::~CallableCall() = default;



CallableBody::CallableBody(UniquePtr<CodeBlock>&& block)
    : CodeBlock(block->getScope()) {
    this->children = std::move(block->children);
    block.reset();
}

CallableBody::~CallableBody() {
    clear();
    getCurrentScope()->clear();
}

CallableBody::CallableBody(UniquePtr<CallableBody>&& oldBody)
  : CodeBlock(std::move(oldBody->getChildren()), oldBody->getScope()) {
    oldBody.reset();
}

CallableBody::CallableBody(SharedPtr<Scope> scope)
    : CodeBlock(scope) {}

CallableBody::CallableBody(UniquePtr<CallableBody>* block) : CodeBlock(std::move(block->get()->getChildren()), block->get()->getScope()) {
    block->release();
}

CallableDef::CallableDef(String name, ParamList parameters, UniquePtr<CallableBody> body, CallableType callType, SharedPtr<Scope> scope) 
    : ASTStatement(scope), name(name), parameters(parameters), body(std::move(body)), callType(callType){
    if (callableTypeAsString(callType) == "Unknown"){
        throw MerkError("Failed to instantiate callType at CallableDef instantiation");
    }
    branch = "Callable";
}

CallableCall::CallableCall(String name, Vector<UniquePtr<ASTStatement>> arguments, SharedPtr<Scope> scope)
    : ASTStatement(scope), name(name), arguments(std::move(arguments)) {
        branch = "Callable";
}

CallableRef::CallableRef(String name, SharedPtr<Scope> scope)
    : ASTStatement(scope), name(name) {
        branch = "Callable";
}

CallableSignature::CallableSignature(SharedPtr<Callable> callable, CallableType callTypeAdded)
    : callable(std::move(callable)), callType(callTypeAdded)
{
  DEBUG_FLOW(FlowLevel::VERY_LOW);
  callType = callTypeAdded;
  if (callableTypeAsString(callType) == "Unknown"){
    throw MerkError("Failed to instantiate callType at CallableSignature instantiation");
}
    // parameters = std::move(callable->parameters.clone());

  DEBUG_FLOW_EXIT();
}



void CallableSignature::setCallableType(CallableType functionType) { 
    callType = functionType; 
}



void CallableSignature::setParameters(ParamList params) {
    parameters = params;
}


bool CallableSignature::getIsUserFunction() { return callType != CallableType::NATIVE;}



Node CallableSignature::call(const Vector<Node>& args, SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::LOW);
    // scope->debugPrint();
    // scope->printChildScopes();
    auto val = callable->execute(args, scope);
    DEBUG_FLOW_EXIT();
    return val;
}

Node CallableSignature::call(const Vector<Node>& args, SharedPtr<Scope> scope, SharedPtr<Scope> classScope) const {
    (void)args;
    (void)scope;
    (void)classScope;
    throw MerkError("BaseClass CallableSignature has no implementation for CallableSignature::call");

}
  
  
SharedPtr<Callable> CallableSignature::getCallable() const { return callable;}

const ParamList CallableSignature::getParameters() {
    if (parameters.empty()) {
        parameters = getCallable()->parameters.clone();
        if (parameters.empty()) {
            return parameters;
        }
    }
    return parameters;
}

const Vector<NodeValueType>& CallableSignature::getParameterTypes() const {
    if (parameterTypes.empty()) {
        parameterTypes = callable->parameters.getParameterTypes();
        return parameterTypes;
    }
    if (getParameterTypes().size() == 0) {
        parameterTypes = callable->parameters.getParameterTypes();
    }
    return parameterTypes;
}



bool CallableSignature::matches(const Vector<NodeValueType>& argTypes) const {
    size_t paramCount = parameters.size();
    size_t argCount = argTypes.size();

    if (!parameters.empty() && parameters.back().isVarArgsParameter()) {
        if (argCount < paramCount - 1) return false;
    } else {
        if (argCount != paramCount) return false;
    }

    for (size_t i = 0; i < parameters.size(); ++i) {
        if (parameters[i].isVarArgsParameter()) break;

        NodeValueType expected = parameters[i].getType();
        NodeValueType provided = i < argTypes.size() ? argTypes[i] : NodeValueType::Uninitialized;

        if (expected == NodeValueType::Any || provided == NodeValueType::Uninitialized)
            continue;

        if (expected != provided) return false;
    }

    return true;
}




Vector<Node> CallableCall::handleArgs(SharedPtr<Scope> scope) const {
    Vector<Node> evaluatedArgs;


    for (const auto &arg : arguments) {
        evaluatedArgs.push_back(arg->evaluate(scope));
    }

    return evaluatedArgs;
}




Node CallableBody::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::LOW);
    auto val = Evaluator::evaluateBlock(children, scope, instanceNode);
    DEBUG_FLOW_EXIT();
    return val;
}

Node CallableDef::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    (void)scope;
    throw MerkError("Base CallableDef::evaluate called directly for: " + name);
}

Node CallableCall::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    (void)scope;
    return Node(scope->getVariable("var"));
}


Node CallableRef::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    (void)scope;
    return Node(scope->getVariable("var"));
}




CallableSignature::~CallableSignature() {
    parameterTypes.clear();
}






//ORIGINAL

// bool CallableSignature::matches(const Vector<NodeValueType>& argTypes) const {
//     size_t paramCount = parameters.size();
//     size_t argCount = argTypes.size();

//     if (!parameters.empty() && parameters.back().isVarArgsParameter()) {
//         if (argCount < paramCount - 1) return false;
//     } else {
//         if (argCount != paramCount) return false;
//     }

//     for (size_t i = 0; i < parameters.size(); ++i) {
//         if (parameters[i].isVarArgsParameter()) break;

//         NodeValueType expected = parameters[i].getType();
//         NodeValueType provided = i < argTypes.size() ? argTypes[i] : NodeValueType::Uninitialized;

//         if (expected == NodeValueType::Any || provided == NodeValueType::Uninitialized)
//             continue;

//         if (expected != provided) return false;
//     }

//     return true;
// }


// bool CallableSignature::matches(const Vector<NodeValueType>& argTypes) const {
//     DEBUG_FLOW(FlowLevel::LOW);
//     DEBUG_LOG(LogLevel::ERROR, highlight("Entering: ", Colors::orange), highlight("CallableSignature::matches", Colors::bold_blue));
//     // First, check that the number of arguments matches.
//     if (getParameterTypes().size() != argTypes.size()) {
        
//         DEBUG_LOG(LogLevel::ERROR, "Parameter count does not match. Expected: ", parameterTypes.size(),
//                     ", got: ", argTypes.size());
//         DEBUG_FLOW_EXIT();
//         return false;
//     }

//     // Now check each parameter type.
//     for (size_t i = 0; i < parameterTypes.size(); ++i) {
//         NodeValueType expected = parameterTypes[i];
//         NodeValueType provided = argTypes[i];
//         DEBUG_LOG(LogLevel::ERROR, "Expected: ", nodeTypeToString(expected), "Provided: ", nodeTypeToString(provided));

//         // Allow a parameter declared as "Any" to match any argument.
//         if (expected == NodeValueType::Any) {
//             continue;
//         }
//         // Optionally: Allow an argument of Uninitialized to match, if that is acceptable.
//         if (provided == NodeValueType::Uninitialized) {
//             continue;
//         }

//         // Otherwise, they must match exactly.
//         if (expected != provided) {
//             DEBUG_LOG(LogLevel::ERROR, "Type mismatch on parameter ", i, ": expected ",
//             nodeTypeToString(expected), ", got ", nodeTypeToString(provided));
//             DEBUG_LOG(LogLevel::ERROR, "Parameter Failed Expected Type Criteria, returning false");

//             DEBUG_FLOW_EXIT();
//             return false;
//         }
//     }

//     DEBUG_LOG(LogLevel::DEBUG, "Parameter Did Not Fail Any Criteria, returning true");

//     DEBUG_FLOW_EXIT();
//     return true;
// }

// template<typename T>
// SharedPtr<T> CallableSignature::getCallableClonedAs() const {
//     static_assert(std::is_base_of<Callable, T>::value, "T must derive from Callable");
//     auto casted = std::dynamic_pointer_cast<T>(callable);
//     if (!casted) {
//         throw MerkError("CallableSignature::getCallableAsClone() failed: Type mismatch or null callable.");
//     }
//     // Perform a deep copy of the Callable object
//     return makeShared<T>(*casted);
// }
