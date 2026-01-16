#include <unordered_set>
#include <ostream>

#include "core/TypesFWD.hpp"
#include "core/node/Node.hpp"
#include "core/node/ArgumentNode.hpp"

#include "core/errors.h"
#include "utilities/debugger.h"
#include "utilities/helper_functions.h"
#include "core/Scope.hpp"
#include "ast/AstBase.hpp"
#include "ast/Ast.hpp"
#include "ast/AstControl.hpp"
#include "core/callables/Callable.hpp"
#include "core/callables/functions/Function.hpp"

#include "core/Evaluator.h"
#include "ast/AstFunction.hpp"
#include "ast/AstMethod.hpp"
#include "ast/AstClass.hpp"  
#include "ast/AstCallable.hpp"

#include "utilities/debugger.h"


void Argument::setScope(SharedPtr<Scope> newScope) {
    MARK_UNUSED_MULTI(newScope);
    // if (key) { key->setScope(newScope); }
    
    // value->setScope(newScope);
};


String Argument::toString() const {
    String out = getAstTypeAsString() + "(";
    if (isKeyword() && key) {
        out += key->toString() + ":";
    }

    if (value) {
        out += value->toString() + ")";
    } else {
        throw MerkError("value in Arguments is null");
    }

    
    return out;
}

Arguments::~Arguments() {
    clear();
}


void Arguments::clear() {
    for (auto& arg : arguments) {
        arg.clear();
        
    }

}

Argument::Argument(Argument&& other) noexcept: key(std::move(other.key)), value(std::move(other.value)) {
    // if (other.key) {key = std::move(other.key);}
    // else {key = nullptr;}
}
Argument::~Argument() {
    clear();
}
void Argument::clear() {
    value.reset();
    key.reset();
}

Argument& Argument::operator=(Argument&& other) noexcept {
    if (this != &other) {
        // if (other.key) {key = std::move(other.key);}
        // value = std::move(other.value);
        key = std::move(other.key);
        value = std::move(other.value);
    }
    return *this;
};

Argument::Argument() = default;



Argument::Argument(UniquePtr<BaseAST> v) : key(nullptr), value(std::move(v)) {}
Argument::Argument(UniquePtr<BaseAST> k, UniquePtr<BaseAST> v)
    : key(std::move(k)), value(std::move(v)) {}



Node Argument::evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (isKeyword()) {
        auto k = key->evaluate(scope, instanceNode);
        auto v = value->evaluate(scope, instanceNode);

        v.setFlags(v.getFlags().merge({{"name", k.toString()}}));
        // v.setFlags(v.getFlags().mergeFlagsWith())
        // v.key = k.toString();
        DEBUG_FLOW_EXIT();
        return v; 
    } else {
        DEBUG_LOG(LogLevel::TRACE, "IN Argument::evaluate(SharedPtr<Scope> scope", "ABOUT TO EVALUATE ARGUMENT OF: ", value->getAstTypeAsString());
        DEBUG_FLOW_EXIT();
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
    MARK_UNUSED_MULTI(newScope);
    // scope = newScope;
    // for (auto& arg : arguments) {
    //     arg.setScope(newScope);
    // }
} 


ArgResultType Arguments::evaluateAll(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    ArgResultType evaluated;
    
    for (auto& arg : arguments) {
        if (arg.isKeyword()) {
            auto keyVal = arg.key->evaluate(scope, instanceNode);
            auto valVal = arg.value->evaluate(scope, instanceNode);
            evaluated.addNamedArg(keyVal.toString(), valVal);

        } else {
            DEBUG_LOG(LogLevel::TRACE, "ABOUT TO EVALUATE ARG: ", arg.getAstTypeAsString());
            auto argVal = arg.value->evaluate(scope, instanceNode);
            DEBUG_LOG(LogLevel::TRACE, "ARG VAL: ", argVal, "ARG NAME: ", argVal.getFlags().name);
            // if (argVal.isNull()) {throw MerkError("argVal is null in Arguments::evaluateAll");}
            evaluated.addPositionalArg(argVal);

            DEBUG_LOG(LogLevel::TRACE, "ARGS SO FAR: ", evaluated.toString());
        }
    }
    DEBUG_FLOW_EXIT();
    DEBUG_LOG(LogLevel::TRACE, joinVectorNodeStrings(evaluated.getPositional()));
    return evaluated;
}


