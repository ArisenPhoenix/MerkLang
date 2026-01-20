#include "core/node/Node.hpp"
#include "core/node/NodeStructures.hpp"
#include "core/types.h"
#include "core/callables/classes/ClassBase.hpp"

#include "utilities/debugger.h"
#include "utilities/debugging_functions.h"
#include "core/Scope.hpp"


// static void stampRuntimeType(Node& n) {
//     auto& tr = TypeRegistry::global();
//     auto& f  = n.getFlags();

//     // Always keep these coherent:
//     if (f.fullType.getBaseType().empty()) {
//         f.fullType.setBaseType(nodeTypeToString(f.type));
//     }

//     // Only set inferredSig if it's not set yet (or invalid)
//     if (f.inferredSig == kInvalidTypeSignatureId) {
//         const auto base = f.fullType.getBaseType();

//         // If you represent class instances by fullType name:
//         if (f.isInstance && !base.empty() && base != "Any") {
//             // If Dict/List are modeled as containers, use bindResolvedType or bindName lookup
//             if (auto id = tr.lookupName(base)) {
//                 f.inferredSig = *id;
//             } else {
//                 f.inferredSig = tr.classType(base);
//             }
//         } else {
//             // primitives + null + any
//             switch (f.type) {
//                 case NodeValueType::Int:    f.inferredSig = tr.primitive(NodeValueType::Int); break;
//                 case NodeValueType::Float:  f.inferredSig = tr.primitive(NodeValueType::Float); break;
//                 case NodeValueType::Double: f.inferredSig = tr.primitive(NodeValueType::Double); break;
//                 case NodeValueType::Bool:   f.inferredSig = tr.primitive(NodeValueType::Bool); break;
//                 case NodeValueType::String: f.inferredSig = tr.primitive(NodeValueType::String); break;
//                 case NodeValueType::Null:   f.inferredSig = tr.lookupName("Null").value_or(tr.any()); break; // if you have Null type
//                 default:                    f.inferredSig = tr.any(); break;
//             }
//         }
//     }
// }


DataTypeFlags::DataTypeFlags() = default;

DataTypeFlags::DataTypeFlags(bool isConst, bool isMutable, bool isStatic, NodeValueType mainType, ResolvedType type) {
    this->isConst = isConst;
    this->isMutable = isMutable;
    this->isStatic = isStatic;
    this->type = mainType;
    this->fullType = type;
}

DataTypeFlags::DataTypeFlags(bool isConst, bool isMutable, bool isStatic, ResolvedType type) {
    this->isConst = isConst;
    this->isMutable = isMutable;
    this->isStatic = isStatic;
    this->fullType = type;
}

DataTypeFlags::DataTypeFlags(String& thisName, bool isConst, bool isMutable, bool isStatic, ResolvedType type) {
    this->isConst = isConst;
    this->isMutable = isMutable;
    this->isStatic = isStatic;
    this->fullType = type;
    this->name = thisName;
    // type.getBaseType()
    auto t = stringToNodeType(type.getBaseType());
    if (t != NodeValueType::UNKNOWN) {this->type = t;}
}

DataTypeFlags& DataTypeFlags::mergeVarMetaFromDecl(const DataTypeFlags& decl) {
        // name/key are optional; depends on how you use them
        if (!decl.name.empty()) name = decl.name;
        if (!decl.key.empty())  key  = decl.key;

        // These three are "var properties" (but see note below about tri-state)
        // If you have tri-state, replace these with "if (decl.hasConst) ..."
        isConst   = decl.isConst;
        isMutable = decl.isMutable;
        isStatic  = decl.isStatic;

        // Declared typing constraints belong here
        if (decl.declaredSig != kInvalidTypeSignatureId) declaredSig = decl.declaredSig;
        if (decl.inferredSig != kInvalidTypeSignatureId) inferredSig = decl.inferredSig;

        // If you use fullType as the declared type constraint, copy it
        const auto& b = decl.fullType.getBaseType();
        if (!b.empty() && b != "Any") {
            fullType = decl.fullType;
        }

        // Intentionally DO NOT copy:
        // - isInstance/isCallable (runtime identity)
        // - type (NodeValueType) if you treat it as runtime category
        // - fullType if decl is Any/empty
        return *this;
    }

