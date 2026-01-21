#include "core/node/Node.hpp"
#include "core/node/NodeStructures.hpp"
#include "core/types.h"
#include "core/Scope.hpp"
#include "utilities/debugger.h"
#include "utilities/debugging_functions.h"
#include "core/callables/classes/ClassBase.hpp"
#include "core/callables/functions/Function.hpp"
#include "core/callables/Callable.hpp"
#include "core/evaluators/TypeEvaluator.hpp"
#include <charconv>

// virtual type checkers
int NodeBase::toInt() const { throw MerkError("Not an Int"); }
String NodeBase::toString() const { throw MerkError("Not a String"); }
bool NodeBase::toBool() const { return false; }
bool NodeBase::isValid() const { return false; }
bool NodeBase::isNumeric() const { throw MerkError("isNumeric Called On NodeBase"); }


SharedPtr<NodeBase> Node::getInner() { return data; }
SharedPtr<NodeBase> Node::getInner() const { return data; }



bool NodeBase::isString() const { return false; }

bool NodeBase::isChars() const {return false;}
bool NodeBase::isChar() const { return false; }
char* NodeBase::toChars() const { throw MerkError("Not a Char*"); }
char NodeBase::toChar() const {throw MerkError("Not a Char");}

bool NodeBase::isInt() const { return false; }
bool NodeBase::isBool() const { return false; }
bool NodeBase::isTruthy() const { return false; }
bool NodeBase::isNull() const {throw MerkError("isNull called on NodeBase"); return true;}

bool NodeBase::isDouble() const { return false; }
double NodeBase::toDouble() const {throw MerkError("Not a Double"); }

bool NodeBase::isFloat() const { return false; }
float NodeBase::toFloat() const { throw MerkError("Not A Float"); }

bool NodeBase::isNative() const { return false; }
SharedPtr<NativeNode> NodeBase::toNative() const {throw MerkError("Not A NativeNode");}
void NodeBase::setFlags(DataTypeFlags newOnes) { 

    auto actual = getType();

    if (actual != NodeValueType::Any && // if you use Any for DynamicNode
        newOnes.type != NodeValueType::Any &&
        newOnes.type != actual) {
        throw MerkError("setFlags would contradict node runtime type: actual=" +
            nodeTypeToString(actual) + " new=" + nodeTypeToString(newOnes.type));
    }

    // if (flags.type != NodeValueType::Any && newOnes.type == NodeValueType::Any) {
    //     // throw MerkError("setFlags would erase runtime type");
    // }

    flags = newOnes;

    // Optionally: force to actual
    flags.type = actual;
}


bool NodeBase::isList() const {
        if (flags.fullType.getBaseType() == "List" ) { return true; }
        if (TypeEvaluator::getTypeFromValue(getValue()) == NodeValueType::List) { return true; }
        return false;       
    }
bool NodeBase::isArray() const {
    if (flags.fullType.getBaseType() == "Array" ) { return true; }
    if (TypeEvaluator::getTypeFromValue(getValue()) == NodeValueType::Array) { return true; }
    return false;    
}
bool NodeBase::isDict() const {
    if (flags.fullType.getBaseType() == "Dict" ) { return true; }
    if (TypeEvaluator::getTypeFromValue(getValue()) == NodeValueType::Dict) { return true; }
    return false;    
}
bool NodeBase::isSet() const {
    if (flags.fullType.getBaseType() == "Set" ) { return true; }
    if (TypeEvaluator::getTypeFromValue(getValue()) == NodeValueType::Set) { return true; }
    return false;    
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

    return h1 ^ (h2 << 1); /* ^ (h3 << 2) ^ (h4 << 3); */
}


std::size_t NodeBase::strictHash() const {
    auto base = hash();
    std::size_t h3 = std::hash<std::string>()(flags.fullType.getBaseType());
    std::size_t h4 = (flags.isConst << 1) ^ (flags.isMutable << 2) ^ (flags.isStatic << 3);
    return base ^ (h3 << 2) ^ (h4 << 3);
}

NullNode::NullNode() {
    value = Null;
    flags.type = NodeValueType::Null;
    flags.fullType.setBaseType("Null");
    instantiatedWithNull = true;
}

NullNode::NullNode(NullType v) {
    if (TypeEvaluator::getTypeFromValue(v) != NodeValueType::Null) {
        throw MerkError("Tried Instantiating a NullNode from Null, but of type " + nodeTypeToString(TypeEvaluator::getTypeFromValue(v)) + " Onto NullNode");
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
    throw MerkError("Tried Creating a NullNode from " + nodeTypeToString(TypeEvaluator::getTypeFromValue(v)));
}

VariantType NullNode::getValue() const {
    return value;
}

void NullNode::setValue(const VariantType& v) {

    if (std::holds_alternative<NullType>(v)) {
        value = std::get<NullType>(v);
    }
    throw MerkError("Tried Setting NullNode to " + nodeTypeToString(TypeEvaluator::getTypeFromValue(v)));
};

NodeValueType NullNode::getType() const {return TypeEvaluator::getTypeFromValue(getValue());}
SharedPtr<NodeBase> NullNode::clone() const { return makeShared<NullNode>(*this); }

int NullNode::toInt() const { 
    throw MerkError("NullNode Cannot be an Integer");
    // return -10000000000000000; 
}

String NullNode::toString() const {return "null"; };

bool NullNode::toBool() const { return false; };

bool NullNode::isValid() const { return instantiatedWithNull; }

bool NullNode::isNull() const { return true; }
bool NullNode::isNumeric() const { return false; }
void NullNode::clear() {};

SharedPtr<NativeNode> Node::toNative() {
    return data->toNative();
}
SharedPtr<NativeNode> Node::toNative() const {
    return data->toNative();
}

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
    // return std::holds_alternative<SharedPtr<Callable>>(getValue()) && getType() != NodeValueType::ClassInstance;
}

