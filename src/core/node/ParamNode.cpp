// #include "core/node/Node.hpp"
// #include "core/node/ParamNode.hpp"
// #include "core/node/ArgumentNode.hpp"
// #include "core/node/NodeStructures.hpp"
// #include "core/callables/classes/ClassBase.hpp"
// #include "core/callables/Callable.hpp"
// #include "core/errors.h"
// #include "utilities/debugger.h"

// ParamNode::~ParamNode() = default;

// ParamNode::ParamNode() = default;

// ParamNode::ParamNode(DataTypeFlags flgs, VariantType defaultVal) {
//     flags = flgs;
//     defaultValue = defaultVal;
//     setType(flags.type);
// }

// // // Explicit Copy Constructor
// ParamNode::ParamNode(const ParamNode& other) {
//     flags.name = other.getName();
//     defaultValue = other.defaultValue;
//     isVarArgs = other.isVarArgs;
//     hasDefaultValue = other.hasDefaultValue;
//     setValue(other.defaultValue);
// }
        
// // // Explicit Move Constructor (optional, for efficiency)
// ParamNode::ParamNode(ParamNode&& other) noexcept:
//     defaultValue(std::move(other.defaultValue)),
//     isVarArgs(other.isVarArgs),
//     hasDefaultValue(other.hasDefaultValue) {
//         flags.name = other.getName();
//         setValue(other.defaultValue);
//     }

// // Explicit Copy Assignment Operator
// ParamNode& ParamNode::operator=(const ParamNode& other) {
//     if (this != &other) {
        
//         flags.name = other.getName();
//         defaultValue = other.defaultValue;
//         isVarArgs = other.isVarArgs;
//         hasDefaultValue = other.hasDefaultValue;
//         setValue(other.defaultValue);
//     }
//     return *this;
// }


// std::size_t ParamNode::hash() const {
//     std::size_t h1 = std::hash<int>()(static_cast<int>(flags.type));
//     std::size_t h2 = 0;
    
//     std::visit([&](auto&& arg) {
//         using T = std::decay_t<decltype(arg)>;

//         if constexpr (std::is_same_v<T, NodeList>) {
//             // Hash the vector contents
//             std::size_t combined = 0;
//             for (const auto& n : arg) {
//                 combined ^= n.hash() + 0x9e3779b9 + (combined << 6) + (combined >> 2);
//             }
//             h2 = combined;
//         } else if constexpr (std::is_same_v<T, Vector<SharedPtr<CallableSignature>>>) {
//             std::size_t combined = 0;
//             for (const auto& sig : arg) {
//                 if (sig) {
//                     combined ^= sig->hash() + 0x9e3779b9 + (combined << 6) + (combined >> 2);
//                 }
//             }
//             h2 = combined;
            
//         } else if constexpr (std::is_same_v<T, NodeSetU>) {
//             std::size_t combined = 0;
//             for (const auto& sig : arg) {
//                 combined ^= sig.hash() + 0x9e3779b9 + (combined << 6) + (combined >> 2);
//             }
//             h2 = combined;   
//         } else if constexpr (std::is_same_v<T, NodeMapU> || std::is_same_v<T, NodeMapO>) {
//             std::size_t combined = 0;
//             for (const auto& [key, val] : arg) {
//                 combined ^= key.hash() + 0x9e3779b9 + val.hash() + (combined << 6) + (combined >> 2);
//             }
//             h2 = combined;
//         }
//         else {
//             h2 = std::hash<T>()(arg);
//         }
//     }, defaultValue);

//     std::size_t h3 = std::hash<std::string>()(flags.fullType.getBaseType());
//     std::size_t h4 = (flags.isConst << 1) ^ (flags.isMutable << 2) ^ (flags.isStatic << 3);

//     return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
// }

// // Check if this is a varargs parameter
// bool ParamNode::isVarArgsParameter() const { return isVarArgs; }

// void ParamNode::setIsVarArgsParam(bool val) {
//     isVarArgs = val;
// }

// // Check if this is a positional argument
// bool ParamNode::isPositional() const { return getName().empty(); }