// Apply *runtime identity* based on an evaluated value node.
// This should be used for valueNode.flags, not for VarNode.varFlags.
DataTypeFlags& DataTypeFlags::applyRuntimeIdentityFromValue(const DataTypeFlags& valueFlags) {
    // Runtime identity
    isInstance = valueFlags.isInstance;
    isCallable = valueFlags.isCallable;

    // Runtime category (NodeValueType)
    type = valueFlags.type;

    // Runtime fullType (if you track it on values)
    if (!valueFlags.fullType.getBaseType().empty()) {
        fullType = valueFlags.fullType;
    }

    // Optionally propagate debug/name info (usually fine)
    if (!valueFlags.name.empty()) name = valueFlags.name;
    if (!valueFlags.key.empty())  key  = valueFlags.key;

    // Intentionally DO NOT copy declaredSig/inferredSig/isConst/isMutable/isStatic
    // Those are var-level constraints, not runtime identity.
    return *this;
}

// Optional convenience: only apply the variable name without touching anything else.
DataTypeFlags& DataTypeFlags::applyDeclName(const String& n, const String& k) {
    if (!n.empty()) name = n;
    if (!k.empty()) key = k;
    return *this;
}
String DataTypeFlags::toString() const {
    std::ostringstream oss;
    oss << "--> Flags (" << (isConst ? " isConst" : "") << (isMutable ? " isMutable," : "") << (isStatic ? " isStatic" : "") << " ) ";
    oss << "--> Type ( " << nodeTypeToString(type) + "," << (!fullType.getBaseType().empty() ? " FullType: " + fullType.toString() : "") << " ) ";
    oss << "--> Utility (" << (isInstance ? "isInstance" : "") << (isCallable ? "isCallable" : "") << " ) ";
    oss << "--> PlaceHolders ( " << "Name: " << name << (!key.empty() ? "Key " + key : "") << " )";

    return oss.str();
}

DataTypeFlags DataTypeFlags::merge(DataTypeFlags other) {
    name = other.name;
    isConst = other.isConst;
    isMutable = other.isMutable;
    isStatic = other.isStatic;
    type = other.type;
    fullType = other.fullType;
    isCallable = other.isCallable;
    isInstance = other.isInstance;
    declaredSig = other.declaredSig;   // <-- add
    inferredSig = other.inferredSig;   // <-- add
    return *this;
}

size_t DataTypeFlags::hash() const {
    size_t h = 0;

    auto combine = [](size_t& seed, size_t value) {
        // A common hash combining function
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    };

    combine(h, std::hash<bool>()(isConst));
    combine(h, std::hash<bool>()(isMutable));
    combine(h, std::hash<bool>()(isStatic));
    combine(h, std::hash<int>()(static_cast<int>(type)));  // assuming NodeValueType is enum class
    combine(h, fullType.hash()); // assuming ResolvedType has a hash() method
    combine(h, std::hash<bool>()(isCallable));
    combine(h, std::hash<bool>()(isInstance));
    combine(h, std::hash<String>()(name));
    combine(h, std::hash<String>()(key));

    return h;
}

DataTypeFlags DataTypeFlags::merge(const std::unordered_map<String, String>& values) {
    for (const auto& [k, val] : values) {
        if (k == "isConst") isConst = (val == "true");
        else if (k == "isMutable") isMutable = (val == "true");
        else if (k == "isStatic") isStatic = (val == "true");
        else if (k == "isCallable") isCallable = (val == "true");
        else if (k == "isInstance") isInstance = (val == "true");
        else if (k == "name") name = val;
        else if (k == "type") {
            auto nodeType = stringToNodeType(val);
            if (nodeType != NodeValueType::UNKNOWN) {
                type = nodeType;
            }
        }

        else if (k == "fullType") {fullType.setBaseType(val);}
        else if (k == "key") {key = k;}
        else {
            String out = k + ": ";
            out += " Not a valid key for passing into DataTypeFlags::merge";
            throw MerkError(out);
        }
    }

    // auto innerType = values.find("innerType"); if (innerType != values.end()) { Need to Parse Inner Structures}
    return *this;
}

