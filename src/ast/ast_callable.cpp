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


String Argument::toString() const {
    String out = getAstTypeAsString() + "(";
    if (isKeyword()) {
        out += key->toString() + ":";
    }

    out += value->toString() + ")";
    return out;
}
void Argument::setScope(SharedPtr<Scope> newScope) {
    if (key) { key->setScope(newScope); }
    
    value->setScope(newScope);
};
Node Argument::evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) const {
    if (isKeyword()) {
        auto k = key->evaluate(scope, instanceNode);
        auto v = value->evaluate(scope, instanceNode);
        v.key = k.toString();
        return v; 
    } else {
        return value->evaluate(scope, instanceNode);
    }
}
Argument Argument::clone() const {
    auto arg = Argument();
    if (isKeyword()) { arg.key = std::move(key->clone()); }

    arg.value = std::move(value->clone());
    
    return arg;
}


Arguments::Arguments(SharedPtr<Scope> scope) : ASTStatement(scope) {}
Arguments::Arguments(Vector<Argument> arg, SharedPtr<Scope> scope) : ASTStatement(scope), arguments(std::move(arg)) {}

void Arguments::setScope(SharedPtr<Scope> newScope) {
    scope = newScope;
    for (auto& arg : arguments) {
        arg.setScope(newScope);
    }
}


ArgResultType Arguments::evaluateAll(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) {
    ArgResultType evaluated;
    
    for (auto& arg : arguments) {
        if (arg.isKeyword()) {
            auto keyVal = arg.key->evaluate(scope, instanceNode);
            auto valVal = arg.value->evaluate(scope, instanceNode);
            evaluated.addNamedArg(keyVal.toString(), valVal);
        } else {
            evaluated.addPositionalArg(arg.value->evaluate(scope, instanceNode));
        }
    }

    return evaluated;
}


Node Arguments::evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) const {
    MARK_UNUSED_MULTI(scope, instanceNode);
    throw MerkError("USE EVALUATEALL to evaluate Arguments");
};


void Arguments::add(Argument arg) {
    arguments.emplace_back(std::move(arg));
}


void Arguments::addPositional(UniquePtr<ASTStatement> val) {
    arguments.push_back({nullptr, std::move(val)});
}

void Arguments::addKeyword(UniquePtr<ASTStatement> key, UniquePtr<ASTStatement> val) {
    arguments.push_back({std::move(key), std::move(val)});
}

const Vector<Argument>& Arguments::getArgs() const { return arguments; }

bool Arguments::hasKeywords() const {
    for (auto &a : arguments) if (a.isKeyword()) return true;
    return false;
}


// CallableCall::CallableCall(String name, Vector<UniquePtr<ASTStatement>> arguments, SharedPtr<Scope> scope)
//     : ASTStatement(scope), name(name), arguments(std::move(arguments)) {
//         branch = "Callable";
// }

CallableCall::CallableCall(String name, UniquePtr<ArgumentType> arguments, SharedPtr<Scope> scope)
    : ASTStatement(scope), name(name), arguments(std::move(arguments)) {
        branch = "Callable";
}


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


CallableRef::CallableRef(String name, SharedPtr<Scope> scope)
    : ASTStatement(scope), name(name) {
        branch = "Callable";
}

CallableSignature::CallableSignature(SharedPtr<Callable> newCallable)
    : callType(newCallable->callType), subType(newCallable->subType), parameters(newCallable->parameters.clone())  {
        DEBUG_FLOW(FlowLevel::HIGH);
        if (!newCallable) {throw MerkError("Callable is NULL in: ");}
        callable = std::move(newCallable);
        DEBUG_FLOW_EXIT();
}

CallableSignature::CallableSignature(SharedPtr<Callable> callable, CallableType callTypeAdded)
    : callable(std::move(callable)), callType(callTypeAdded)
{
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    callType = callTypeAdded;
    DEBUG_LOG(LogLevel::TRACE, "ClassSignature::ClassSignature -> classSig:", "CallableType: ", callableTypeAsString(getCallableType()), "SubType: ", callableTypeAsString(getSubType()));

  DEBUG_FLOW_EXIT();
}

Vector<Chain*> CallableBody::getNonStaticElements() {
    if (nonStaticElements.size() > 0) {
        return nonStaticElements;
    }
    return {};
}

bool CallableBody::getIsStatic() {
    return getNonStaticElements().size() > 0;
}

void CallableBody::setNonStaticElements(Vector<Chain*> nonStaticEls){
    // if (!nonStaticEls.size()) {
    //     throw MerkError("Cannot set nonStaticEls to nullptr");
    // }
    nonStaticElements = nonStaticEls;
}



void CallableSignature::setCallableType(CallableType functionType) { 
    callType = functionType; 
}



void CallableSignature::setParameters(ParamList params) {
    parameters = params;
}


bool CallableSignature::getIsUserFunction() { return callType != CallableType::NATIVE;}



Node CallableSignature::call(const ArgumentList& args, SharedPtr<Scope> scope) const {
    DEBUG_FLOW(FlowLevel::LOW);
    auto val = callable->execute(args, scope);
    DEBUG_FLOW_EXIT();
    return val;
}

Node CallableSignature::call(const ArgumentList& args, SharedPtr<Scope> scope, SharedPtr<Scope> classScope) const {
    MARK_UNUSED_MULTI(args, scope, classScope);
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




ArgResultType CallableCall::handleArgs(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    ArgResultType evaluatedArgs;
    for (const auto &arg : arguments->getArgs()) {
        auto& value = arg.value;
        if (name == "other" && value->getAstType() == AstType::BinaryOperation && !scope->hasVariable("otherValue")) {
            DEBUG_LOG(LogLevel::PERMISSIVE, "Scope Being Used For Evaluating Args");
            scope->debugPrint();
            throw MerkError("Binary Operation will not compute without 'otherValue'");
        }
        auto val = arg.evaluate(scope, instanceNode);
        evaluatedArgs.addPositionalArg(val);
    }

    DEBUG_LOG(LogLevel::PERMISSIVE, "evaluatedArgs: ", evaluatedArgs.toString());
    return evaluatedArgs;
}




Node CallableBody::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    MARK_UNUSED_MULTI(scope, instanceNode);
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    auto val = Evaluator::evaluateBlock(children, scope, instanceNode);
    DEBUG_FLOW_EXIT();
    return val;
}

Node CallableDef::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    MARK_UNUSED_MULTI(scope, instanceNode);
    throw MerkError("Base CallableDef::evaluate called directly for: " + name);
}

Node CallableCall::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    MARK_UNUSED_MULTI(scope, instanceNode);
    throw MerkError("Cannot Evaluate Base CallableCall");
}


Node CallableRef::evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    MARK_UNUSED_MULTI(scope, instanceNode);
    throw MerkError("Cannot Evaluate Base CallableRef");
}




CallableSignature::~CallableSignature() {
    if (parameterTypes.data()) { parameterTypes.clear(); }   
}