#include "core/node/node.h"
#include "core/node/node_structures.h"
#include "core/types.h"
#include "core/scope.h"

#include "utilities/debugger.h"
#include "utilities/debugging_functions.h"

#include "core/callables/classes/class_base.h"
#include "core/callables/functions/function.h"


// DynamicNode
NodeValueType DynamicNode::inferTypeFromString (String& valueStr, String& typeStr) {
    if (typeStr == "Variable" || typeStr == "Argument" || typeStr == "FunctionCall" || typeStr == "ClassMethodCall" || typeStr == "Parameter") {
        return NodeValueType::String;
    } 

    if (typeStr == "Number") {
        try {
            // Check if it has a decimal point or scientific notation
            if (valueStr.find('.') != String::npos || valueStr.find('e') != String::npos || valueStr.find('E') != String::npos) {
                double doubleValue = std::stod(valueStr); // Try converting to double
                if (doubleValue >= -std::numeric_limits<float>::max() &&
                    doubleValue <= std::numeric_limits<float>::max()) {
                    return NodeValueType::Float; // Fits within float range
                }
                return NodeValueType::Double; // Otherwise, it's a double
            }

            // Otherwise, try integer conversions
            long longValue = std::stol(valueStr); // Try converting to long
            if (longValue >= std::numeric_limits<int>::min() &&
                longValue <= std::numeric_limits<int>::max()) {
                return NodeValueType::Int; // Fits within int range
            }
            return NodeValueType::Long; // Otherwise, it's a long
        } catch (const std::exception&) {
            throw MerkError("Invalid numeric string: " + valueStr);
        }
    } else if (typeStr == "String") {
        return NodeValueType::String;
    } else if (typeStr == "Bool") {
        return NodeValueType::Bool;
    } else if (typeStr == "Null") {
        return NodeValueType::Null;
    } else if (typeStr == "Char") {
        return NodeValueType::Char;
    }

    throw MerkError("Unknown type string: " + typeStr);

        // DEBUG_LOG(LogLevel::DEBUGC, "Set initial value with String. Type: ", static_cast<int>(data.type), ", Value: ", data.value);
}
NodeValueType DynamicNode::getNodeTypeFromString(String& type) {
    DEBUG_FLOW(FlowLevel::MED);

    auto nodeType = stringToNodeType(type);
    if (nodeType == NodeValueType::UNKNOWN) {
        throw MerkError("Cannot Have An UNKNOWN TYPE Passed Back");
    }
    DEBUG_FLOW_EXIT();
    return nodeType;
}
std::pair<VariantType, NodeValueType> DynamicNode::getCoercedStringAndType(const String& value, String& typeStr) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (value == "Variable" || value == "Argument" || value == "FunctionCall" || value == "ClassMethodCall" || value == "Parameter") {
        DEBUG_FLOW_EXIT();
        return {value, NodeValueType::String};
    }

    

    NodeValueType type = getNodeTypeFromString(typeStr);

    if (typeStr == "Number") { if (type != NodeValueType::Number) {throw MerkError("Not A Number");} }
    
    try {
        switch (type) {
            case NodeValueType::Number: {
                if (type == NodeValueType::Number) {
                    if (value.find('.') != String::npos) {
                        return { std::stod(value), NodeValueType::Double };

                    } else {
                        return { std::stoi(value), NodeValueType::Int };
                    }
                } else {
                    throw MerkError("Failed the number check");
                }

                if (value.size() == 3 && value.front() == '\'' && value.back() == '\'') {
                    return { value[1], NodeValueType::Char };
                }

                if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                    DEBUG_LOG(LogLevel::PERMISSIVE, "Casting Number To Int For Some Reason");
                    return { value.substr(1, value.size() - 2), NodeValueType::String };
                }

                else {throw MerkError("Could Not Convert Number to a Number Type");}
            }
            case NodeValueType::String: return { value, type };
            case NodeValueType::Int: return { std::stoi(value), type };
            case NodeValueType::Float: return { std::stof(value), type };
            case NodeValueType::Double: return { std::stod(value), type };
            case NodeValueType::Long: return { std::stol(value), type };
            case NodeValueType::Bool:
                if (value == "true") return { true, type };
                if (value == "false") return { false, type };
                throw MerkError("Invalid boolean: " + value);
            case NodeValueType::Char:
                if (value.size() == 3 && value.front() == '\'' && value.back() == '\'') {return { value[1], type };}
                else if (value.size() == 1) { return {value[0], type }; }

                throw MerkError("Invalid char: " + value);
            
            default:
                throw MerkError("Unsupported coercion target type: " + nodeTypeToString(type));
        }
    } catch (const std::exception& e) {
        throw MerkError("Failed to coerce value '" + value + "' to type " + nodeTypeToString(type));
    }
}
std::pair<VariantType, NodeValueType> DynamicNode::validateAndCopy(const VariantType& value, NodeValueType type) {
    auto visitor = [&](auto&& arg) -> std::pair<VariantType, NodeValueType> {
        using T = std::decay_t<decltype(arg)>;
        if (getNodeTypeFromType<T>() != type) {
            throw MerkError("Value does not match declared NodeValueType: expected " + nodeTypeToString(type));
        }
        return { arg, type };
    };
    return std::visit(visitor, value);
}

