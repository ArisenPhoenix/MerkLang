
#include "core/node/Node.hpp"
#include "core/node/ParamNode.hpp"
#include "core/node/ArgumentNode.hpp"

#include "core/types.h"
#include "utilities/debugger.h"

#include "core/Scope.hpp" 
#include "ast/Ast.hpp"
#include "ast/Exceptions.hpp"
#include "ast/AstControl.hpp"

#include "ast/AstFunction.hpp"

#include "core/callables/Invocable.hpp"

#include "ast/AstCallable.hpp"




Function::Function(String name, ParamList params, [[maybe_unused]] CallableType funcType, bool requiresReturn, bool isStatic)
    : Invocable(name, params, CallableType::FUNCTION, requiresReturn, isStatic)
{
    DEBUG_FLOW(FlowLevel::NONE);
    // DEBUG_LOG(LogLevel::TRACE, "FuncType: ", callableTypeAsString(funcType));
    DEBUG_FLOW_EXIT();
}

UserFunction::UserFunction(String name, UniquePtr<FunctionBody> body, ParamList parameters, CallableType funcType)
    : Function(name, parameters, CallableType::FUNCTION), body(std::move(body)) {
        DEBUG_FLOW(FlowLevel::NONE);

        setSubType(funcType);
        setCallableType(CallableType::FUNCTION);
        DEBUG_FLOW_EXIT();
}



Node UserFunction::execute(ArgumentList args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
    // (void)args;
    // DEBUG_FLOW(FlowLevel::NONE);
    // if (!scope){throw MerkError("UserFunction::execute -> Starting Scope Null in: ");}
    // placeArgsInCallScope(args, scope);

    // DEBUG_FLOW_EXIT();
    // return body->evaluate(scope, instanceNode);

    DEBUG_FLOW(FlowLevel::NONE);
    if (!scope){throw MerkError("UserFunction::execute -> Starting Scope Null in: ");}
    placeArgsInCallScope(args, scope);

    DEBUG_FLOW_EXIT();
    EvalResult r = body->evaluateFlow(scope, instanceNode);

    if (r.isReturn()) return r.value;
    if (r.isThrow())  throw RunTimeError("Unhandled throw"); // or convert to your error model
    if (r.isBreak() || r.isContinue()) throw MerkError("break/continue used outside loop");

    // no explicit return
    if (requiresReturn) throw MerkError("Function did not return a value.");
    return Node();
}

// Node UserFunction::executeFlow(ArgumentList args, SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode) const {
//     DEBUG_FLOW(FlowLevel::NONE);
//     if (!scope){throw MerkError("UserFunction::execute -> Starting Scope Null in: ");}
//     placeArgsInCallScope(args, scope);

//     DEBUG_FLOW_EXIT();
//     EvalResult r = body->evaluateFlow(scope, instanceNode);

//     if (r.isReturn()) return r.value;
//     if (r.isThrow())  throw RunTimeError("Unhandled throw"); // or convert to your error model
//     if (r.isBreak() || r.isContinue()) throw MerkError("break/continue used outside loop");

//     // no explicit return
//     if (requiresReturn) throw MerkError("Function did not return a value.");
//     return Node();
// }

FunctionBody::~FunctionBody(){if (getScope()) {DEBUG_LOG(LogLevel::TRACE, highlight("Destroying FunctionBody", Colors::orange)); getScope().reset();}} 

SharedPtr<CallableSignature> UserFunction::toCallableSignature() {
    DEBUG_FLOW(FlowLevel::NONE);

    SharedPtr<CallableSignature> funcSig = makeShared<CallableSignature>(
        shared_from_this(), getCallableType()
    );

    funcSig->setSubType(getSubType());

    if (funcSig->getCallableType() == CallableType::DEF) {throw MerkError("Primary Callable Type is: " + callableTypeAsString(funcSig->getCallableType()));}

    funcSig->setParameters(parameters.clone());
 
    DEBUG_FLOW_EXIT();
    return funcSig;
}

void UserFunction::setScope(SharedPtr<Scope> newScope) const {
    MARK_UNUSED_MULTI(newScope);
    DEBUG_FLOW(FlowLevel::NONE);
    // if (!newScope) {throw MerkError("No newScope in UserFunction::setScope");}
    // newScope->owner = generateScopeOwner("UserFunction", name);
    // auto children = body->getAllAst();
    // body->setScope(newScope);

    DEBUG_FLOW_EXIT();
}

void UserFunction::setCapturedScope(SharedPtr<Scope> newScope) {
    capturedScope = newScope;
    // setScope(newScope);
}

String UserFunction::toString() const {return "<Function: " + getName() + ">";}


FunctionNode::FunctionNode(SharedPtr<Function> function) : CallableNode(function, "Function") {getFlags().type = NodeValueType::Function;}


