#include "core/node/ArgumentNode.hpp"

#include "core/types.h"
#include "ast/AstCallable.hpp"
#include "core/callables/functions/Function.hpp"
#include "core/registry/FunctionRegistry.hpp"
#include "utilities/debugger.h"
// #include "core/callables/helpers.hpp"

FunctionRegistry::~FunctionRegistry() {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    for (auto& [name, vec] : functions) {
        vec.clear();
    }
    functions.clear();
    DEBUG_FLOW_EXIT();
}

void FunctionRegistry::clear() {
    for (auto& [name, obj] : functions) {
        obj.clear();
    }
    functions.clear();
}

void FunctionRegistry::registerFunction(const String& name, SharedPtr<CallableSignature> signature) {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    
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
        if (!overloads.empty()) {
            overloads[0] = signature;
        } else {
            overloads.push_back(signature);
        }
    }

    else if (subType == CallableType::FUNCTION) {
        // Duplicate check: compare parameter hashes (includes annotation info).
        const auto newHash = signature->hash();
        for (const auto& existingSig : overloads) {
            if (existingSig && existingSig->getSubType() == CallableType::FUNCTION) {
                if (existingSig->hash() == newHash) {
                    DEBUG_FLOW_EXIT();
                    throw MerkError("Duplicate overload for function: " + name);
                }
            }
        }
        overloads.push_back(signature);
        DEBUG_LOG(LogLevel::DEBUG, "Overload: ", name, "Added");
    }

    else if (subType == CallableType::NATIVE) {
        // const auto& newParamTypes = signature->getParameterTypes();
        // for (const auto& existingSig : overloads) {
        //     if (existingSig->getSubType() == CallableType::FUNCTION && existingSig->matches(newParamTypes)) {
        //         DEBUG_FLOW_EXIT();
                
        //         throw MerkError("Duplicate overload for function: " + name);
        //     }
        // }
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



std::optional<Vector<SharedPtr<CallableSignature>>> FunctionRegistry::getFunction(const String& name) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    auto it = functions.find(name);
    if (it != functions.end() && !it->second.empty()) {
        DEBUG_FLOW_EXIT();
        return it->second;
    }

    DEBUG_FLOW_EXIT();
    return std::nullopt;
} 




void FunctionRegistry::debugPrint() const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    if (functions.empty()) {
        debugLog(true, "No functions registered.");
    } else {
        debugLog(true, "Registered Functions:");
        for (const auto &pair : functions) {
            debugLog(true, "  ", pair.first,  "(", pair.second.size()-1, " overload(s))");
        }
    }
    
    DEBUG_FLOW_EXIT();
}


FunctionRegistry FunctionRegistry::clone() const {
    FunctionRegistry copy;
    for (const auto& [name, sigList] : this->functions) {
        std::vector<SharedPtr<CallableSignature>> copied;
        for (const auto& sig : sigList) {
            copied.push_back(makeShared<CallableSignature>(*sig)); // Clone the signature
        }
        copy.functions[name] = std::move(copied);
    }
    return copy;
}

void FunctionRegistry::merge(const FunctionRegistry& other) {
    for (const auto& [name, overloads] : other.getFunctions()) {
        for (auto& sig : overloads) {
            registerFunction(name, sig);
        }
    }
}

size_t FunctionRegistry::size() {return functions.size();}
size_t FunctionRegistry::size() const {return functions.size();};

bool FunctionRegistry::empty() const {return functions.empty();}