DynamicNode::DynamicNode() { value = Null; }

DynamicNode::DynamicNode(SharedPtr<NodeBase> val) { value = val->getValue(); }

DynamicNode::DynamicNode(const VariantType val) { value = val; }

DynamicNode DynamicNode::fromVariant(VariantType v) {
    DynamicNode node;
    node.value = v;

    auto t = DynamicNode::getTypeFromValue(v);
    node.flags.type = t;
    node.flags.fullType.setBaseType(nodeTypeToString(t));
    node.flags.isInstance = (t == NodeValueType::ClassInstance); // if that’s your rule
    node.flags.isCallable = (t == NodeValueType::Callable);

    return node;
}

DynamicNode::DynamicNode(Node& node)
  : value(node.getValue())
{
    flags = node.getFlags();  // preserve runtime identity
}

DynamicNode::DynamicNode(const Node& node)
  : value(node.getValue())
{
    flags = node.getFlags();
}

DynamicNode::DynamicNode(const NodeBase& dyn)
  : value(dyn.getValue())
{
    flags = dyn.flags;  // assuming NodeBase stores flags
}

DynamicNode::DynamicNode(NodeBase&& dyn)
  : value(dyn.getValue())
{
    flags = dyn.flags;
}


void DynamicNode::clear() {
    if (std::holds_alternative<SharedPtr<Callable>>(getValue()) && flags.isInstance) {
        auto instance = std::static_pointer_cast<ClassInstance>(std::get<SharedPtr<Callable>>(getValue()));
        instance->getInstanceScope()->clear();
        instance->setScope(nullptr);
        instance.reset();
    } 
}

BoolNode::BoolNode(bool v) : value(v) { flags.type = NodeValueType::Bool; flags.fullType.setBaseType("Bool");}

BoolNode::BoolNode(VariantType v) {
    throw MerkError("Tried To construct Bool From Variant");
    if (std::holds_alternative<bool>(v)) { value = std::get<bool>(v); } 

    if (DynamicNode::getTypeFromValue(v) == NodeValueType::Null) {
        value = false;
    } else {
        throw MerkError("Attempted To Implicitly Cast " + nodeTypeToString(DynamicNode::getTypeFromValue(v)) + " to Bool");
    }
    flags.type = NodeValueType::Bool;
    flags.fullType.setBaseType("Bool");
    
}

StringNode::StringNode(String v) : value(v) { flags.type = NodeValueType::String; flags.fullType.setBaseType("String");}

void StringNode::clear() {value = "";}

IntNode::IntNode(int v) : value(v) {    
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    flags.type = NodeValueType::Int;
    flags.fullType.setBaseType("Int");
    DEBUG_FLOW_EXIT();
}

IntNode::IntNode(VariantType v) {
    throw MerkError("Tried To construct Int From Variant");
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (!std::holds_alternative<int>(v)) {
        throw MerkError("Cannot make an Int from another type");
    }
    value = std::get<int>(v);
    flags.type = NodeValueType::Int;
    flags.fullType.setBaseType("Int");
    DEBUG_FLOW_EXIT();
}

void IntNode::clear() {value = 0;}

FloatNode::FloatNode(float v) {DEBUG_FLOW(FlowLevel::PERMISSIVE);
    flags.type = NodeValueType::Float;
    flags.fullType.setBaseType("Float");
    value = v; DEBUG_FLOW_EXIT();

}