NodeValueType DynamicNode::getTypeFromValue(const VariantType& value) {
    DEBUG_FLOW(FlowLevel::NONE);
    auto val = std::visit([](auto&& arg) -> NodeValueType {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, NumberVariant>) {
            return std::visit([](auto&& num) {
                using N = std::decay_t<decltype(num)>;
                return getNodeTypeFromType<N>();
            }, arg);
        }
        else if constexpr (std::is_same_v<T, TextVariant>) {
            return std::visit([](auto&& txt) {
                using S = std::decay_t<decltype(txt)>;
                return getNodeTypeFromType<S>();
            }, arg);
        }
        else {
            return getNodeTypeFromType<T>();
            
        }
    }, value);

    DEBUG_LOG(LogLevel::TRACE, "GOT TYPE " + nodeTypeToString(val) + " In DynamicNode::getTypeFromValue");
    // if (val == NodeValueType::Null) {throw MerkError(" DynamicNode::getTypeFromValue returned NULL");}
    if (val == NodeValueType::DataStructure || val == NodeValueType::Dict) {
        throw MerkError("Got " + nodeTypeToString(val) + " In DynamicNode::getTypeFromValue");
    }
    DEBUG_FLOW_EXIT();
    if (val == NodeValueType::DataStructure || val == NodeValueType::Dict || val == NodeValueType::List) {throw MerkError("DynamicNode::getTypeFromValue -> DataStructure Found");}
    // if (val == NodeValueType::Callable) { throw MerkError("DynamicNode::getTypeFromValue -> Callable"); }
    return val;
}


String DynamicNode::forceToString(VariantType value) {
    if (std::holds_alternative<String>(value)) {return std::get<String>(value);}
    NodeValueType type = getTypeFromValue(value);
    switch (type)
    {
    case NodeValueType::Int: return std::to_string(std::get<int>(value));
    case NodeValueType::Float: return std::to_string(std::get<float>(value));
    case NodeValueType::Long: return std::to_string(std::get<long>(value));
    case NodeValueType::Char: return std::to_string(std::get<char>(value));
    case NodeValueType::Bool: return std::get<bool>(value) ? "true" : "false";
    case NodeValueType::Double: return std::to_string(std::get<double>(value));
    case NodeValueType::Text: return std::get<String>(value);
    case NodeValueType::Number: return std::to_string(std::get<float>(value));
    case NodeValueType::Null: throw MerkError("Output will be null in DynamicNode::forceToString"); return "null";
    case NodeValueType::Uninitialized: return "Uninitialized";
    case NodeValueType::Function: return "Function";
    case NodeValueType::DataStructure: throw MerkError("DynamicNode::forceToString FOUND DictNode"); return std::get<SharedPtr<NativeNode>>(value)->toString();
    case NodeValueType::Callable: return std::get<SharedPtr<Callable>>(value)->toString();
    case NodeValueType::ClassInstance: return std::get<SharedPtr<Callable>>(value)->toString();

    default: throw MerkError("Type: " + nodeTypeToString(type) + " is not yet supported for forceToString"); }
}