// void ParamNode::setType(NodeValueType type) {
//     flags.type = type;
//     flags.isStatic = flags.type != NodeValueType::Any && flags.type != NodeValueType::Uninitialized && flags.type != NodeValueType::Null;
// }

// // Override setValue for VariantType with proper type checks
// ParamNode::ParamNode(String name, NodeValueType type, bool isVarArg) {
//     flags.name = name;
//     flags.type = type;
//     isVarArgs = isVarArg;
//     setType(type);
    
// }

// // Get parameter name
// String ParamNode::getName() const { return flags.name; }

// // Check if parameter has a default value
// bool ParamNode::hasDefault() const { return hasDefaultValue; }

// void ParamNode::setValue(VariantType newValue) {
//     defaultValue = newValue;
// }

// // Debugging
// void ParamNode::setValue(Node newValue) {
//     DEBUG_FLOW();
//     if (flags.type == NodeValueType::Uninitialized || flags.type == NodeValueType::Any) {
//         flags.type = newValue.getType();
//     } else {
//         validateExpectedType(newValue.getType());
//     }

//     DEBUG_FLOW_EXIT();
// }

// // Retrieve default value safely
// VariantType ParamNode::getDefaultValue() const {
//     DEBUG_FLOW();
//     if (!hasDefaultValue) {
//         throw RunTimeError("No default value set for parameter '" + getName() + "'.");
//     }
//     DEBUG_FLOW_EXIT();
//     return defaultValue;
// }

// // Validate expected type
// void ParamNode::validateExpectedType(NodeValueType expectedType) {
//     DEBUG_FLOW();
//     DEBUG_LOG(LogLevel::DEBUG, "Validating ParamNode Type -> Expected: ", nodeTypeToString(expectedType),
//              " | Actual: ", nodeTypeToString(flags.type));

//     if (expectedType == NodeValueType::Any || flags.type == NodeValueType::Any) {
//         return;  // Allow `Any` without type enforcement
//     }

//     if (expectedType == NodeValueType::Null && flags.type == NodeValueType::Null) {
//         return;  // Allow explicitly nullable parameters
//     }

//     if (expectedType != NodeValueType::Uninitialized && expectedType != flags.type) {
//         throw RunTimeError("Type mismatch for parameter '" + getName() +
//                            "': Expected " + nodeTypeToString(expectedType) +
//                            ", but got " + nodeTypeToString(flags.type) + ".");
//     }

//     DEBUG_FLOW_EXIT();
// }
 
// String ParamNode::toString() const {
//     return "ParamNode(name: " + getName() + 
//            ", Expected Type: " + nodeTypeToString(flags.type) + 
//            flags.toString() +
//            ", Default: " + (hasDefaultValue ? DynamicNode::forceToString(defaultValue) : "None") +  
//            ", Uninitialized: " + (std::holds_alternative<UninitializedType>(defaultValue) ? highlight("true", Colors::bold_blue) : highlight("false", Colors::bold_blue)) + ")";
// }

// String ParamNode::toShortString() const {
//     return getName() + ":" + DynamicNode::forceToString(defaultValue);
// }

// ParamNode ParamNode::copy() const {
//     // return *this;
//     // return ParamNode(*this);
//     // auto param = ParamNode(getValueNode(), getFlags().type);
//     return *this;
// }

// // // Add a parameter to the list
// void ParamList::addParameter(const ParamNode& param) {
//     DEBUG_FLOW();
//     parameters.push_back(param);
//     DEBUG_FLOW_EXIT();
// }

// void ParamList::verifyArguments(Vector<Node> args) {
//     DEBUG_FLOW(FlowLevel::MED);

//     bool variadic = !parameters.empty() && parameters.back().isVarArgsParameter();
//     size_t fixedParamCount = variadic ? parameters.size() - 1 : parameters.size();

//     // Check fixed arguments
//     for (size_t i = 0; i < fixedParamCount; ++i) {
//         if (i >= args.size() && parameters[i].hasDefault()) {
//             continue;
//         }

//         if (i >= args.size()) {
//             throw MerkError("Missing argument for parameter: " + parameters[i].getName());
//         }

//         parameters[i].setValue(args[i]);
//     }