FloatNode::FloatNode(VariantType v) {
    throw MerkError("Tried To construct Float From Variant");

    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (std::holds_alternative<float>(v)) {
        value = std::get<float>(v);
        DEBUG_FLOW_EXIT();
        return;
    }
    

    flags.type = NodeValueType::Float;
    flags.fullType.setBaseType("Float");
    throw MerkError("Cannot Set type: " + nodeTypeToString(DynamicNode::getTypeFromValue(v)) + " to Float");
}

DoubleNode::DoubleNode(double v) {
    value = v;
    flags.type = NodeValueType::Double;
    flags.fullType.setBaseType("Double");
}

DoubleNode::DoubleNode(VariantType v) {
    throw MerkError("Tried To construct Double From Variant");

    if (std::holds_alternative<double>(v)) {
        value = std::get<double>(v);
        flags.type = NodeValueType::Double;
        flags.fullType.setBaseType("Double");
        return;
    }
    throw MerkError("Cannot Cast type " + nodeTypeToString(DynamicNode::getTypeFromValue(v)) + " To Double");
}

NodeWrapper::NodeWrapper() = default;

NodeWrapper::NodeWrapper(Node value) {DEBUG_FLOW(FlowLevel::PERMISSIVE); valueNode = std::move(value); DEBUG_FLOW_EXIT();}

// IntNode
VarNode::VarNode() = default;

VarNode::VarNode(Node node, bool isC, bool isMut, ResolvedType t, bool isStatic): NodeWrapper(std::move(node)) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    varFlags.isConst = isC;
    varFlags.isMutable = isMut;
    varFlags.fullType = t;
    varFlags.isStatic = isStatic || t.getBaseType() != "Any";
    
    varFlags.type = stringToNodeType(t.getBaseType());
    valueNode.getFlags().merge({{"isMutable", varFlags.isMutable ? "true" : "false"}});
    // valueNode.setFlags(varFlags);
    if (varFlags.type != NodeValueType::Any && valueNode.getType() != varFlags.type) {
        throw TypeMismatchError(t.getBaseType(), nodeTypeToString(valueNode.getType()), "(Node node, bool isC, bool isMut, ResolvedType t, bool isStatic)");
    }
    DEBUG_FLOW_EXIT();
}

UniquePtr<VarNode> VarNode::uniqClone() {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    // UniquePtr<VarNode> uniqCloned;
    auto uniqCloned = makeUnique<VarNode>();
    auto oldValue = valueNode;

    uniqCloned->valueNode = valueNode.clone();
    uniqCloned->varFlags = varFlags;
    auto type = DynamicNode::getTypeFromValue(valueNode.getValue());
    auto isTarget = (type == NodeValueType::Dict || type == NodeValueType::Callable || type == NodeValueType::DataStructure || type == NodeValueType::List || type == NodeValueType::ClassInstance);
    (void) isTarget;
    // if (!oldValue.isNative() && !oldValue.isInstance() && isTarget && uniqCloned->valueNode == valueNode) {
    //     String isDynamic = " isn't dynamic";
    //     if (dynamic_cast<DynamicNode*>(oldValue.getInner().get())) {
    //         isDynamic = " is certainly dynamic";
    //     }
    //     throw MerkError("the uniqueCloned value is still the same as the original -> " + isDynamic);
    // } 

    // if (isTarget) {
    //     throw MerkError("HAHA " + nodeTypeToString(type));
    // } 
    // else {
    //     throw MerkError("HAHA " + nodeTypeToString(type));
    // }

    // if ( isTarget ) { throw MerkError("Cloning a Target Type in VarNode " + nodeTypeToString(type)); }
    
    // if ((oldValue.getValue() == valueNode.getValue()) && isTarget) {
    //     auto msg = "Value didn't change for type " + nodeTypeToString(DynamicNode::getTypeFromValue(valueNode.getValue()));
    //     String msg2 = " -> " + nodeTypeToString(oldValue.getType()) + " " + oldValue.getFlags().toString();
        
    //     if (oldValue.isInstance()) {
    //         auto instance = oldValue.toInstance()->clone();
    //         if (instance == valueNode.toInstance()) {
    //             throw MerkError("The Instances haven't Changed");
    //         }
    //         msg2 += " \n Has A ClassInstance " + instance->toString();
    //     }

    //     throw MerkError(msg + msg2);
    // }

    // throw MerkError("Cloned Variable");
    DEBUG_FLOW_EXIT();
    return uniqCloned;
}