SharedPtr<NodeBase> DynamicNode::dispatch(VariantType val, NodeValueType type, bool coerce) {
    (void)coerce;
    DEBUG_FLOW(FlowLevel::MED);

    if (type == NodeValueType::UNKNOWN) {
        if (std::holds_alternative<String>(val)) {
            auto actualVal = std::get<String>(val);
            if (actualVal == "Variable" || actualVal == "Argument" || actualVal == "FunctionCall" || actualVal == "ClassMethodCall" || actualVal == "Parameter") {
                return makeShared<StringNode>(actualVal);
            } else {throw MerkError("Type Unknwon Is Not Of the Exceptions");}
        } else {throw MerkError("Cannot Handle Type UNKNWON, valType: " + nodeTypeToString(DynamicNode::getTypeFromValue(val)));}
    }
    

    auto valType = getTypeFromValue(val);
    auto passedIsANumber = type == NodeValueType::Int || type == NodeValueType::Number || type == NodeValueType::Long || type == NodeValueType::Double || type == NodeValueType::Float;
    if (valType == type || (valType == NodeValueType::Number && passedIsANumber) || type == NodeValueType::ClassInstance) {
        switch (type)
        {
        case NodeValueType::Number:
            {
                switch (valType)
                {
                case NodeValueType::Int:
                    DEBUG_FLOW_EXIT();
                    return makeShared<IntNode>(std::get<int>(val));
                
                default:
                    throw MerkError("Cannot Cast " + nodeTypeToString(valType) + " to " + nodeTypeToString(type));
                }
            }
        case NodeValueType::Int:
            DEBUG_FLOW_EXIT();
            return makeShared<IntNode>(std::get<int>(val));
        case NodeValueType::String:
            DEBUG_FLOW_EXIT();
            if (!std::holds_alternative<String>(val)) {
                return makeShared<StringNode>(forceToString(val));
            }
            return makeShared<StringNode>(std::get<String>(val));

        case NodeValueType::Bool:
            DEBUG_FLOW_EXIT();
            return makeShared<BoolNode>(std::get<bool>(val));
        case NodeValueType::Double:
            DEBUG_FLOW_EXIT();
            return makeShared<DoubleNode>(std::get<double>(val));
        case NodeValueType::Float:
            DEBUG_FLOW_EXIT();
            return makeShared<FloatNode>(std::get<float>(val));
        case NodeValueType::DataStructure:
        case NodeValueType::Dict:
            throw MerkError("DynamicNode::dispatch found Dict | DataStructure");
        case NodeValueType::Callable:
            {
                auto call = std::get<SharedPtr<Callable>>(val);
                // return makeShared<CallableNode>(call);
                throw MerkError("Tried to create Callable " + call->toString());
            }
            
        // case NodeValueType::DataStructure:
        //     {
        //         DEBUG_FLOW_EXIT();
        //         auto data = std::get<SharedPtr<DataStructure>>(val);
        //         return makeShared<DynamicNode>(std::get<SharedPtr<DataStructure>>(val));
        //         // if (data->getType() == NodeValueType::List) {
        //         //     throw MerkError("Got A List");
        //         // }

        //         // throw MerkError("Found Another DataStructure, Need to disambiguate it");
        //     }
            

        
        default:
            DEBUG_FLOW_EXIT();
            throw MerkError("Not A Currently Supported Type: " + nodeTypeToString(type));
        }
    }

    // else if (coerce) {
    //     switch (valType)
    //     {
    //     case NodeValueType::String:
    //         if (type == NodeValueType::Number) {}
    //         /* code */
    //         break;
        
    //     default:
    //         break;
    //     }
    // }



    DEBUG_FLOW_EXIT();
    throw MerkError("Not A supported Type " + nodeTypeToString(valType) + ". Tried to dispatch to: " + nodeTypeToString(type));
}
Node DynamicNode::dispatchNode(VariantType value, String typeStr, bool coerce) {
    DEBUG_FLOW(FlowLevel::MED);

    auto nodeType = DynamicNode::getNodeTypeFromString(typeStr);
    if (nodeType == NodeValueType::UNKNOWN) {throw MerkError("The Type of " + nodeTypeToString(nodeType, false) + " for Starting Lit Value Is: " + DynamicNode::forceToString(value));}
    auto nodeBase = DynamicNode::dispatch(value, nodeType, coerce);
    auto node = Node(nodeBase);
    DEBUG_FLOW_EXIT();
    return node;
}


VariantType DynamicNode::getValue() const { return value; }
void DynamicNode::setValue(const VariantType& v)  {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    auto type = DynamicNode::getTypeFromValue(v);
    value = v;
    flags.type = type;
    flags.fullType.setBaseType(nodeTypeToString(type, false));
    DEBUG_FLOW_EXIT();
}

NodeValueType DynamicNode::getType() const { return NodeValueType::Any; }
SharedPtr<NodeBase> DynamicNode::clone() const {
    if (std::holds_alternative<SharedPtr<Callable>>(getValue())) { throw MerkError("Clone called on an DynamicNode holding an instance"); }

    if (std::holds_alternative<SharedPtr<Callable>>(getValue())) {
        throw MerkError("Cloning a Callable");
        auto instance = std::static_pointer_cast<ClassInstance>(std::get<SharedPtr<Callable>>(getValue()));
        auto clonedInstance = instance->cloneInstance();
        clonedInstance->setNativeData(instance->getNativeData() ? std::static_pointer_cast<NativeNode>(instance->getNativeData()->clone()) : nullptr);
        auto data = clonedInstance->getInstanceNode()->getValue();
        if (std::get<SharedPtr<NativeNode>>(data)) {
            throw MerkError("Found the Missing Data Structure 1");
        } else {
            throw MerkError("1 Actually found " + nodeTypeToString(DynamicNode::getTypeFromValue(data)));
        }
        auto node = makeShared<DynamicNode>(instance->clone());
        node->flags = flags;
        return node;
    } 
    else if (std::holds_alternative<SharedPtr<NativeNode>>(getValue())) {
        throw MerkError("Cloning a DataStructure");
    }

    throw MerkError("Calling Clone on DynamicNode holding " + nodeTypeToString(DynamicNode::getTypeFromValue(getValue())));
    auto node = makeShared<DynamicNode>(*this); 
    node->flags = flags;
    return node;
}
 