bool Node::isCallable() const {return getInner()->getType() == NodeValueType::Callable || (std::holds_alternative<SharedPtr<Callable>>(getValue()) && getType() != NodeValueType::ClassInstance); return std::holds_alternative<SharedPtr<Callable>>(getValue()) && getType() != NodeValueType::ClassInstance; }

SharedPtr<Callable> Node::toCallable() {
    if (isCallable()) {
        return std::get<SharedPtr<Callable>>(getValue());
    }
    throw MerkError("Current Value is not a Callable, but a " + nodeTypeToString(TypeEvaluator::getTypeFromValue(getValue())));
}

SharedPtr<Callable> Node::toCallable() const {
    if (isCallable()) {
        return std::get<SharedPtr<Callable>>(getValue());
    }
    throw MerkError("Current Value is not a Callable, but a " + nodeTypeToString(TypeEvaluator::getTypeFromValue(getValue())));
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
        data = makeShared<AnyNode>(v);
        return;
    }
    if (auto nullVar = dynamic_cast<NullNode*>(data.get())) {
        if (nullVar->instantiatedWithNull && !std::holds_alternative<NullType>(v)) {
            data = makeShared<AnyNode>(v);
            return;
        }
    }
    data->setValue(v); 
}

NodeValueType Node::getType() const { return data->getType(); }
Node Node::clone() const {
    if (!data) throw MerkError("clone: null data");
    auto t = data->getType();
    DEBUG_LOG(LogLevel::ERROR, "CLONE type=", nodeTypeToString(t), " str=", data->toString());
    auto c = data->clone();
    
    if (!c) throw MerkError("clone returned null for type " + nodeTypeToString(t));
    c->flags = getFlags();
    DEBUG_LOG(LogLevel::ERROR, "CLONE RESULT type=", nodeTypeToString(c->getType()), " str=", c->toString());
    return Node(c);
}

bool Node::isNumeric() const { return data->isNumeric(); }
int Node::toInt() const { return data->toInt(); }

String Node::toString() const { 
    if (isFunctionNode()) {
        return "<FunctionRef " + std::to_string(std::get<Vector<SharedPtr<CallableSignature>>>(getValue()).size()) + " overload(s)>"; 
    } else if (isInstance()) {
        // throw MerkError("Trying to represent an Instance");
        return toInstance()->toString();
    } 

    return data->toString(); 
}

bool Node::toBool() const { return data->toBool(); }
bool Node::isBool() const {return data->isBool();}
bool Node::isInt() const {return data->isInt();}
bool Node::isString() const {return data->isString();}

bool Node::isChars() const {return data->isChars();}
char* Node::toChars() const {return data->toChars();}

bool Node::isNative() const {return data->isNative();}
bool Node::isNative() {return data->isNative();}

NodeValueType Node::getNodeType() const {return getFlags().type;}
String Node::getTypeAsString() const {return nodeTypeToString(getNodeType());}

bool Node::isInstance() const { return getFlags().isInstance; }
bool Node::isInstance() { return getFlags().isInstance; }

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
    if (std::holds_alternative<SharedPtr<Callable>>(getValue())) {
        auto v = std::static_pointer_cast<ClassInstance>(std::get<SharedPtr<Callable>>(getValue()))->getNativeData();
        auto inst = std::static_pointer_cast<ListNode>(v); 
        return inst;
    }
    throw MerkError("Node Is Not A NativeNode, so not a " + nodeTypeToString(getNodeType()) + ", but a " + nodeTypeToString(TypeEvaluator::getTypeFromValue(getValue())));
}

SharedPtr<ArrayNode> Node::toArray() const {
    if (std::holds_alternative<SharedPtr<Callable>>(getValue())) {
        auto v = std::static_pointer_cast<ClassInstance>(std::get<SharedPtr<Callable>>(getValue()))->getNativeData();
        auto inst = std::static_pointer_cast<ArrayNode>(v); 
        return inst;
    }
    throw MerkError("Node Is Not A NativeNode, so not a " + nodeTypeToString(getNodeType()) + ", but a " + nodeTypeToString(TypeEvaluator::getTypeFromValue(getValue())));
}