// For paramNode
VarNode::VarNode(Node defaultValue, bool isConst, bool isStatic, bool isMutable) {
    throw MerkError("Entered: VarNode::VarNode(Node defaultValue, bool isConst, bool isStatic, bool isMutable)");
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    valueNode = defaultValue;
    valueNode.setFlags(DataTypeFlags(isConst, isMutable, isStatic, defaultValue.getType(), ResolvedType(nodeTypeToString(defaultValue.getType()))));
    varFlags = valueNode.getFlags();
    // DEBUG_LOG(LogLevel::PERMISSIVE, "GOT DEFAULT VALUE OF " + defaultValue.toString());
    // if (varFlags.type != NodeValueType::Any && valueNode.getType() != varFlags.type) {
    //     throw TypeMismatchError(t.getBaseType(), nodeTypeToString(valueNode.getType()));
    // }
    DEBUG_FLOW_EXIT();
}

void printTypeComparison(Node& startingValue, DataTypeFlags flags) {
    MARK_UNUSED_MULTI(startingValue, flags);
        DEBUG_LOG(LogLevel::PERMISSIVE, 
        "\nValue Name: ", startingValue.getFlags().name,
        "\nValue: ", startingValue.toString(), 
        "\nValue Type: ", startingValue.getTypeAsString(), 
        "\nValue Flags: ", startingValue.getFlags().toString(),
        "\nFlag Type: ", nodeTypeToString(flags.type),
        "\nFlag Name: ", flags.name,
        "\nProvided Flags", flags.toString()   
    );
}

void validateTypes(NodeValueType defType, Node& startValue) {
    if (defType != NodeValueType::Any && defType != startValue.getType()) {
        printTypeComparison(startValue, startValue.getFlags());
        throw TypeMismatchError(
            nodeTypeToString(defType), 
            nodeTypeToString(startValue.getType()), 
            "VarNode::VarNode(Node startingValue, DataTypeFlags flags) Actual: " + startValue.getFlags().toString()
        );
    }
}


static bool matchesPrimitive(const Node& v, const String& t) {
    if (t=="String") return v.isString();
    if (t=="Char")   return v.isChars();
    if (t=="Float")  return v.isFloat();
    if (t=="Double") return v.isDouble();
    if (t=="Long")   return v.isDouble();
    if (t=="Number") return v.isInt() || v.isDouble() || v.isFloat();
    if (t=="Int")    return v.isInt();
    if (t=="Bool")   return v.isBool();
    if (t=="Null")   return v.isNull();
    if (t=="None")   return v.isNull();
    return false;
}

static bool isPrimitiveAnnotation(const String& t) {
    return t=="String" || t=="Char" || t=="Number" ||
           t=="Int" || t=="Bool" || t=="Float" || t=="Double" || t=="Long" ||
           t=="Null" || t=="None";
}


void stringBasedValidation(Node& valueNode, DataTypeFlags& varFlags) {
    const String expectedName = varFlags.fullType.getBaseType();
    if (!expectedName.empty() && expectedName != "Any") {

        if (isPrimitiveAnnotation(expectedName)) {
            if (!matchesPrimitive(valueNode, expectedName)) {
                throw TypeMismatchError(expectedName, valueNode.getTypeAsString(), "VarNode::VarNode -> stringBasedValidation 1");
            }
            return;
        }

        // Nominal (classes, including Dict/List/Set/Array)
        if (!valueNode.getFlags().isInstance) {
            throw TypeMismatchError(expectedName, "<not an instance>", "VarNode::VarNode -> stringBasedValidation 2");
        }

        const String gotName = valueNode.getFlags().fullType.getBaseType();
        if (gotName.empty()) {
            throw TypeMismatchError(expectedName, "<unknown instance type>", "VarNode::VarNode -> stringBasedValidation 3");
        }

        if (gotName != expectedName) {
            throw TypeMismatchError(expectedName, gotName, "VarNode::VarNode -> stringBasedValidation 4");
        }
    }

}