int DynamicNode::toInt() const {  
    if (isInt()) {
        return std::get<int>(value);
    } else if (isBool()) {
        return toBool();
    } else if (isNumeric()) {
        // when other types are added
        throw MerkError("Need To Handle Numeric Types in DynamicNode::toInt()");
    }

    throw MerkError("Cannnot cast " + nodeTypeToString(getType()) + " to " + "Int");
}


bool DynamicNode::isString() const {return std::holds_alternative<String>(value);}
bool DynamicNode::isBool() const {return std::holds_alternative<bool>(value);}
bool DynamicNode::isValid() const { return DynamicNode::getTypeFromValue(value) == getNodeType(); }


String DynamicNode::toString() const { return DynamicNode::forceToString(value); }
bool DynamicNode::toBool() const { 
    NodeValueType ofThis = DynamicNode::getTypeFromValue(value);
    auto node = DynamicNode::dispatchNode(value, nodeTypeToString(ofThis, false));
    return node.toBool();
}

bool DynamicNode::isNumeric() const { return isInt() || isFloat() || isDouble();}
bool DynamicNode::isInt() const {return std::holds_alternative<int>(value);}




// DynamicNode




// virtual type checkers
int NodeBase::toInt() const { throw MerkError("Not an Int"); }
String NodeBase::toString() const { throw MerkError("Not a String"); } 
bool NodeBase::toBool() const { return false; }
bool NodeBase::isValid() const { return false; }
bool NodeBase::isNumeric() const { throw MerkError("isNumeric Called On NodeBase"); }


SharedPtr<NodeBase> Node::getInner() { return data; }
SharedPtr<NodeBase> Node::getInner() const { return data; }
bool NodeBase::isString() const { return false; }
bool NodeBase::isInt() const { return false; }
bool NodeBase::isBool() const { return false; }
bool NodeBase::isTruthy() const { return false; }
bool NodeBase::isNull() const {throw MerkError("isNull called on NodeBase"); return true;}

bool NodeBase::isDouble() const { return false; }
double NodeBase::toDouble() const {throw MerkError("Not a Double"); }

bool NodeBase::isFloat() const { return false; }
float NodeBase::toFloat() const { throw MerkError("Not A Float"); }

void NodeBase::setFlags(DataTypeFlags newOnes) {
    flags = newOnes;
}


std::size_t NodeBase::hash() const {
    std::size_t h1 = std::hash<int>()(static_cast<int>(getNodeType()));
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
    }, getValue());

    std::size_t h3 = std::hash<std::string>()(flags.fullType.getBaseType());
    std::size_t h4 = (flags.isConst << 1) ^ (flags.isMutable << 2) ^ (flags.isStatic << 3);

    return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
}

NullNode::NullNode() {
    value = Null;
    flags.type = NodeValueType::Null;
    flags.fullType.setBaseType("Null");
    instantiatedWithNull = true;
}
NullNode::NullNode(NullType v) {
    if (DynamicNode::getTypeFromValue(v) != NodeValueType::Null) {
        throw MerkError("Tried Instantiating a NullNode from Null, but of type " + nodeTypeToString(DynamicNode::getTypeFromValue(v)) + " Onto NullNode");
    }
    flags.type = NodeValueType::Null;
    flags.fullType.setBaseType("Null");
    instantiatedWithNull = true;
    value = Null;
}

NullNode::NullNode(VariantType v) {
    if (std::holds_alternative<NullType>(v)) {
        value = std::get<NullType>(v);
    }
    throw MerkError("Tried Creating a NullNode from " + nodeTypeToString(DynamicNode::getTypeFromValue(v)));
}

VariantType NullNode::getValue() const {
    return value;
}
void NullNode::setValue(const VariantType& v) {

    if (std::holds_alternative<NullType>(v)) {
        value = std::get<NullType>(v);
    }
    throw MerkError("Tried Setting NullNode to " + nodeTypeToString(DynamicNode::getTypeFromValue(v)));
};

NodeValueType NullNode::getType() const {return DynamicNode::getTypeFromValue(getValue());}
SharedPtr<NodeBase> NullNode::clone() const {
    return makeShared<NullNode>(*this);
}

int NullNode::toInt() const { throw MerkError("Cannot Convert Null to Int"); }

String NullNode::toString() const {return "null"; };

bool NullNode::toBool() const { return false; };

bool NullNode::isValid() const { return instantiatedWithNull; }

bool NullNode::isNull() const { return true; }
bool NullNode::isNumeric() const { return false; }
void NullNode::clear() {};