//     // Handle variadic parameter
//     if (variadic) {
//         if (args.size() < fixedParamCount) {
//             throw MerkError("Too few arguments for variadic function.");
//         }

//         // Copy variadic arguments to a new vector to store in the param
//         Vector<Node> variadicArgs(args.begin() + fixedParamCount, args.end());
        
//         parameters.back().setValue(UninitializedType());  // Optional: clear placeholder
//     }

//     DEBUG_FLOW_EXIT();
// }

// void ParamList::verifyArguments(ArgumentList& args) {
//     DEBUG_FLOW(FlowLevel::MED);

//     bool variadic = !parameters.empty() && parameters.back().isVarArgsParameter();
//     size_t fixedParamCount = variadic ? parameters.size() - 1 : parameters.size();

//     // Check fixed arguments
//     for (size_t i = 0; i < fixedParamCount; ++i) {
//         const auto& param = parameters[i];

//         Node arg;
//         if (i < args.positionalCount()) {
//             arg = args.getArg(i);
//         } else if (args.hasNamedArg(param.getName())) {
//             arg = args.getNamedArg(param.getName());
//         } else if (param.hasDefault()) {
//             arg = Node::fromVariant(param.getDefaultValue());
//         } else {
//             throw MerkError("Missing argument for parameter: " + param.getName());
//         }

//         // paramaters[i].setValue(arg.getValue());
//         parameters[i].setValue(arg);
//         // param.setValueFromVariant(arg.getValue());
//     }

//     // Handle variadic parameter
//     if (variadic) {
//         if (args.size() < fixedParamCount) {
//             throw MerkError("Too few arguments for variadic function.");
//         }

//         // Copy variadic arguments to a new vector to store in the param
//         Vector<Node> variadicArgs(args.begin() + fixedParamCount, args.end());

//         parameters.back().setValue(UninitializedType());  // Optional: clear placeholder
//     }

//     DEBUG_FLOW_EXIT();
// }

// // Retrieve parameter by index
// const ParamNode& ParamList::operator[](size_t index) const {
//     if (index >= parameters.size()) {
//         throw MerkError("Parameter index out of range.");
//     }
//     return parameters[index];
// }

// // Retrieve parameter by name
// const ParamNode* ParamList::getByName(const String& name) const {
//     for (const auto& param : parameters) {
//         if (param.getName() == name) {
//             return &param;
//         }
//     }
//     return nullptr;
// }

// // Debugging: Convert to string
// String ParamList::toString() const {
//     String output = "ParamList(";
//     for (const auto& param : parameters) {
//         output += param.toString() + ", ";
//     }
//     output += ")";
//     return output;
// }

// String ParamList::toShortString() const {
//     String output;
//     size_t length = parameters.size() - 1;
//     for (const auto& param: parameters) {
//         output += param.toShortString();
//         if (param.getName() != parameters[length].getName()){
//             output += ", ";
//         }
//     }
//     output += ")";
//     return output;
// }

// void ParamList::resetToDefaults() {
//     for (auto& param : parameters) {
//         if (param.hasDefault()) {
//             param.setValue(param.getDefaultValue()); // Restore default
//         } else {
//             param.setValue(UninitializedType()); // Reset to uninitialized
//         }
//     }
// }

// Vector<NodeValueType> ParamList::getParameterTypes() {
//     Vector<NodeValueType> paramTypes;
//     for (auto& param : parameters){
//         paramTypes.emplace_back(param.flags.type);
//     }
//     return paramTypes;
// }

// ParamNode& ParamList::back() { return parameters.back();}
// ParamNode ParamList::back() const {return parameters.back();}

// void ParamList::printAST(std::ostream& os, int indent) const {
//     for (auto& param: parameters){
//         printIndent(os, indent);        
//         os << param.toString() << std::endl;
//     }
// }

// Vector<String> ParamList::getNames() const {
//     Vector<String> paramNames;
//     for (auto& param : parameters){
//         paramNames.emplace_back(param.getName());
//     }
//     return paramNames;
// }

