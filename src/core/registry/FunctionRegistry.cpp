#include "core/node/ArgumentNode.hpp"

#include "core/types.h"
#include "ast/AstCallable.hpp"
#include "core/callables/functions/Function.hpp"
#include "core/registry/FunctionRegistry.hpp"
#include "utilities/debugger.h"


FunctionRegistry::~FunctionRegistry() {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    for (auto& [name, vec] : functions) {
        vec.clear();
    }
    functions.clear();
    DEBUG_FLOW_EXIT();
}

void FunctionRegistry::clear() {
    functions.clear();
}

void FunctionRegistry::registerFunction(const String& name, SharedPtr<CallableSignature> signature) {
    DEBUG_FLOW(FlowLevel::LOW);
    
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
        const auto& newParamTypes = signature->getParameterTypes();
        for (const auto& existingSig : overloads) {
            if (existingSig->getSubType() == CallableType::FUNCTION && existingSig->matches(newParamTypes)) {
                DEBUG_FLOW_EXIT();
                
                throw MerkError("Duplicate overload for function: " + name);
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

// std::optional<SharedPtr<CallableSignature>>
// FunctionRegistry::getFunction(const String& name, const ArgumentList& args) const {
//     DEBUG_FLOW(FlowLevel::VERY_LOW);

//     auto it = functions.find(name);
//     if (it == functions.end() || it->second.empty()) {
//         DEBUG_LOG(LogLevel::TRACE, "Failed To Get Function:", name, "at first attempt");
//         return std::nullopt;
//     }

//     DEBUG_LOG(LogLevel::TRACE, highlight("Function:", Colors::bold_blue), name,
//               "was defined using", highlight("function", Colors::red));

//     // Invariant: if DEF exists, it is the only variant for that name.
//     for (auto& candidate : it->second) {
//         if (candidate->getSubType() == CallableType::DEF) {
//             return candidate;
//         }
//     }

//     SharedPtr<CallableSignature> best = nullptr;
//     int bestScore = std::numeric_limits<int>::max();
//     bool ambiguous = false;

//     for (auto& candidate : it->second) {
//         const auto sub = candidate->getSubType();

//         if (sub != CallableType::NATIVE && sub != CallableType::FUNCTION) {
//             continue;
//         }

//         DEBUG_LOG(LogLevel::TRACE, "Checking Function Candidate", name,
//                   candidate->getCallable()->parameters.toString());
//         DEBUG_LOG(LogLevel::TRACE, "Function Type: ", callableTypeAsString(sub));

//         // Clone params because verifyArguments mutates ParamNodes.
//         ParamList params = candidate->getParameters().clone();

//         // verifyArguments takes non-const ArgumentList& in your code, so copy args.
//         ArgumentList argsCopy = args;

//         try {
//             params.verifyArguments(argsCopy);
//         } catch (const MerkError&) {
//             DEBUG_LOG(LogLevel::TRACE, "Candidate ", name, " skipped: bind failed");
//             continue;
//         }

//         // Bind succeeded -> score type match using bound param values.
//         int score = 0;
//         bool reject = false;

//         for (std::size_t i = 0; i < params.size(); ++i) {
//             const auto& p = params[i];

//             const NodeValueType expected = p.flags.type;

//             // You need a way to get the Node bound into ParamNode and ask its NodeValueType.
//             // I'm guessing you have something like p.getValue() returning Node, or similar.
//             // const Node actualNode = p.getDefaultValue();          // <-- rename to your real API
//             // const NodeValueType actual = actualNode.getType(); // <-- you already use arg.getType()
//             // const Node actualNode = DynamicNode::fromVariant(p.getDefaultValue());
//             // const NodeValueType actual = DynamicNode::dispatch(p.getDefaultValue());4'
//             NodeValueType actual = DynamicNode::getTypeFromValue(p.getDefaultValue());
//             // exact match
//             if (actual == expected) continue;

//             // if (actual == NodeValueType::Int && expected == NodeValueType::Float) { score += 1; continue; }

//             // Otherwise incompatible
//             reject = true;
//             break;
//         }

//         if (reject) {
//             DEBUG_LOG(LogLevel::TRACE, "Candidate ", name, " skipped: type mismatch after bind");
//             continue;
//         }

//         // Prefer lower score
//         if (!best || score < bestScore) {
//             best = candidate;
//             bestScore = score;
//             ambiguous = false;
//         } else if (score == bestScore) {
//             ambiguous = true;
//         }
//     }

//     if (!best) {
//         DEBUG_LOG(LogLevel::TRACE, "Failed To Get Function:", name, "at end of getFunction");
//         DEBUG_FLOW_EXIT();
//         return std::nullopt;
//     }

//     if (ambiguous) {
//         // You can dump candidate signatures here if you want.
//         throw MerkError("Ambiguous overload resolution for function: " + name);
//     }

//     DEBUG_FLOW_EXIT();
//     return best;
// }


std::optional<SharedPtr<CallableSignature>> FunctionRegistry::getFunction(const String& name, const ArgResultType& args) const  {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    auto it = functions.find(name);
    if (it == functions.end() || it->second.empty()){
        DEBUG_LOG(LogLevel::TRACE, "Failed To Get Function:", name, "at first attempt");
        return std::nullopt;
    }

        
    Vector<NodeValueType> argTypes;
    for (const auto &arg : args) { argTypes.push_back(arg.getType()); }
    DEBUG_LOG(LogLevel::TRACE, highlight("Function:", Colors::bold_blue), name, "was defined using", highlight("function", Colors::red));

    // Search for a matching overload.
    for (auto candidate : it->second) {
        if (candidate->getSubType() == CallableType::DEF) {            
            return candidate;
        }
        if (candidate->getSubType() == CallableType::NATIVE) {
            if (candidate->matches(argTypes)) {return candidate;}
            // return candidate;
        }
        DEBUG_LOG(LogLevel::TRACE, "Checking Function Candidate", name, candidate->getCallable()->parameters.toString());

        // If this is a def function, return it regardless.
        DEBUG_LOG(LogLevel::TRACE, "Function Type: ", callableTypeAsString(candidate->getSubType()));
        
        if (candidate->getSubType() == CallableType::FUNCTION) {

            if (candidate->matches(argTypes)){
                DEBUG_FLOW_EXIT();
                return candidate;
            }
            DEBUG_LOG(LogLevel::TRACE, "Candidate ", name, " skipped: subtype=", callableTypeAsString(candidate->getSubType()));
        }
    }

    DEBUG_LOG(LogLevel::TRACE, "Failed To Get Function:", name, "at end of getFunction");

    DEBUG_FLOW_EXIT();
    return std::nullopt;
}


std::optional<Vector<SharedPtr<CallableSignature>>> FunctionRegistry::getFunction(const String& name) const {
    DEBUG_FLOW(FlowLevel::MED);
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