SharedPtr<ClassInstance> Node::toInstance() {
    if (std::holds_alternative<SharedPtr<Callable>>(getValue())) {
        auto callable = std::get<SharedPtr<Callable>>(getValue());
        auto instance = std::static_pointer_cast<ClassInstance>(callable);
        return instance;
    } else if (getInner()->getType() == NodeValueType::Callable || getInner()->getType() == NodeValueType::ClassInstance) {
        return std::static_pointer_cast<ClassInstance>(getInner());
    }
    throw MerkError("Cannot Cast A simple Node to an Instance");
}
SharedPtr<ClassInstance> Node::toInstance() const {
    if (std::holds_alternative<SharedPtr<Callable>>(getValue())) {
        auto callable = std::get<SharedPtr<Callable>>(getValue());
        auto instance = std::static_pointer_cast<ClassInstance>(callable);
        return instance;
    } 

    else if (getInner()->getType() == NodeValueType::Callable || getInner()->getType() == NodeValueType::ClassInstance) {
        // throw MerkError("The Item Gotten is a " + nodeTypeToString(getInner()->getType()));
        return std::static_pointer_cast<ClassInstance>(getInner());
    }

    throw MerkError("Cannot Cast A simple Node to an Instance");
}

bool Node::isFunctionNode() {
    return std::holds_alternative<Vector<SharedPtr<CallableSignature>>>(getValue());
}
bool Node::isFunctionNode() const {
    return std::holds_alternative<Vector<SharedPtr<CallableSignature>>>(getValue());
}

bool Node::isCallable() {
    return getInner()->getType() == NodeValueType::Callable || (std::holds_alternative<SharedPtr<Callable>>(getValue()) && getType() != NodeValueType::ClassInstance);
    return std::holds_alternative<SharedPtr<Callable>>(getValue()) && getType() != NodeValueType::ClassInstance;
}
bool Node::isCallable() const {return getInner()->getType() == NodeValueType::Callable || (std::holds_alternative<SharedPtr<Callable>>(getValue()) && getType() != NodeValueType::ClassInstance); return std::holds_alternative<SharedPtr<Callable>>(getValue()) && getType() != NodeValueType::ClassInstance; }

SharedPtr<Callable> Node::toCallable() {
    if (isCallable()) {
        return std::get<SharedPtr<Callable>>(getValue());
    }

    throw MerkError("Current Value is not a Callable, but a " + nodeTypeToString(DynamicNode::getTypeFromValue(getValue())));
}
SharedPtr<Callable> Node::toCallable() const {
    if (isCallable()) {
        return std::get<SharedPtr<Callable>>(getValue());
    }

    throw MerkError("Current Value is not a Callable, but a " + nodeTypeToString(DynamicNode::getTypeFromValue(getValue())));
}


FunctionNode Node::toFunctionNode() {
    if (isFunctionNode()) {
        return FunctionNode(getFlags().name, std::get<Vector<SharedPtr<CallableSignature>>>(getValue()));
    }

    throw MerkError("Cannot Cast A Simple Node To A FUnctionNode");
}
FunctionNode Node::toFunctionNode() const {
    if (isFunctionNode()) {
        return FunctionNode(getFlags().name, std::get<Vector<SharedPtr<CallableSignature>>>(getValue()));
    }

    throw MerkError("Cannot Cast A Simple Node To A FUnctionNode");
}

void Node::setFlags(DataTypeFlags newOnes) { data->setFlags(newOnes); }

VariantType Node::getValue() const { return data->getValue(); }
void Node::setValue(const VariantType& v) {
    if (!data) {
        data = makeShared<DynamicNode>(v);
        return;
    }

    if (auto nullVar = dynamic_cast<NullNode*>(data.get())) {
        if (nullVar->instantiatedWithNull && !std::holds_alternative<NullType>(v)) {
            data = makeShared<DynamicNode>(v);
            return;
        }
    }

    data->setValue(v); 
}

NodeValueType Node::getType() const { return data->getType(); }
Node Node::clone() const {

    if (isInstance()) {
        return Node(toInstance()->clone());
    }
    if (std::holds_alternative<SharedPtr<NativeNode>>(getValue())) {
        throw MerkError("Currently Holding A NativeNode");
    }

    if (std::holds_alternative<std::unordered_map<Node, Node>>(getValue())) {
        throw MerkError("Currently Holding A Map");
    }

    if (std::holds_alternative<std::map<Node, Node>>(getValue())) {
        throw MerkError("Currently Holding A Map");
    }
    DEBUG_LOG(LogLevel::PERMISSIVE, "----------->>>>>>>>>>>>>++++++++++++++++++++++++++++++++++++++++++++ HOLDING A ", nodeTypeToString(DynamicNode::getTypeFromValue(getValue())));
    return Node(data->clone()); 
}

