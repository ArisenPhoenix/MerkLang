#include "core/types.h"
#include "ast/ast_callable.h"
#include "core/functions/function_node.h"
#include "core/registry/function_registry.h"
#include "utilities/debugger.h"

 
FunctionRegistry::~FunctionRegistry() {
    DEBUG_FLOW(FlowLevel::VERY_LOW);

    for (auto& [name, functionSignatures] : functions) {
        functionSignatures.clear();  // Explicitly clear each vector of UniquePtrs
    }
    functions.clear();  // Clear the map itself
    DEBUG_FLOW_EXIT();
}


void FunctionRegistry::registerFunction(const String& name, SharedPtr<CallableSignature> signature) {
    DEBUG_FLOW(FlowLevel::LOW);
    
    // CallableType newFuncType = signature->getCallable()->getCallableType();
    const CallableType callType = signature->getCallableType();
    const CallableType subType = signature->getSubType();

    DEBUG_LOG(LogLevel::TRACE, "Registering Function: ", name);
    DEBUG_LOG(LogLevel::TRACE, "CallType: ", callableTypeAsString(callType), "SubType: ", callableTypeAsString(subType));
    auto& overloads = functions[name];
    if (callType != CallableType::FUNCTION && callType != CallableType::METHOD) {
        throw MerkError("Unsupported callable type in function registry: " + callableTypeAsString(callType));
    }
    bool isFunctionLike = callType == CallableType::FUNCTION || callType == CallableType::METHOD;
    if (!isFunctionLike) {

        throw MerkError("Unsupported callable type in function registry: " + callableTypeAsString(callType));
    }
    if (subType == CallableType::DEF) {
        // For def functions, there is only one allowed.
        if (!overloads.empty()) {
            overloads[0] = signature;
        } else {
            overloads.push_back(signature);
        }
    }

    else if (subType == CallableType::FUNCTION) {
        // For functions, check if any overload has the same signature.
        const auto& newParamTypes = signature->getParameterTypes();
        for (const auto& existingSig : overloads) {
            // Only compare if the existing overload is also a function.
            if (existingSig->getSubType() == CallableType::FUNCTION && existingSig->matches(newParamTypes)) {
                DEBUG_FLOW_EXIT();
                
                throw MerkError("Duplicate overload for function: " + name);
            }
        }
        overloads.push_back(signature);
        DEBUG_LOG(LogLevel::DEBUG, "Overload: ", name, "Added");
    }
    else {
        throw MerkError("Unsupported function type for function: " + name + " Type: " + callableTypeAsString(subType));
    }
    
    DEBUG_FLOW_EXIT();
}


bool FunctionRegistry::hasFunction(const String& name) const {
    DEBUG_FLOW(FlowLevel::MED);
    auto func = functions.find(name) != functions.end();
    DEBUG_FLOW_EXIT();
    return func;
}


std::optional<std::reference_wrapper<CallableSignature>> FunctionRegistry::getFunction(const String& name, const Vector<Node>& args) {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    auto it = functions.find(name);
    if (it == functions.end() || it->second.empty()){
        DEBUG_LOG(LogLevel::TRACE, "Failed To Get Function:", name, "at first attempt");
        return std::nullopt;
    }
        

    // Build a vector of argument types.
    Vector<NodeValueType> argTypes;
    for (const auto &arg : args) {
        argTypes.push_back(arg.getType());
    }
    DEBUG_LOG(LogLevel::TRACE, highlight("Function:", Colors::bold_blue), name, "was defined using", highlight("function", Colors::red));

    // Search for a matching overload.
    for (const auto& candidate : it->second) {
        DEBUG_LOG(LogLevel::TRACE, "Checking Function Candidate", name, candidate->getCallable()->parameters.toString());

        // If this is a def function, return it regardless.
        // Def functions are meant to be more light weight and flexible.
        DEBUG_LOG(LogLevel::TRACE, "Function Type: ", callableTypeAsString(candidate->getSubType()));
        if (candidate->getSubType() == CallableType::DEF) {
            return std::optional<std::reference_wrapper<CallableSignature>>(*candidate);
        }
        else if (candidate->getSubType() == CallableType::FUNCTION) {

            if (candidate->matches(argTypes)){
                DEBUG_FLOW_EXIT();
                return std::optional<std::reference_wrapper<CallableSignature>>(*candidate);
            }
            DEBUG_LOG(LogLevel::DEBUG, highlight("Function:", Colors::bold_blue), name, "args didn't match");
        }
    }

    DEBUG_LOG(LogLevel::TRACE, "Failed To Get Function:", name, "at end of getFunction");

    DEBUG_FLOW_EXIT();
    return std::nullopt;
}


std::optional<std::reference_wrapper<CallableSignature>> FunctionRegistry::getFunction(const String& name) {
    DEBUG_FLOW(FlowLevel::MED);
    auto it = functions.find(name);
    if (it != functions.end() && !it->second.empty()) {
        
        DEBUG_FLOW_EXIT();
        return std::optional<std::reference_wrapper<CallableSignature>>(*it->second.front());
    }

    DEBUG_FLOW_EXIT();
    return std::nullopt;
} 




void FunctionRegistry::debugPrint() const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    debugLog(true, "Registered Functions:");
    for (const auto &pair : functions) {
        debugLog(true, "  ", pair.first,  "(", pair.second.size()-1, " overload(s))");
    }
    DEBUG_FLOW_EXIT();
}


FunctionRegistry FunctionRegistry::clone() const {
    DEBUG_FLOW(FlowLevel::MED);
    FunctionRegistry copy;
    for (const auto& [fname, sigs] : functions) {
        for (const auto& sig : sigs) {
            // Shallow copy of the shared pointer.
            copy.registerFunction(fname, sig);
        }
    }

    DEBUG_FLOW_EXIT();
    return copy;
}