FunctionNode::FunctionNode(String originalName, Vector<SharedPtr<CallableSignature>> functionSigs) : CallableNode(functionSigs, "CallableSignature") {
    auto first = functionSigs[0];
    auto instanceSpecificFlags = std::unordered_map<String, String>{
        {"isCallable", "true"},
        {"isInstance", "false"},
        {"name", originalName},
        {"fullType", "Function"}
    };
    setFlags(getFlags().merge(instanceSpecificFlags));
    setValue(functionSigs);
    if (getFlags().name.find("Method") != String::npos) {
        throw MerkError("HIT CallableNode::CallableNode(SharedPtr<CallableNode> WITH META: " + getFlags().toString());
    }
}

String FunctionNode::toString() const {
    return "<FunctionRef " + std::to_string(std::get<Vector<SharedPtr<CallableSignature>>>(getValue()).size()) + " overload(s)>";
}

// FunctionNode::FunctionNode(SharedPtr<Callable> function) : CallableNode(function, "Function") {data.type = NodeValueType::Function;}

SharedPtr<Callable> FunctionNode::getCallable() const {return std::get<SharedPtr<Callable>>(getValue());}


CallableBody* UserFunction::getInvocableBody() {return body.get();}
CallableBody* UserFunction::getBody() const {return body.get();}
FunctionBody* UserFunction::getThisBody() const {return body.get();}
UniquePtr<CallableBody> UserFunction::getBody() {return static_unique_ptr_cast<CallableBody>(body->clone());}


std::optional<SharedPtr<CallableSignature>> FunctionNode::getFunction(String name, const ArgumentList& args, SharedPtr<Scope> scope) {
    if (!scope) { throw MerkError("FunctionNode::getFunction called with null scope"); }
    if (!std::holds_alternative<Vector<SharedPtr<CallableSignature>>>(getValue())) {
        throw MerkError("FunctionNode holds type " + nodeTypeToString(DynamicNode::getTypeFromValue(getValue())));
    }
    auto funcSigs = std::get<Vector<SharedPtr<CallableSignature>>>(getValue());

    if (funcSigs.empty()) { throw MerkError("There were no function signatures to pull from"); }

    auto typeReg = scope->localTypes;
    // if (!typeReg) { throw MerkError("No TypeRegistry available on scope"); }

    struct Cand { SharedPtr<CallableSignature> sig; TypeMatchResult m; };
    Vector<Cand> viable;
    viable.reserve(funcSigs.size());

    for (auto& sig : funcSigs) {
        if (!sig) continue;

        // DEF: keep as fallback
        if (sig->getSubType() == CallableType::DEF) {
            viable.push_back(Cand{sig, TypeMatchResult::Yes(0, 1000)});
            continue;
        }

        if (sig->getSubType() == CallableType::NATIVE) {
            // if (funcSig->matches(argTypes)) {
            //     return funcSig;
            // }
            viable.push_back(Cand{sig, TypeMatchResult::Yes(0, 1000)});
        }

        ParamList params = sig->getParameters();
        BoundArgs bound;
        try {
            bound = args.bindToBound(params, /*allowDefaults=*/true);
        } catch (...) {
            continue;
        }

        // Lazily build signature id
        if (sig->getTypeSignature() == kInvalidTypeSignatureId) {
            params.bindTypes(typeReg, *scope);
            InvocableSigType mt;
            mt.methodName = name;
            mt.variadic = (!params.empty() && params.back().isVarArgsParameter());
            mt.ret = typeReg.any();
            mt.retEnforced = false;
            mt.params.reserve(params.size());
            mt.enforced.reserve(params.size());
            for (size_t i = 0; i < params.size(); ++i) {
                const auto& p = params[i];
                if (p.isTyped() && p.getTypeSig() != 0) {
                    mt.params.push_back(p.getTypeSig());
                    mt.enforced.push_back(1);
                } else {
                    mt.params.push_back(typeReg.any());
                    mt.enforced.push_back(0);
                }
            }
            sig->setTypeSignature(typeReg.invocableType(mt));
        }

        ArgumentList flat;
        for (auto& n : bound.flatten()) flat.addPositionalArg(n);
        auto m = typeReg.matchCall(sig->getTypeSignature(), flat);
        if (!m.ok) continue;
        viable.push_back(Cand{sig, m});
    }

    if (viable.empty()) return std::nullopt;

    auto best = viable[0];
    for (auto& c : viable) {
        if (c.m.score > best.m.score) best = c;
        else if (c.m.score == best.m.score && c.m.cost < best.m.cost) best = c;
    }

    int ties = 0;
    for (auto& c : viable) {
        if (c.m.score == best.m.score && c.m.cost == best.m.cost) ties++;
    }
    if (ties > 1) {
        throw MerkError("Ambiguous overload for function reference '" + name + "'");
    }

    return best.sig;
    
}