bool Node::isNumeric() const { return data->isNumeric(); }
int Node::toInt() const { return data->toInt(); }
String Node::toString() const { 
    if (isFunctionNode()) {
        return "<FunctionRef " + std::to_string(std::get<Vector<SharedPtr<CallableSignature>>>(getValue()).size()) + " overload(s)>"; 
    } else if (isDict()) {
        throw MerkError("Trying to represent a Dict");
    } else if (isList()) {
        throw MerkError("Trying to represent a List");
    } else if (isInstance()) {
        // throw MerkError("Trying to represent an Instance");
        return toInstance()->toString();
    }
    // DEBUG_LOG(LogLevel::PERMISSIVE, "DATA: ", getFlags().toString());
    return data->toString(); 
}
bool Node::toBool() const { return data->toBool(); }

bool Node::isBool() const {return data->isBool();}
bool Node::isInt() const {return data->isInt();}
bool Node::isString() const {return data->isString();}


bool Node::isInstance() const {
    return getFlags().isInstance;
}

bool Node::isInstance() {
    return getFlags().isInstance;
}

bool Node::isList() const {return data && data->isList();}
bool Node::isArray() const {return data && data->isArray();}
bool Node::isDict() const {return data && data->isDict();}
bool Node::isSet() const {return data && data->isSet();}
bool Node::isFloat() {return data->isFloat();}
bool Node::isFloat() const {return data->isFloat();}
bool Node::isDouble() {return data->isDouble();}
bool Node::isDouble() const {return data->isDouble();}

float Node::toFloat() const {return data->toFloat(); }
double Node::toDouble() const { return data->toDouble(); }


SharedPtr<ListNode> Node::toList() const {
    if (std::holds_alternative<SharedPtr<NativeNode>>(getValue())) {
        auto native = std::get<SharedPtr<NativeNode>>(getValue());
        auto list = std::static_pointer_cast<ListNode>(native);
        return list;
    }
    // else if (std::holds_alternative<SharedPtr<ClassInstance>>(getValue())) {
    //     throw MerkError("The Data Contained within is a classInstance");
    // }
    throw MerkError("Node Is Not A NativeNode, so not a list");
}
SharedPtr<ArrayNode> Node::toArray() const {
    if (std::holds_alternative<SharedPtr<NativeNode>>(getValue())) {
        auto native = std::get<SharedPtr<NativeNode>>(getValue());
        auto list = std::static_pointer_cast<ArrayNode>(native);
        return list;
    }
    throw MerkError("Node Is Not A NativeNode, so not an array");
}

SharedPtr<DictNode> Node::toDict() const {
    if (std::holds_alternative<SharedPtr<NativeNode>>(getValue())) {
        auto native = std::get<SharedPtr<NativeNode>>(getValue());
        auto list = std::static_pointer_cast<DictNode>(native);
        return list;
    }
    else if (std::holds_alternative<SharedPtr<Callable>>(getValue())) {
        auto inst = toInstance();
        if (inst) {
            if (inst->getNativeData()) {
            return std::static_pointer_cast<DictNode>(inst->getNativeData());
        }
            if (inst) {throw MerkError("This is an instance with data of " + inst->toString());}
            throw MerkError("The Data Contained within is a classInstance, not a Dict");
        }
        
    }
    throw MerkError("Node Is Not A NativeNode, so not a dict, but a " + nodeTypeToString(DynamicNode::getTypeFromValue(getValue())));
}
SharedPtr<SetNode> Node::toSet() const {
    if (std::holds_alternative<SharedPtr<NativeNode>>(getValue())) {
        auto native = std::get<SharedPtr<NativeNode>>(getValue());
        auto list = std::static_pointer_cast<SetNode>(native);
        return list;
    }
    throw MerkError("Node Is Not A NativeNode, so not a set");
}

bool Node::isString() {return data->isString();}
bool Node::isInt() {return data->isInt();}
bool Node::isNull() const {return std::holds_alternative<NullType>(data->getValue());}

bool Node::isValid() const { return data != nullptr; }
bool Node::isTruthy() const { return data && data->isTruthy(); }
Node Node::negate() const {return Node(!toBool());}




DataTypeFlags& Node::getFlags() { return data->flags; }
const DataTypeFlags& Node::getFlags() const { return data->flags; }


void Node::clear() { if (data) {data->clear();} }
std::size_t Node::hash() const {
    if (data) {
        return data->hash();
    }
    return Null.hash();
}


// BoolNode

void BoolNode::clear() {}
bool BoolNode::isInt() const {return false;}
bool BoolNode::isBool() const { return true; }
bool BoolNode::isValid() const {return true;}
bool BoolNode::isNumeric() const {return isInt();}


VariantType BoolNode::getValue() const { return value; }
void BoolNode::setValue(const VariantType& v)  { value = std::get<bool>(v); }

NodeValueType BoolNode::getType() const { return NodeValueType::Bool; }
SharedPtr<NodeBase> BoolNode::clone() const { return makeShared<BoolNode>(*this); }

