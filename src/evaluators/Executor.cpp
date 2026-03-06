#include "core/TypesFWD.hpp"
#include "core/evaluators/Executor.hpp"
#include "core/errors.h"
#include "utilities/debugger.h"
#include "core/node/ArgumentNode.hpp"
#include "core/evaluators/EvalResult.hpp"
#include "ast/AstControl.hpp"
#include "core/node/ParamNode.hpp"
#include "core/Environments/Scope.hpp"
#include "ast/Exceptions.hpp"

void placeArgsInCallScope(const ArgumentList& evaluatedArgs, SharedPtr<Scope> callScope, ParamList& parameters) {
    DEBUG_FLOW(FlowLevel::MED);
    parameters.verifyArguments(evaluatedArgs);
    ArgumentList args;

    for (auto& evaluated: evaluatedArgs) {
        DEBUG_LOG(LogLevel::TRACE, "EVALUATED ARG: ", evaluated.toString(), " Evaluated ARG NAME: ", evaluated.getFlags().name);
        args.addPositionalArg(evaluated);
    }

    DEBUG_LOG(LogLevel::TRACE, "EVALUATED ARGS: ", args.toString());

    auto finalArgs = args.bindTo(parameters);

    for (size_t i = 0; i < parameters.size(); ++i) {
        auto arg = finalArgs[i];
        const auto& param = parameters[i];
        const String paramName = param.getName();
        if (paramName.empty()) { throw MerkError("Parameter name is empty"); }
        callScope->declareVariable(paramName, makeUnique<VarNode>(arg, param.flags));
    }

    DEBUG_FLOW_EXIT();
}

Node static nonFlowHandler(SharedPtr<Scope> callScope, String name, SharedPtr<Scope> capturedScope, CodeBlock* body, SharedPtr<ClassInstanceNode> instanceNode) {
    try {
        DEBUG_LOG(LogLevel::TRACE, "In try block");
        if (!callScope) { throw MerkError("Method " + name +" Has No Call Scope:"); }

        String matches = callScope == capturedScope ? "true" : "false";

        Node val = body->evaluate(callScope, instanceNode);

        DEBUG_FLOW_EXIT();
        return val;
    } catch (const ReturnException& e) {
        auto val = e.getValue();
        DEBUG_LOG(LogLevel::TRACE, "METHOD " + name + " RETURNED: " + val.getFlags().toString());
        DEBUG_FLOW_EXIT();
        return val;
    }
}


namespace Executor {
Node Function(const String& name, SharedPtr<Scope> callScope, SharedPtr<Scope> capturedScope, bool requiresReturn, const ArgumentList& args, CodeBlock* body, ParamList& parameters,
    SharedPtr<ClassInstanceNode> instanceNode) {
    MARK_UNUSED_MULTI(name, capturedScope);
    DEBUG_FLOW(FlowLevel::NONE);
    if (!callScope){throw MerkError("UserFunction::execute -> Starting Scope Null in: ");}
    placeArgsInCallScope(args, callScope, parameters);

    DEBUG_FLOW_EXIT();
    EvalResult r = body->evaluateFlow(callScope, instanceNode);
    if (r.isReturn()) { return r.value; }
    if (r.isThrow())  { throw MerkError("Unhandled throw"); }
    if (r.isBreak() || r.isContinue()) { throw MerkError("break/continue used outside loop"); }

    if (requiresReturn) { throw MerkError("Function did not return a value."); }
    return Node();
}

Node Method(const String& name, SharedPtr<Scope> callScope, SharedPtr<Scope> capturedScope, bool requiresReturn, const ArgumentList& args, CodeBlock* body, ParamList& parameters,
    SharedPtr<ClassInstanceNode> instanceNode) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!instanceNode) { throw MerkError("An Instance In UserMethod::execute was not provided"); }
    DEBUG_LOG(LogLevel::TRACE, "Validated Instance Node");
    callScope->owner = generateScopeOwner("MethodExecutor", name);
    
    if (callScope == instanceNode->getInstanceScope()) { throw MerkError("callScope cannot be the same as instanceScope"); }
    DEBUG_LOG(LogLevel::TRACE, "Placing Args in Call Scope");
    placeArgsInCallScope(args, callScope, parameters);

    
    bool executeFlow = true;
    if (executeFlow) {
        EvalResult r = body->evaluateFlow(callScope, instanceNode);
        if (r.isReturn()) { return r.value; }
        if (r.isThrow())  { throw RunTimeError("Unhandled throw"); }
        if (r.isBreak() || r.isContinue()) throw MerkError("break/continue used outside loop");

        if (requiresReturn) { throw MerkError("Method did not return a value."); }
        return Node();
    }

    return nonFlowHandler(callScope, name, capturedScope, body, instanceNode);
}
}