void typeIdBasedValidation(Node& startingValue, DataTypeFlags& varFlags) {
    if (varFlags.declaredSig != kInvalidTypeSignatureId) {
    // Enforce only if not Any (you need a way to compare to Any without registry;
    // easiest is: if fullType.base != "Any" as your signal)
    }

    if (!varFlags.fullType.getBaseType().empty() && varFlags.fullType.getBaseType() != "Any") {
        // Here we assume evaluateVariableDeclaration already set startingValue.flags.inferredSig
        if (startingValue.getFlags().inferredSig == kInvalidTypeSignatureId) {
            throw MerkError("VarNode: inferredSig missing on startingValue -> typeIdBasedValidation 1");
        }

        // Strict equality of sig ids is ok for nominal types in your current system
        if (startingValue.getFlags().inferredSig != varFlags.declaredSig) {
            throw TypeMismatchError(
                varFlags.fullType.getBaseType(),
                "<sig " + std::to_string(startingValue.getFlags().inferredSig) + ">",
                "VarNode::VarNode -> typeIdBasedValidation 2"
            );
        }
    }
}

// static void displaySigInfo(Node startingValue, DataTypeFlags& declFlags) {
//     DEBUG_LOG(LogLevel::DEBUG,
//         "VarNode ctor: decl.fullBase=", declFlags.fullType.getBaseType(),
//         " declSig=", declFlags.declaredSig,
//         " start.base=", startingValue.getFlags().fullType.getBaseType(),
//         " start.isInst=", startingValue.isInstance(),
//         " start.type=", startingValue.getTypeAsString(),
//         " start.inferredSig=", startingValue.getFlags().inferredSig
//     );
// }


VarNode::VarNode(Node startingValue, DataTypeFlags declFlags) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    

    if (!startingValue.getFlags().isMutable) {
        valueNode = startingValue.clone();
        valueNode.getFlags().applyRuntimeIdentityFromValue(startingValue.getFlags());
    } else {
        valueNode = startingValue;
    }

    if (valueNode.isNull() && !valueNode.isValid()) {
        throw MerkError("Set VarNode to Null Value");
    }

    varFlags = DataTypeFlags{};
    varFlags.mergeVarMetaFromDecl(declFlags);

    

    if (!declFlags.name.empty()) {
        varFlags.applyDeclName(declFlags.name, declFlags.key);
        valueNode.getFlags().applyDeclName(declFlags.name, declFlags.key);
    }
    // typeIdBasedValidation(startingValue, varFlags);
    stringBasedValidation(valueNode, varFlags);
    DEBUG_FLOW_EXIT();
}


VarNode::VarNode(VariantType value, bool isConst, bool isStatic, bool isMutable) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    throw MerkError("Entered: VarNode::VarNode(VariantType value, bool isConst, bool isStatic, bool isMutable)");
    NodeValueType dataType = DynamicNode::getTypeFromValue(value);
    setValue(Node::fromVariant(value));
    
    valueNode.setFlags(DataTypeFlags(isConst, isMutable, isStatic, dataType, ResolvedType(nodeTypeToString(dataType))));
    varFlags = valueNode.getFlags();
    DEBUG_FLOW_EXIT();
}


// VarNode Move Constructor
VarNode::VarNode(VarNode&& other) noexcept {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    setValue(other.getValueNode());
    valueNode.setFlags(other.getFlags());
    this->varFlags = other.varFlags;
    DEBUG_FLOW_EXIT();
}