int BoolNode::toInt() const {  throw MerkError("Cannot Implicitly cast String to Int"); }
String BoolNode::toString() const { return value ? "true" : "false"; }
bool BoolNode::toBool() const { return value; }
bool BoolNode::isTruthy() const { return toBool(); }



// BoolNode

bool StringNode::isNumeric() const {return false;}
bool StringNode::isString() const { return true; }
bool StringNode::isValid() const {return true;}
bool StringNode::isBool() const {return value == "true" || value == "false" || value == "0" || value == "1";}
bool StringNode::isTruthy() const {return !value.empty() || value == "true" || value == "1" || value != "null";}
// StringNode

VariantType StringNode::getValue() const { return value; }
void StringNode::setValue(const VariantType& v)  { value = std::get<String>(v); }

NodeValueType StringNode::getType() const { return NodeValueType::String; }
SharedPtr<NodeBase> StringNode::clone() const { return makeShared<StringNode>(*this); }

int StringNode::toInt() const {  throw MerkError("Cannot Implicitly cast String to Int"); }
String StringNode::toString() const { return String(value); }
bool StringNode::toBool() const { return !value.empty(); }


// StringNode



// IntNode



VariantType IntNode::getValue() const { return value; }
void IntNode::setValue(const VariantType& v)  { value = std::get<int>(v); }

NodeValueType IntNode::getType() const { return NodeValueType::Int; }
SharedPtr<NodeBase> IntNode::clone() const { return makeShared<IntNode>(*this); }

bool IntNode::isInt() const {return true;}
bool IntNode::isNumeric() const {return true;}
bool IntNode::isBool() const {return false;}

int IntNode::toInt() const { return value; }
float IntNode::toFloat() const {return float(value);}
double IntNode::toDouble() const {return double(value);}

String IntNode::toString() const { return std::to_string(value); }
bool IntNode::toBool() const { return value != 0; }
bool IntNode::isValid() const {return true;}
bool IntNode::isTruthy() const { return toBool(); }




VariantType FloatNode::getValue() const {
    return value;
}
void FloatNode::setValue(const VariantType& v) {
    if (std::holds_alternative<float>(v)) {
        value = std::get<float>(v);
        return;
    }

    throw MerkError("Cannot convert type: " + nodeTypeToString(DynamicNode::getTypeFromValue(v)) + " to Float");
}

NodeValueType FloatNode::getType() const {return NodeValueType::Float; }
SharedPtr<NodeBase> FloatNode::clone() const { return makeShared<FloatNode>(value); }

int FloatNode::toInt() const {return int(value);}
double FloatNode::toDouble() const {return double(value);}
float FloatNode::toFloat() const { return value; }
bool FloatNode::isFloat() const { return true; }

String FloatNode::toString() const {return std::to_string(value);};
bool FloatNode::toBool() const { return value != 0; }
bool FloatNode::isTruthy() const { return toBool(); }
bool FloatNode::isValid() const { return toBool(); }
bool FloatNode::isNumeric() const { return true; }

void FloatNode::clear() {}



// DOUBLE 



VariantType DoubleNode::getValue() const { return value; }
void DoubleNode::setValue(const VariantType& v) {
    if (std::holds_alternative<double>(v)) {
        value = std::get<double>(v);
        return;
    }
    throw MerkError("Cannot Set Type " + nodeTypeToString(DynamicNode::getTypeFromValue(v)) + " To Double");
}

NodeValueType DoubleNode::getType() const { return NodeValueType::Double; }
SharedPtr<NodeBase> DoubleNode::clone() const { return makeShared<DoubleNode>(value); }

int DoubleNode::toInt() const { return int(value); }
double DoubleNode::toDouble() const { return value; }
float DoubleNode::toFloat() const {return float(value); }
bool DoubleNode::isDouble() const {return true; }

String DoubleNode::toString() const {return std::to_string(value); }
bool DoubleNode::toBool() const { return value != 0; }
bool DoubleNode::isTruthy() const { return toBool(); }
bool DoubleNode::isValid() const { return toBool(); }
bool DoubleNode::isNumeric() const { return true; }
void DoubleNode::clear() {}



// DOUBLE 



// Node Wrapper

Node& NodeWrapper::getValueNode() {return valueNode;}
const Node& NodeWrapper::getValueNode() const {return valueNode;}
const DataTypeFlags& NodeWrapper::getFlags() const {return valueNode.getFlags();}
String NodeWrapper::toString() const {
    return valueNode.isInstance() ? valueNode.toInstance()->toString() : valueNode.toString();
}

int NodeWrapper::toInt() const {return valueNode.toInt();}
bool NodeWrapper::toBool() const {return valueNode.toBool();}