SharedPtr<DictNode> Node::toDict() const {
    if (std::holds_alternative<SharedPtr<Callable>>(getValue())) {
        auto v = std::static_pointer_cast<ClassInstance>(std::get<SharedPtr<Callable>>(getValue()))->getNativeData();
        auto inst = std::static_pointer_cast<DictNode>(v); 
        return inst;
    }
    throw MerkError("Node Is Not A NativeNode, so not a " + nodeTypeToString(getNodeType()) + ", but a " + nodeTypeToString(TypeEvaluator::getTypeFromValue(getValue())));
}

SharedPtr<SetNode> Node::toSet() const {
    if (std::holds_alternative<SharedPtr<Callable>>(getValue())) {
        auto v = std::static_pointer_cast<ClassInstance>(std::get<SharedPtr<Callable>>(getValue()))->getNativeData();
        auto inst = std::static_pointer_cast<SetNode>(v); 
        return inst;
    }
    throw MerkError("Node Is Not A NativeNode, so not a " + nodeTypeToString(getNodeType()) + ", but a " + nodeTypeToString(TypeEvaluator::getTypeFromValue(getValue())));
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


std::size_t Node::strictHash() const {
    if (data) {
        return data->strictHash();
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

int BoolNode::toInt() const {  return value ? 1 : 0; }
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

int StringNode::toInt() const { throw MerkError("Cannot Implicitly cast String to Int"); }
String StringNode::toString() const { return String(value); }
bool StringNode::toBool() const { return !value.empty(); }


// CharNode

CharNode::CharNode(VariantType v) {
    if (std::holds_alternative<char>(v)) {
        value = std::get<char>(v);
    }
    throw MerkError("CharNode::CharNode(VariantType v) -> v is not a char");
}

CharNode::CharNode(char v) {
    value += v;
}

VariantType CharNode::getValue() const {
    return value;
}

void CharNode::setValue(const VariantType& v)  { value = std::get<char>(v); }

NodeValueType CharNode::getType() const { return NodeValueType::Char; }
SharedPtr<NodeBase> CharNode::clone() const {return makeShared<CharNode>(value);}

int CharNode::toInt() const {return TypeEvaluator::to<int>(value);}

bool CharNode::isString() const { return false; }
bool CharNode::isChars() const { return true; }
char* CharNode::toChars() const { throw MerkError("Not Implemented CharNode::toChars()"); }

bool CharNode::isChar() const { return true; }
char CharNode::toChar() const { return value; }
String CharNode::toString() const { return TypeEvaluator::as<String>(value); }
bool CharNode::isBool() const {String tmp = TypeEvaluator::as<String>(value); return tmp == "true" || tmp == "0" || tmp == "1" || tmp == "false";}

bool CharNode::toBool() const { return false; }

bool CharNode::isNumeric() const {return false;}
bool CharNode::isValid() const { return value; }
bool CharNode::isTruthy() const { return value; }

void CharNode::clear() {
    value = 0;
}


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

    throw MerkError("Cannot convert type: " + nodeTypeToString(TypeEvaluator::getTypeFromValue(v)) + " to Float");
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
    throw MerkError("Cannot Set Type " + nodeTypeToString(TypeEvaluator::getTypeFromValue(v)) + " To Double");
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
bool NodeWrapper::isString() const {
    if (isValid()) { 
        return valueNode.isString(); 
    } 
    else {throw MerkError("NodeWrapper::toString() -> Cannot Validate String Because Node is Invalid");}
}
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

String VarNode::varString() const { return NodeWrapper::toString() + (!varFlags.name.empty() ? ", Var: " + varFlags.name : "");}

void VarNode::setValue(Node other) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    if (std::holds_alternative<NullType>(other.getValue()) || other.getType() == NodeValueType::Null) {
        if (other.getType() == NodeValueType::Null && !std::holds_alternative<NullType>(other.getValue())) {
            throw MerkError("Tried Setting value typed as null, but actually isn't. It is type " + nodeTypeToString(TypeEvaluator::getTypeFromValue(other.getValue())));
        }

        throw MerkError("Tried Setting Var " + varFlags.name + " to Null, Value Is: " + TypeEvaluator::to<String>(other.getValue()) + " " + other.getFlags().toString() + " "  + other.getInner()->toString());
    }
    if (!other.isValid()) { throw MerkError("Other is not valid"); }

    if (varFlags.isConst) { throw MerkError("Assignment to const variable"); }

    auto otherFlags = other.getFlags();
    if (varFlags.type != NodeValueType::Any) {
        if (otherFlags.type != varFlags.type) {
            String expected = "Expected: " + nodeTypeToString(varFlags.type) + " | " + varFlags.fullType.toString();
            String got = "Got " + nodeTypeToString(otherFlags.type) + " | " + otherFlags.fullType.toString(); 
            throw MerkError("Type Mismatch from declaration: " + expected + ", But " + got);
        }
    }  

    if (other.getNodeType() == NodeValueType::Uninitialized) { throw MerkError("Parameter being set");  }

    // Keeping the variable's declaration flags intact
    auto varName = varFlags.name;
    auto varType = varFlags.fullType.getBaseType();

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
DataTypeFlags VarNode::getVarFlags() { return varFlags; }




