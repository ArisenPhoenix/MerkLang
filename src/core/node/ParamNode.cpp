#include "core/node/ParamNode.hpp"
#include "core/node/Node.hpp"
#include "core/node/NodeStructures.hpp"
#include "core/errors.h"
#include "core/registry/TypeSignatureRegistry.hpp"
#include "core/node/ArgumentNode.hpp"
#include "utilities/debugger.h"
#include "core/callables/Callable.hpp"
#include "core/evaluators/TypeEvaluator.hpp"

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

void ParamNode::bindType(TypeSignatureRegistry& reg, Scope& scope) {
    if (!hasAnnotation) {
        typeSig = 0;
        return;
    }
    typeSig = reg.bindResolvedType(flags.fullType, scope);
    // scope.resolveTypeNameSig(name);
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
           ", Default: " + (hasDefaultValue ? TypeEvaluator::as<String>(defaultValue) : "None") +  
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
void ParamList::bindTypes(TypeSignatureRegistry& reg, Scope& scope) {
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