void NodeWrapper::setValue(Node other) { valueNode = other; if (!valueNode.isValid()) {throw MerkError("ValueNode Was Not Properly Set");}}
bool NodeWrapper::getIsMutable() {return valueNode.getFlags().isMutable;}
bool NodeWrapper::getIsStatic() {return valueNode.getFlags().isStatic;}
bool NodeWrapper::getIsConst() {return valueNode.getFlags().isConst;}
bool NodeWrapper::isBool() const {return valueNode.isBool();}
bool NodeWrapper::isInt() const {return valueNode.isInt();}
bool NodeWrapper::isString() const {if (isValid()) { return valueNode.isString(); } else {throw MerkError("NodeWrapper::toString() -> Cannot Validate String Because Node is Invalid");}}
void NodeWrapper::clear() { valueNode.clear(); }

bool NodeWrapper::isList() const {return valueNode.isList();}
bool NodeWrapper::isArray() const {return valueNode.isArray();}
bool NodeWrapper::isDict() const {return valueNode.isDict();}
bool NodeWrapper::isSet() const {return valueNode.isSet();}
bool NodeWrapper::isValid() {return valueNode.isValid();}
bool NodeWrapper::isValid() const {return valueNode.isValid();}
bool NodeWrapper::isNumeric() const { return valueNode.isNumeric(); }
bool NodeWrapper::isTruthy() const {return valueNode.isTruthy(); }
void NodeWrapper::setFlags(DataTypeFlags newOnes) { valueNode.setFlags(newOnes); }
// Node Wrapper




String VarNode::varString() const { return NodeWrapper::toString() + (!varFlags.name.empty() ? ", Var: " + varFlags.name : "");}


void VarNode::setValue(Node other) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (std::holds_alternative<NullType>(other.getValue()) || other.getType() == NodeValueType::Null) {
        if (other.getType() == NodeValueType::Null && !std::holds_alternative<NullType>(other.getValue())) {
            throw MerkError("Tried Setting value typed as null, but actually isn't. It is type " + nodeTypeToString(DynamicNode::getTypeFromValue(other.getValue())));
        }

        throw MerkError("Tried Setting Var " + varFlags.name + " to Null, Value Is: " + DynamicNode::forceToString(other.getValue()) + " " + other.getFlags().toString() + " "  + other.getInner()->toString());
    }
    if (!other.isValid()) { throw MerkError("Other is not valid"); }

    if (varFlags.isConst) { throw MerkError("Assignment to const variable"); }
    // if (varFlags.fullType.getBaseType() != "Any" && other.getType() != varFlags.type) {
    //     throw MerkError("Type mismatch: expected " + varFlags.fullType.getBaseType() + " | " + nodeTypeToString(varFlags.type));
    // }
    auto otherFlags = other.getFlags();
    if (varFlags.type != NodeValueType::Any) {
        if (otherFlags.type != varFlags.type) {
            String expected = "Expected: " + nodeTypeToString(varFlags.type) + " | " + varFlags.fullType.toString();
            String got = "Got " + nodeTypeToString(otherFlags.type) + " | " + otherFlags.fullType.toString(); 
            throw MerkError("Type Mismatch from declaration: " + expected + ", But " + got);
        }
    }  

    if (other.getNodeType() == NodeValueType::Uninitialized) { throw MerkError("Parameter being set");  }

    // Keep the variable's declaration flags intact
    auto varName = varFlags.name;
    auto varType = varFlags.fullType.getBaseType();

    // Assign the value
    valueNode = other;
    if (!other.isValid()) {throw MerkError("Other Is Invalid");}

    // Merge variable metadata into the value's flags without overwriting varFlags
    auto mergedFlags = valueNode.getFlags().merge({
        {"key", varName},
        {"isMutable", varFlags.isMutable ? "true" : "false"},
        {"isStatic", varFlags.isStatic ? "true" : "false"},
        {"name", valueNode.getFlags().name.empty() ? varName : valueNode.getFlags().name},
        {"fullType", varType}
    });
    valueNode.setFlags(mergedFlags);

    // Don't touch varFlags except maybe to update its type if it's 'Any'
    if (varFlags.fullType.getBaseType() == "Any") {
        varFlags.type = valueNode.getType();
    }

    if (std::holds_alternative<NullType>(getValueNode().getValue()) || getValueNode().getType() == NodeValueType::Null) {
        throw MerkError("Var " + varFlags.name + " was set to Null during the process");
    }

    DEBUG_FLOW_EXIT();
}

bool VarNode::getIsMutable() {return varFlags.isMutable;}
bool VarNode::getIsStatic() {return varFlags.isMutable;}
bool VarNode::getIsConst() {return varFlags.isConst;}

Node& VarNode::pullValue() {return valueNode;}

bool VarNode::isFunctionNode() const { return valueNode.isFunctionNode();}
FunctionNode VarNode::toFunctionNode() const {return valueNode.toFunctionNode();}