// ParamList ParamList::clone() {
//     if (parameters.size() == 0) {
//         DEBUG_LOG(LogLevel::DEBUG, "ParamList is empty. Nothing to clone.");
//         return ParamList();
//         // throw MerkError("ParamList::clone() failed: parameters vector is null. (NON CONST)");
//     }
//     ParamList params;
//     for (auto& param : parameters) {
//         params.addParameter(param.copy());
//     }

//     return params;
// };

// ParamList ParamList::clone() const {
//     if (parameters.data() == nullptr) {
//         throw MerkError("ParamList::clone() failed: parameters vector is null.");
//     }
//     ParamList params;
//     for (auto& param : parameters) {
//         params.addParameter(param.copy());
//     }

//     return params;
// };

// void ParamList::erase(size_t index) {
//     if (index < parameters.size()) {
//         parameters.erase(parameters.begin() + index);
//     } else {
//         throw MerkError("ParamList::erase: index out of range");
//     }
// }

// bool ParamList::eraseByName(const String& name) {
//     auto it = std::find_if(parameters.begin(), parameters.end(),
//                            [&](const ParamNode& param) {
//                                return param.getName() == name;
//                            });
//     if (it != parameters.end()) {
//         parameters.erase(it);
//         return true;
//     }
//     return false;
// }

// bool ParamList::empty() const { return parameters.empty(); }

// ParamList::~ParamList() {
//     parameters.clear();
// }


#include "core/node/ParamNode.hpp"
#include "core/node/Node.hpp"
#include "core/node/NodeStructures.hpp"
#include "core/errors.h"
#include "core/registry/TypeRegistry.hpp"
#include "core/node/ArgumentNode.hpp"
#include "utilities/debugger.h"
#include "core/callables/Callable.hpp"

ParamNode::ParamNode(DataTypeFlags flgs, VariantType defaultVal) {
    flags = flgs;
    defaultValue = defaultVal;
    updateDefaultFlags();

    // If parser filled fullType, treat as annotation present.
    if (!flags.fullType.getBaseType().empty()) {
        hasAnnotation = true;
    }
}

ParamNode::ParamNode(String name, NodeValueType type, bool var) {
    flags.name = std::move(name);
    flags.type = type;
    isVarArgs = var;

    // legacy typing only
    hasAnnotation = false;
    typeSig = 0;

    updateDefaultFlags();
}

void ParamNode::updateDefaultFlags() {
    hasDefaultValue = !std::holds_alternative<UninitializedType>(defaultValue);
}

VariantType ParamNode::getDefaultValue() const {
    if (!hasDefaultValue) {
        throw RunTimeError("No default value set for parameter '" + flags.name + "'.");
    }
    return defaultValue;
}

void ParamNode::setAnnotation(const ResolvedType& rt) {
    flags.fullType = rt;      // you already have this field
    hasAnnotation = true;
}

void ParamNode::bindType(TypeRegistry& reg, Scope& scope) {
    if (!hasAnnotation) {
        typeSig = 0;
        return;
    }
    typeSig = reg.bindResolvedType(flags.fullType, scope);
}

String ParamNode::toShortString() const {
    // Prefer annotation name if exists; else legacy type
    String t = hasAnnotation ? flags.fullType.getBaseType() : nodeTypeToString(flags.type);
    return flags.name + ":" + t;
}



String ParamNode::toString() const {
    return "ParamNode(name: " + getName() + 
           ", Expected Type: " + nodeTypeToString(flags.type) + 
           flags.toString() +
           ", Default: " + (hasDefaultValue ? DynamicNode::forceToString(defaultValue) : "None") +  
           ", Uninitialized: " + (std::holds_alternative<UninitializedType>(defaultValue) ? highlight("true", Colors::bold_blue) : highlight("false", Colors::bold_blue)) + ")";
}