// Copy Assignment Operator
VarNode& VarNode::operator=(const VarNode& other) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (this != &other) {
        this->setValue(other.getValueNode());
        this->varFlags = other.varFlags;
    }
    DEBUG_FLOW_EXIT();
    return *this;
}

// Move Assignment Operator
VarNode& VarNode::operator=(VarNode&& other) noexcept {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    
    if (this != &other) {
        this->setValue(other.getValueNode());
        this->varFlags = other.varFlags;
    }

    DEBUG_FLOW_EXIT();
    return *this;
}

VarNode::VarNode(const Node value) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    setValue(value);
    DEBUG_FLOW_EXIT();
}

VarNode::VarNode(const VarNode& other): NodeWrapper(other.getValueNode()) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    varFlags = other.varFlags;
    DEBUG_FLOW_EXIT();
}


// LitNode
LitNode::LitNode() = default;

LitNode::LitNode(const String& value, const String& typeStr) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    String typeOf = typeStr;
    if (typeStr == "Variable" && value == "null") { setValue(Node(Null)); /* indicates an actual value of null */ } 
    
    else {
        auto [val, type] = DynamicNode::getCoercedStringAndType(value, typeOf);
        if (type == NodeValueType::DataStructure || type == NodeValueType::Dict) {
            throw MerkError("Found " + nodeTypeToString(type) + "Being Constructed By LitNode");
        }
        if (nodeTypeToString(type, false) == "Unknown") { throw MerkError("Cannot Cast Unknown"); }
        auto node = DynamicNode::dispatchNode(val, nodeTypeToString(type, false), true);
        setValue(node);

        if (!node.isValid()) { throw MerkError("LiteralNode is invalid at construction"); }
    }

    DEBUG_FLOW_EXIT();
}

Node::Node(int v) : data(makeShared<IntNode>(v)) {
    if (!data) { throw MerkError("Data is Invalid in Node::Node(int v)"); }
}

Node::Node(String v) : data(makeShared<StringNode>(v)) {
    if (getFlags().type != NodeValueType::String) { throw MerkError("A String Didn't end up as string");}
    if (!data) { throw MerkError("Data is Invalid in Node::Node(String v)"); }
}

Node::Node(const char* s): Node(std::string{s}) {}

Node::Node(bool v) : data(makeShared<BoolNode>(v)) {
    if (!data) { throw MerkError("Data is Invalid in Node::Node(bool v)"); }
}

Node Node::fromVariant(VariantType v) {
    auto type = DynamicNode::getTypeFromValue(v);
    // if (type == NodeValueType::ClassInstance || type == NodeValueType::UserDefined || type == NodeValueType::Callable) {
    //     throw MerkError("Attempted To Create A Node of " + nodeTypeToString(type));
    // }

    if (type == NodeValueType::Dict || type == NodeValueType::DataStructure || type == NodeValueType::List) {
        throw MerkError("Attempted To create a Node from VariantType " + nodeTypeToString(type));
    }

    // data = makeShared<ClassInstanceNode>(v);
    auto node = Node();
    node.setValue(v);
    node.setFlags(node.getFlags().merge({{"type", nodeTypeToString(DynamicNode::getTypeFromValue(v), false)}}));
    // setValue(v);
    if (!node.data) { throw MerkError("Data is Invalid in Node::Node(VariantType v)"); }
    return node;
}

Node::Node(float v): data (makeShared<FloatNode>(v)) { }

Node::Node(double v): data (makeShared<DoubleNode>(v)) {}

Node::Node(NullType v): data(makeShared<NullNode>(v)) {}

Node::Node(SharedPtr<ListNode> v) {
    // throw MerkError("Attempted ListNode construction");
    // setValue(v);
    data = v;
    getFlags().type = NodeValueType::List;
    getFlags().fullType.setBaseType("List");
    if (!data) { throw MerkError("Data is Invalid in Node::Node(VariantType v)"); }
    throw MerkError("No Instance Associated With ListNode");
}