Node Arguments::evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) const {
    MARK_UNUSED_MULTI(scope, instanceNode);
    throw MerkError("USE EVALUATEALL to evaluate Arguments");
};


void Arguments::add(Argument arg) {
    // throw MerkError("Not Ready");
    arguments.emplace_back(std::move(arg));
}


void Arguments::addPositional(UniquePtr<ASTStatement> val) {
    if (!val) {throw MerkError("val is Null");}
    // makeUnique<NoOpNode>(nullptr)
    // auto arg = ;
    arguments.push_back(Argument(std::move(val)));
}

void Arguments::addKeyword(UniquePtr<ASTStatement> key, UniquePtr<ASTStatement> val) {
    arguments.push_back(Argument(std::move(key), std::move(val)));
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

CallableSignature::CallableSignature(SharedPtr<NodeBase> newCallable) {
    // callType(newCallable->callType), subType(newCallable->subType), parameters(newCallable->parameters.clone())
    //     DEBUG_FLOW(FlowLevel::HIGH);
    //     if (!newCallable) {throw MerkError("Callable is NULL in: ");}
    //     callable = std::move(newCallable);
    //     DEBUG_FLOW_EXIT();
    if (!newCallable) {throw MerkError("Callable is NULL in: ");}
    auto call = asCallable(newCallable);
    // auto call = std::static_pointer_cast<Callable>(newCallable);
    callable = call;
    subType = call->subType;
    callType = call->callType;
    parameters = call->parameters.clone();

}


// CallableSignature::CallableSignature(SharedPtr<Callable> callable, CallableType callTypeAdded)
//     : callable(std::move(callable)), callType(callTypeAdded)
// {
//     DEBUG_FLOW(FlowLevel::VERY_LOW);
//     callType = callTypeAdded;
//     DEBUG_LOG(LogLevel::TRACE, "ClassSignature::ClassSignature -> classSig:", "CallableType: ", callableTypeAsString(getCallableType()), "SubType: ", callableTypeAsString(getSubType()));

//   DEBUG_FLOW_EXIT();
// }


CallableSignature::CallableSignature(SharedPtr<NodeBase> callable, CallableType callTypeAdded)
    : callable(std::move(asCallable(callable))), callType(callTypeAdded) {}

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
        if (argCount < paramCount - 1) { DEBUG_LOG(LogLevel::TRACE, "RETURNING FALSE ON argCount < paramCount"); return false;}
    } else {
        if (argCount != paramCount) { DEBUG_LOG(LogLevel::TRACE, "RETURNING FALSE ON argCount != paramCount"); return false; }
    }

    for (size_t i = 0; i < parameters.size(); ++i) {
        if (parameters[i].isVarArgsParameter()) break;

        NodeValueType expected = parameters[i].flags.type;
        NodeValueType provided = i < argTypes.size() ? argTypes[i] : NodeValueType::Uninitialized;

        if (expected == NodeValueType::Any || provided == NodeValueType::Uninitialized)
            continue;

        if (expected != provided) {DEBUG_LOG(LogLevel::TRACE, "RETURNING FALSE ON expected != provided "); return false; }
    }

    return true;
}

size_t CallableSignature::hash() const {
    size_t fullSig = 0;
    for (auto& param: parameters) {
        fullSig += param.hash();
    }
    std::size_t h3 = std::hash<std::string>()(getCallable()->getName());
    return h3 + fullSig;
}


ArgResultType CallableCall::handleArgs(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode) const {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    scope->debugPrint();
    
    ArgResultType evaluatedArgs = arguments->evaluateAll(scope, instanceNode);
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
    for (auto& name : parameters.getNames()) {  
        parameters.eraseByName(name);
    } 
}