Vector<Node> ParamList::bindArguments(const Vector<Node>& args) const {
    bool variadic = !parameters.empty() && parameters.back().isVarArgsParameter();
    size_t fixedCount = variadic ? parameters.size() - 1 : parameters.size();

    Vector<Node> out;
    // out.reserve(parameters.size());

    // Fixed params
    for (size_t i = 0; i < fixedCount; ++i) {
        const auto& p = parameters[i];

        if (i < args.size()) {
            out.push_back(args[i]);
        } else if (p.hasDefault()) {
            out.push_back(Node::fromVariant(p.getDefaultValue()));
        } else {
            throw MerkError("Missing argument for parameter: " + p.getName());
        }
    }

    // Variadic param packs remaining args into a ListNode (or ArrayNode)
    if (variadic) {
        Vector<Node> rest;
        // if (args.size() > fixedCount)
        //     rest.assign(args.begin() + fixedCount, args.end());

        // TODO: decide your canonical varargs container:
        // Option A: Node(ListNode)
        // out.push_back(Node(std::make_shared<ListNode>(rest)));
        //
        // Option B: keep as list of Nodes in a ListNode builder you already have
        //
        // For now, simplest: empty list placeholder
        // Node(std::make_shared<ListNode>(rest));
        out.push_back(Node(makeShared<ListNode>(rest))); // if you have this ctor
    }

    return out;
}

void ParamList::verifyArguments(Vector<Node> args) {
    (void)bindArguments(args);
}

void ParamList::verifyArguments(ArgumentList& args) {
    (void)bindArguments(args);
}


const ParamNode* ParamList::getByName(const String& name) const {
    for (const auto& p : parameters) {
        if (p.getName() == name) {
            return &p;
        }
    }
    return nullptr;
}

// ------------------------------------------------------------
// Vector<String> getNames() const;
// ------------------------------------------------------------
Vector<String> ParamList::getNames() const {
    Vector<String> out;
    out.reserve(parameters.size());
    for (const auto& p : parameters) {
        out.push_back(p.getName());
    }
    return out;
}

// ------------------------------------------------------------
// const ParamNode& operator[](size_t index) const;
// ------------------------------------------------------------
const ParamNode& ParamList::operator[](size_t index) const {
    if (index >= parameters.size()) {
        throw MerkError("ParamList index out of range.");
    }
    return parameters[index];
}


std::size_t ParamNode::hash() const {
    std::size_t h1 = std::hash<int>()(static_cast<int>(flags.type));
    std::size_t h2 = 0;
    
    std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, NodeList>) {
            // Hash the vector contents
            std::size_t combined = 0;
            for (const auto& n : arg) {
                combined ^= n.hash() + 0x9e3779b9 + (combined << 6) + (combined >> 2);
            }
            h2 = combined;
        } else if constexpr (std::is_same_v<T, Vector<SharedPtr<CallableSignature>>>) {
            std::size_t combined = 0;
            for (const auto& sig : arg) {
                if (sig) {
                    combined ^= sig->hash() + 0x9e3779b9 + (combined << 6) + (combined >> 2);
                }
            }
            h2 = combined;
            
        } else if constexpr (std::is_same_v<T, NodeSetU>) {
            std::size_t combined = 0;
            for (const auto& sig : arg) {
                combined ^= sig.hash() + 0x9e3779b9 + (combined << 6) + (combined >> 2);
            }
            h2 = combined;   
        } else if constexpr (std::is_same_v<T, NodeMapU> || std::is_same_v<T, NodeMapO>) {
            std::size_t combined = 0;
            for (const auto& [key, val] : arg) {
                combined ^= key.hash() + 0x9e3779b9 + val.hash() + (combined << 6) + (combined >> 2);
            }
            h2 = combined;
        }
        else {
            h2 = std::hash<T>()(arg);
        }
    }, defaultValue);

    std::size_t h3 = std::hash<std::string>()(flags.fullType.getBaseType());
    std::size_t h4 = (flags.isConst << 1) ^ (flags.isMutable << 2) ^ (flags.isStatic << 3);

    return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
}


ParamNode ParamNode::copy() const {
    // return *this;
    // return ParamNode(*this);
    // auto param = ParamNode(getValueNode(), getFlags().type);
    return *this;
}



// ------------------------------------------------------------
// void bindTypes(TypeSignatureRegistry& reg, Scope& scope);
// ------------------------------------------------------------
void ParamList::bindTypes(TypeRegistry& reg, Scope& scope) {
    for (auto& p : parameters) {
        // bindType() should no-op if no annotation exists
        p.bindType(reg, scope);
    }
}