Node::Node(SharedPtr<ArrayNode> v) {
    // throw MerkError("Attempted ListNode construction");
    // setValue(v);
    // throw MerkError("Node Constructing DictNode");
    data = v;
    getFlags().type = NodeValueType::Array;
    getFlags().fullType.setBaseType("Array");
    if (!data) { throw MerkError("Data is Invalid in Node::Node(VariantType v)"); }
    // throw MerkError("No Instance Associated With DictNode");
}

// Node::Node(SharedPtr<DictNode> v) {
//     // throw MerkError("Attempted ListNode construction");
//     // setValue(v);
//     // throw MerkError("Node Constructing DictNode");
//     data = v;
//     getFlags().type = NodeValueType::Dict;
//     getFlags().fullType.setBaseType("Dict");
//     if (!data) { throw MerkError("Data is Invalid in Node::Node(VariantType v)"); }
//     // throw MerkError("No Instance Associated With DictNode");
// }

Node::Node(SharedPtr<DictNode> v) {
    // throw MerkError("Attempted ListNode construction");
    // setValue(v);
    // throw MerkError("Node Constructing DictNode");
    data = v;
    setFlags(data->flags);
    // getFlags().type = NodeValueType::Dict;
    // getFlags().fullType.setBaseType("Dict");
    if (!data) { throw MerkError("Data is Invalid in Node::Node(VariantType v)"); }
    // throw MerkError("No Instance Associated With DictNode");
}

Node::Node(SharedPtr<SetNode> v) {
    if (!v) {
        bool isNull = v == nullptr;
        String out = "The Passed SetNode isn't there";
        out += String("isNull: ") + (isNull ? "true" : "false");
        out += " " + v->toString();

        throw MerkError(out);
    }
    // throw MerkError("Attempted ListNode construction");
    // setValue(v);
    data = v;

    getFlags().type = NodeValueType::Set;
    getFlags().fullType.setBaseType("Set");
    if (!data) { throw MerkError("Data is Invalid in Node::Node(VariantType v)"); }
    // throw MerkError("No Instance Associated With SetNode");
}

// Node& Node::operator=(const Node& other) {
    

//     data = other.data;

//     if (other.isInstance()) {
//         // auto type = DynamicNode::getTypeFromValue(other.getValue());
//         // other.getFlags().merge({{"isInstance", "true"}});
//         // DEBUG_LOG(LogLevel::PERMISSIVE, "Assigning Instance to Node, it holds a " + nodeTypeToString(type) + " and holds meta of " + other.getFlags().toString());
//         data->flags.merge({{"isInstance", "true"}});
//         // *this = ClassInstanceNode(std::get<SharedPtr<Callable>>(other.getValue()));
//     } else {
//         data->flags = other.getFlags();
//     }
    
//     return *this;
// }

Node& Node::operator=(const Node& other) {
    if (this == &other) return *this;

    data = other.data;
    data->flags = other.getFlags(); // always copy full flags
    return *this;
}


Node& Node::operator=(Node&& other) noexcept {
    if (this != &other) {
        data = std::move(other.data);
        // setFlags(data->flags.merge({{"isInstance", (other.isInstance() ? "true" : "false")}}));
    }
    return *this;
}

Node::Node() { data = makeShared<NullNode>(); };

Node::Node(SharedPtr<NodeBase> base) : data(std::move(base)) {
    if (data->getType() == NodeValueType::Dict || data->getType() == NodeValueType::DataStructure) {
        throw MerkError("Constructed Node from " + nodeTypeToString(data->getType()));
    }
    
    // if (data->getType() != NodeValueType::String) {
    //     throw MerkError("Constructed Node from " + nodeTypeToString(data->getType()));
    // }
    
}

Node::Node(Vector<SharedPtr<CallableSignature>> sigRefs) { setValue(sigRefs); }