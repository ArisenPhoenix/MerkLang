#include "core/types.h"
#include "core/registry/registry.h"
#include "utilities/debugger.h"


FunctionRegistry::~FunctionRegistry() {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    // std::cout << "FunctionRegistry destructor called!\n";

    for (auto& [name, functionSignatures] : functions) {
        functionSignatures.clear();  // Explicitly clear each vector of UniquePtrs
    }
    functions.clear();  // Clear the map itself
    DEBUG_FLOW_EXIT();

}


void FunctionRegistry::registerFunction(const String& name, SharedPtr<FunctionSignature> signature) {
    DEBUG_FLOW(FlowLevel::LOW);
    auto& overloads = functions[name];
    FunctionType newFuncType = signature->getFunction()->getFunctionType();

    if (newFuncType == FunctionType::DEF) {
        // For def functions, there is only one allowed.
        if (!overloads.empty()) {
            overloads[0] = signature;
        } else {
            overloads.push_back(signature);
        }
    }

    else if (newFuncType == FunctionType::FUNCTION) {
        // For functions, check if any overload has the same signature.
        const auto& newParamTypes = signature->getParameterTypes();
        for (const auto& existingSig : overloads) {
            // Only compare if the existing overload is also a function.
            if (existingSig->getFunctionType() == FunctionType::FUNCTION && existingSig->matches(newParamTypes)) {
                DEBUG_FLOW_EXIT();
                
                throw MerkError("Duplicate overload for function: " + name);
            }
        }
        overloads.push_back(signature);
        DEBUG_LOG(LogLevel::DEBUG, "Overload: ", name, "Added");
    }
    else {
        throw MerkError("Unsupported function type for function: " + name + " Type: " + functionTypeAsString(newFuncType));
    }
    DEBUG_FLOW_EXIT();
}


bool FunctionRegistry::hasFunction(const String& name) const {
    DEBUG_FLOW(FlowLevel::MED);
    auto func = functions.find(name) != functions.end();
    DEBUG_FLOW_EXIT();
    return func;
}






std::optional<std::reference_wrapper<FunctionSignature>> FunctionRegistry::getFunction(const String& name, const Vector<Node>& args) {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    auto it = functions.find(name);
    if (it == functions.end() || it->second.empty()){
        DEBUG_LOG(LogLevel::ERROR, "Failed To Get Function:", name, "at first attempt");
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
        DEBUG_LOG(LogLevel::TRACE, "Checking Function Candidate", name, candidate->getFunction()->parameters.toString());

        // If this is a def function, return it regardless.
        // Def functions are meant to be more light weight and flexible.
        DEBUG_LOG(LogLevel::TRACE, "Function Type: ", functionTypeAsString(candidate->getFunctionType()));
        if (candidate->getFunctionType() == FunctionType::DEF) {
            return std::optional<std::reference_wrapper<FunctionSignature>>(*candidate);
        }
        else if (candidate->getFunctionType() == FunctionType::FUNCTION) {

            if (candidate->matches(argTypes)){
                DEBUG_FLOW_EXIT();
                return std::optional<std::reference_wrapper<FunctionSignature>>(*candidate);
            }
            DEBUG_LOG(LogLevel::ERROR, highlight("Function:", Colors::bold_blue), name, "args didn't match");
        }
    }

    DEBUG_LOG(LogLevel::TRACE, "Failed To Get Function:", name, "at end of getFunction");

    DEBUG_FLOW_EXIT();
    return std::nullopt;
}


std::optional<std::reference_wrapper<FunctionSignature>> FunctionRegistry::getFunction(const String& name) {
    DEBUG_FLOW(FlowLevel::MED);
    auto it = functions.find(name);
    if (it != functions.end() && !it->second.empty()) {
        
        DEBUG_FLOW_EXIT();
        return std::optional<std::reference_wrapper<FunctionSignature>>(*it->second.front());
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