// ------------------------------------------------------------
// ParamList clone();   (non-const)
// ParamList clone() const;
// ------------------------------------------------------------
ParamList ParamList::clone() {
    ParamList out;
    out.parameters.reserve(parameters.size());
    for (const auto& p : parameters) {
        out.parameters.push_back(p); // ParamNode is copyable
    }
    return out;
}

ParamList ParamList::clone() const {
    ParamList out;
    out.parameters.reserve(parameters.size());
    for (const auto& p : parameters) {
        out.parameters.push_back(p);
    }
    return out;
}


// ------------------------------------------------------------
// Vector<Node> bindArguments(ArgumentList& args) const;
// (declaration-only ParamList: does NOT mutate parameters)
// ------------------------------------------------------------
Vector<Node> ParamList::bindArguments(ArgumentList& args) const {
    Vector<Node> boundArgs;

    const bool variadic = !parameters.empty() && parameters.back().isVarArgsParameter();
    const size_t fixedCount = variadic ? (parameters.size() - 1) : parameters.size();

    boundArgs.reserve(parameters.size());

    // Bind fixed params
    for (size_t i = 0; i < fixedCount; ++i) {
        const auto& param = parameters[i];

        Node arg;
        if (i < args.positionalCount()) {
            arg = args.getArg(i);

            // Preserve your existing behavior: copy param flags onto the arg
            // (If you want to stop mutating runtime Node flags later, remove this.)
            arg.setFlags(param.flags);

        } else if (args.hasNamedArg(param.getName())) {
            arg = args.getNamedArg(param.getName());

            // Optional: also stamp flags
            arg.setFlags(param.flags);

        } else if (param.hasDefault()) {
            arg = Node::fromVariant(param.getDefaultValue());

            // Optional: stamp flags
            arg.setFlags(param.flags);

        } else {
            throw MerkError("Missing argument for parameter: " + param.getName());
        }

        boundArgs.push_back(arg);
    }

    // Bind variadic tail
    if (variadic) {
        // Keep separation: do NOT create a ListNode here.
        // We'll pack as raw NodeList (Vector<Node>) wrapped as VariantType,
        // matching what your existing ArgumentList::bindTo does today.
        Vector<Node> rest = args.getRemainingArgs(fixedCount);

        // If you want param flags on the varargs param itself, you can create
        // a Node wrapper. For now, match your existing behavior:
        boundArgs.push_back(Node::fromVariant(rest));
    }

    return boundArgs;
}

// ------------------------------------------------------------
// Vector<NodeValueType> getParameterTypes();
// ------------------------------------------------------------
Vector<NodeValueType> ParamList::getParameterTypes() {
    Vector<NodeValueType> out;
    out.reserve(parameters.size());
    for (auto& p : parameters) {
        out.push_back(p.flags.type);
    }
    return out;
}

// ------------------------------------------------------------
// String toString() const;
// ------------------------------------------------------------
String ParamList::toString() const {
    String output = "ParamList(";
    for (size_t i = 0; i < parameters.size(); ++i) {
        output += parameters[i].toString();
        if (i + 1 < parameters.size())
            output += ", ";
    }
    output += ")";
    return output;
}

// ------------------------------------------------------------
// String toShortString() const;
// ------------------------------------------------------------
String ParamList::toShortString() const {
    String output = "ParamList(";
    for (size_t i = 0; i < parameters.size(); ++i) {
        output += parameters[i].toShortString();
        if (i + 1 < parameters.size())
            output += ", ";
    }
    output += ")";
    return output;
}


void ParamList::erase(size_t index) {
    if (index < parameters.size()) {
        parameters.erase(parameters.begin() + index);
    } else {
        throw MerkError("ParamList::erase: index out of range");
    }
}

bool ParamList::eraseByName(const String& name) {
    auto it = std::find_if(parameters.begin(), parameters.end(),
                           [&](const ParamNode& param) {
                               return param.getName() == name;
                           });
    if (it != parameters.end()) {
        parameters.erase(it);
        return true;
    }
    return false;
}

void ParamList::printAST(std::ostream& os, int indent) const {
    for (auto& param: parameters){
        printIndent(os, indent);        
        os << param.toString() << std::endl;
    }
}