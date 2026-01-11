#include "core/node/Node.hpp"
#include "core/node/NodeStructures.hpp"
#include "core/types.h"
#include "core/callables/classes/ClassBase.hpp"

#include "utilities/debugger.h"
#include "utilities/debugging_functions.h"
#include "core/Scope.hpp"

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
    DEBUG_FLOW(FlowLevel::PERMISSIVE); 
    auto node = DynamicNode();
    node.flags.type = NodeValueType::Any; 
    
    node.value = v;
    DEBUG_FLOW_EXIT();
    return node;
}

DynamicNode::DynamicNode(const NodeBase& dyn): value(dyn.getValue()) {DEBUG_FLOW(FlowLevel::PERMISSIVE); flags.type = NodeValueType::Any; DEBUG_FLOW_EXIT();}

DynamicNode::DynamicNode(NodeBase&& dyn): value(dyn.getValue()) {DEBUG_FLOW(FlowLevel::PERMISSIVE); flags.type = NodeValueType::Any; DEBUG_FLOW_EXIT();}

DynamicNode::DynamicNode(Node& node): value(node.getValue()) {DEBUG_FLOW(FlowLevel::PERMISSIVE); flags.type = NodeValueType::Any; DEBUG_FLOW_EXIT();}

DynamicNode::DynamicNode(const Node& node): value(node.getValue()) {DEBUG_FLOW(FlowLevel::PERMISSIVE); flags.type = NodeValueType::Any; DEBUG_FLOW_EXIT(); }

void DynamicNode::clear() {
    if (std::holds_alternative<SharedPtr<Callable>>(getValue()) && flags.isInstance) {
        auto instance = std::static_pointer_cast<ClassInstance>(std::get<SharedPtr<Callable>>(getValue()));
        instance->getInstanceScope()->clear();
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
        throw TypeMismatchError(t.getBaseType(), nodeTypeToString(valueNode.getType()));
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
    
    if (isTarget && uniqCloned->valueNode == valueNode) {
        String isDynamic = " isn't dynamic";
        if (dynamic_cast<DynamicNode*>(oldValue.getInner().get())) {
            isDynamic = " is certainly dynamic";
        }
        throw MerkError("the uniqueCloned value is still the same as the original -> " + isDynamic);
    } 

    if (isTarget) {
        throw MerkError("HAHA " + nodeTypeToString(type));
    } 
    else {
        throw MerkError("HAHA " + nodeTypeToString(type));
    }

    if ( isTarget ) { throw MerkError("Cloning a Target Type in VarNode " + nodeTypeToString(type)); }
    
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

    throw MerkError("Cloned Variable");
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

VarNode::VarNode(Node startingValue, DataTypeFlags flags) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    auto definedType = flags.type;
    validateTypes(definedType, startingValue);
    staticType = nodeTypeToString(flags.type);


    if (!startingValue.getFlags().isMutable) {
        // auto cloned = startingValue.clone();
        // if (startingValue.isInstance() && !cloned.isInstance()) {
        //     valueNode = cloned;
        //     // valueNode.getFlags().merge({{"isInstance", "true"},{"name", varFlags.name}, {"isMutable", flags.isMutable ? "true" : "false"}, {"isConst", flags.isConst ? "true" : "false"}, {"type", nodeTypeToString(flags.type)}, {"fullType", flags.fullType.getBaseType()}});
        // }
        // else {
        //     valueNode = cloned;
        // }
        valueNode = startingValue.clone();
        
        
    } else {
        valueNode = startingValue;
    }
    
    if (valueNode.isNull() && !valueNode.isValid()) {throw MerkError("Set VarNode to Null Value");}  

    DEBUG_LOG(LogLevel::PERMISSIVE, "Starting Value BEFORE Merge");
    printTypeComparison(startingValue, flags);

    DEBUG_LOG(LogLevel::PERMISSIVE, "ValueNode Value BEFORE Merge");
    printTypeComparison(valueNode, flags);
    
    valueNode.getFlags().merge({
        {"name", varFlags.name.size() > 0 ? varFlags.name : flags.name}, 
        {"type", nodeTypeToString(flags.type)}, 
        {"fullType", flags.fullType.getBaseType()},
        {"isInstance", (startingValue.isInstance() ? "true" : "false")}, 
        {"isMutable", flags.isMutable ? "true" : "false"}, 
        {"isConst", flags.isConst ? "true" : "false"}
    });

    DEBUG_LOG(LogLevel::PERMISSIVE, "Starting Value AFTER Merge");
    printTypeComparison(startingValue, flags);

    DEBUG_LOG(LogLevel::PERMISSIVE, "ValueNode Value AFTER Merge");
    printTypeComparison(valueNode, flags);

    // throw MerkError("Entered: VarNode::VarNode(Node startingValue, DataTypeFlags flags)");
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

Node::Node(SharedPtr<DictNode> v) {
    // throw MerkError("Attempted ListNode construction");
    // setValue(v);
    // throw MerkError("Node Constructing DictNode");
    data = v;
    getFlags().type = NodeValueType::Dict;
    getFlags().fullType.setBaseType("Dict");
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

Node& Node::operator=(const Node& other) {
    

    data = other.data;

    if (other.isInstance()) {
        // auto type = DynamicNode::getTypeFromValue(other.getValue());
        // other.getFlags().merge({{"isInstance", "true"}});
        // DEBUG_LOG(LogLevel::PERMISSIVE, "Assigning Instance to Node, it holds a " + nodeTypeToString(type) + " and holds meta of " + other.getFlags().toString());
        data->flags.merge({{"isInstance", "true"}});
        // *this = ClassInstanceNode(std::get<SharedPtr<Callable>>(other.getValue()));
    } else {
        data->flags = other.getFlags();
    }
    
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