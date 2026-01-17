#include "core/node/NodeStructures.hpp"
#include "core/node/ArgumentNode.hpp"
#include "core/callables/classes/bultins.h"
#include "utilities/debugger.h"
#include "core/errors.h"
#include "algorithm"
#include <functional> 


String NativeNode::getTypeAsString() const { return nodeTypeToString(dataType); }
String NativeNode::toString() const { return nodeTypeToString(dataType); }
void NativeNode::setType(NodeValueType thisType) {dataType = thisType;}
NodeValueType NativeNode::getType() const {return dataType;}
void NativeNode::setContains(NodeValueType thisContains) {contains = thisContains;}
NodeValueType NativeNode::getContains() {return contains;}
int NativeNode::length() const { return 0; }

ListNode::~ListNode() {elements.clear();};

ListNode::ListNode() {
    setType(NodeValueType::List);
    flags.type = NodeValueType::List;
    dataType = NodeValueType::List;
}

SharedPtr<NodeBase> NativeNode::clone() const {
    throw MerkError("Cannot Clone Base Node of NativeNode");
}

SharedPtr<NodeBase> DataStructure::clone() const {
    throw MerkError("Cannot Clone Base Node of DataStructure");
}

int ListNode::length() const { return elements.size(); }

ListNode::ListNode(ArgumentList init) : elements(std::move(init.getPositional())) {setType(NodeValueType::List);}

ListNode::ListNode(NodeList init) : elements(std::move(init)) {setType(NodeValueType::List);}

bool ListNode::holdsValue() { return elements.size() > 0; }

void ListNode::append(const Node& node) {
    // throw MerkError("Appending Node:  Meta: ");
    DEBUG_LOG(LogLevel::PERMISSIVE, "Appending to List: ", this, " New state: ", this->toString());

    elements.push_back(node);
}

bool ListNode::remove(const Node& value) {
    auto position = std::find(elements.begin(), elements.end(), value);
    if (position != elements.end()) {
        elements.erase(position);
        return true;
    }
    return false;
}

void ListNode::insert(const Node& index, const Node& value) {
    elements.insert(elements.begin() + index.toInt(), value);
}

Node ListNode::pop(const Node& index) {
    auto valToGet = !index.isValid() ? elements.size() - 1 : index.toInt();

    auto position = std::find(elements.begin(), elements.end(), elements[valToGet]);
    if (position != elements.end()) {
        // int pos = position->getValue();
        auto val = Node::fromVariant(position->getValue());
        if (!val.isValid()) {throw MerkError("pop result is not valid");}
        DEBUG_LOG(LogLevel::PERMISSIVE, val.toString());
        elements.erase(position);
        return val;
    }

    throw MerkError("Returning Nothing");
    return Node();
}

VariantType ListNode::getValue() const {return elements;}

void ListNode::setValue(const VariantType& v) {
    if (std::holds_alternative<SharedPtr<NativeNode>>(v)) {
        auto ds = std::get<SharedPtr<NativeNode>>(v);
        auto list = static_cast<ListNode*>(ds.get());
        elements = list->getElements();
        // elements = static_cast<ListNode*>(std::get<SharedPtr<DataStructure>>(v))->getElements();
    }
}

SharedPtr<NodeBase> ListNode::clone() const {
    NodeList clonedElements;
    for (auto& ele: elements) {
        auto type = DynamicNode::getTypeFromValue(ele.getValue());
        auto isTarget = (type == NodeValueType::Dict || type == NodeValueType::Callable || type == NodeValueType::DataStructure || type == NodeValueType::List || type == NodeValueType::ClassInstance);
        auto valClone = ele.clone();
        if ((ele == valClone || ele.getValue() == valClone.getValue()) && isTarget) {
            throw MerkError("List Didn't Clone Something");
        }
        clonedElements.emplace_back(valClone);
    }

    if (clonedElements.size() != elements.size()) { throw MerkError("Elements do not match in number in ListNode::clone"); }
    // throw MerkError("Cloned ListNode");
    
    auto clonedList = makeShared<ListNode>(clonedElements);

    // throw MerkError("Cloned ListNode: " + toString() + "  Into Another " + clonedList->toString());
    return clonedList;
}

void ListNode::clear() {elements.clear();}

std::size_t ListNode::hash() const {
    size_t finalHash = 0;
    for (auto& item : elements) {
        finalHash ^= item.hash() + 0x9e3779b9 + (finalHash << 6) + (finalHash >> 2);
    }
    return finalHash;
}

const Vector<Node>& ListNode::getElements() const { return elements; }

Vector<Node>& ListNode::getMutableElements() { return elements; }

String ListNode::toString() const {
    String repr = "[";
    for (size_t i = 0; i < elements.size(); ++i) {
        repr += elements[i].toString();
        if (i < elements.size() - 1) repr += ", ";
    }
    repr += "]";
    return repr;
}

ArrayNode::ArrayNode(NodeList init, NodeValueType type) : ListNode(init) {
    contains = type;
}

void ArrayNode::append(const Node& node) {
    if (contains == NodeValueType::Any || node.getType() == contains) {
        elements.push_back(node);
    } else {
        throw MerkError("Type: " + node.getTypeAsString() + " is not supported in an Array<" + getTypeAsString() + ">");
    }
}

ArrayNode::ArrayNode(ArgumentList init, NodeValueType nodeType)
: ListNode(init) {
    contains = nodeType;
    setType(NodeValueType::Array);
    flags.type = NodeValueType::Array;
    dataType = NodeValueType::Array;
}

void ArrayNode::setValue(const VariantType& v) {
    if (std::holds_alternative<SharedPtr<NativeNode>>(v)) {
        auto ds = std::get<SharedPtr<NativeNode>>(v);
        auto arr = static_cast<ArrayNode*>(ds.get());
        elements = arr->getElements();
    }
}

void ArrayNode::insert(const Node& index, const Node& value) {
    if (contains == NodeValueType::Any || value.getType() == contains) {
        elements.insert(elements.begin() + index.toInt(), value);
        return;
    }
    
    throw MerkError("Cannot Append value of type " + nodeTypeToString(value.getType()) + " to array of type " + nodeTypeToString(getContains()));
}

String ArrayNode::toString() const {
    String repr = getTypeAsString() + "<";
    for (size_t i = 0; i < elements.size(); ++i) {
        repr += elements[i].toString();
        if (i < elements.size() - 1) repr += ", ";
    }
    repr += ">";
    return repr;
}

SharedPtr<NodeBase> ArrayNode::clone() const {
    NodeList clonedElements;
    for (auto& el: elements) {
        clonedElements.emplace_back(el.clone());
    }

    return makeShared<ArrayNode>(clonedElements, contains);
}

DictNode::~DictNode() {
    elements.clear();
};

const std::unordered_map<Node, Node>& DictNode::getElements() const {
    return elements;
}

DictNode::DictNode(NodeMapU init) {
    elements = init;
}

DictNode::DictNode(ArgumentList init) {
    dataType = NodeValueType::Dict;
    flags.type = NodeValueType::Dict;
    setType(NodeValueType::Dict);
    // DEBUG_LOG(LogLevel::PERMISSIVE, "ARGUMENT LIST: ", init.toString());
    if (init.hasNamedArgs()) {
        auto args = init.getNamedArgs();
        for (auto& [key, val] : args) {
            
            elements[Node(key)] = Node(val);
            // elements[key] = Node(val.getValue());
        }
    }
    // init.getNamedArgs().clear();
    // init.getPositional().clear();
}

int DictNode::length() const {
    return elements.size();
}

DictNode::DictNode() {
    dataType = NodeValueType::Dict;
    flags.type = NodeValueType::Dict;
    setType(NodeValueType::Dict);
};

String DictNode::toString() const {
    String repr = "{";
    // auto it = elements.end();
    for (auto& [key, val] : elements) {
        String valString = val.toString();
        // if (valString.size() > 500) {
        //     valString = valString.substr(0, 500);
        // }
        repr += key.toString() + " : " + valString + ", ";
    }
    repr += "}";
    if (repr.size() > 2) {
        repr.replace(repr.size() - 3, repr.size(), "}");
    }
    
    return repr;
}

bool DictNode::holdsValue() {
    return elements.size() > 0;
}

void DictNode::set(const Node& key, const Node& value) {
    if (value.isNull()) {throw MerkError("Setting A null Value");}
    elements[key] = value;
}

void DictNode::set(const String& key, const Node value) {
    set(Node(key), value);
}

void DictNode::set(const char* key, Node value) { set(Node(key), std::move(value)); }

Node DictNode::pop(const Node& key) {
    auto val = elements.find(key);
    // elements.erase(val);
    return val->second;
}

void DictNode::remove(const Node& key) {
    elements.erase(key);
}

Node DictNode::get(const Node& key, const Node& defaultReturn) {
    auto val = elements.find(key);
    if (val != elements.end()) {
        return val->second;
    }
    return defaultReturn;
}

VariantType DictNode::getValue() const {return elements;}

void DictNode::setValue(const VariantType& v) {
    if (std::holds_alternative<SharedPtr<NativeNode>>(v)) {
        auto ds = std::get<SharedPtr<NativeNode>>(v);
        auto dict = static_cast<DictNode*>(ds.get());
        elements = dict->getElements();
        // elements = static_cast<ListNode*>(std::get<SharedPtr<DataStructure>>(v))->getElements();
    }
}

SharedPtr<NodeBase> DictNode::clone() const {
    std::unordered_map<Node, Node> clonedElements;
    for (auto& [key, val] : elements) {
        auto type = DynamicNode::getTypeFromValue(val.getValue());
        auto isTarget = (type == NodeValueType::Dict || type == NodeValueType::Callable || type == NodeValueType::DataStructure || type == NodeValueType::List || type == NodeValueType::ClassInstance);
        auto valClone = val.clone();
        if ((val == valClone || val.getValue() == valClone.getValue()) && isTarget) {
            throw MerkError("DictNode Didn't clone something");
        }
        clonedElements[key.clone()] = valClone;    
    }
    return makeShared<DictNode>(clonedElements);
}

void DictNode::clear() {elements.clear();}

std::size_t DictNode::hash() const {
    size_t finalHash = 0;
    for (auto& [key, val] : elements) {
        std::size_t pairHash = key.hash() ^ (val.hash() << 1);
        finalHash ^= pairHash + 0x9e3779b9 + (finalHash << 6) + (finalHash >> 2);
    }

    return finalHash;
}

SetNode::~SetNode() {clear();};

void SetNode::clear() {elements.clear();}

String SetNode::toString() const {
    String repr = getTypeAsString() + "{";
    if (getTypeAsString() == "UNKNOWN") {throw MerkError("getTypeAsString return UNKNOWN");}
    // elements.end();
    // elements.size();
    // auto currentElem = elements.begin();
    for (auto & elem : elements) {
        repr += elem.toString();
        repr += ", ";
    }

    repr += "}";
    return repr;
}

SharedPtr<NodeBase> SetNode::clone() const {
    std::unordered_set<Node> clonedElements;
    for (auto& el : elements) {
        clonedElements.emplace(el.clone());
    }
    return makeShared<SetNode>(clonedElements);
}

bool SetNode::holdsValue() {
    return elements.size() > 0;
}

int SetNode::length() const {
    return elements.size();
}

void SetNode::add(const Node& value) {
    elements.emplace(value);
}

Node SetNode::get(const Node& value) {
    for (auto val : elements) {
        if (val.toString() == value.toString()) {
            return val;
        }
    }
    return Node(Null);
}

void SetNode::remove(const Node& value) {
    elements.erase(value);
}

Node SetNode::has(const Node& value) {
    return Node(elements.count(value) > 0);
}

std::size_t SetNode::hash() const {
    size_t finalHash = 0;
    for (auto& val : elements) {
        finalHash += val.hash();
    }

    return finalHash;
}

VariantType SetNode::getValue() const {return elements;}

void SetNode::setValue(const VariantType& v) {
    if (std::holds_alternative<SharedPtr<NativeNode>>(v)) {
        auto ds = std::get<SharedPtr<NativeNode>>(v);
        auto set = static_cast<SetNode*>(ds.get());
        elements = set->getElements();
    }
    throw MerkError("Trying to Set variant to Set, but is not Native");
}

SetNode::SetNode(VariantType& val) {
    (void)val;
    setType(NodeValueType::Set);
    throw MerkError("This shouldn't execute...I think");
}

SetNode::SetNode(ArgumentList val) {
    auto args = val.getPositional();
    if (args.size() != 0) {
        if (args.size() == 1) {
            auto first = args.front();
            if (first.isSet()) {
                for (auto v : first.toSet()->getElements()) {
                    add(v);
                }
            }
            else if (first.isArray() || first.isList()) {
                for (auto v : first.toList()->getElements()) {
                    add(v);
                }
            }
            else if (first.isDict()) {
                for (auto v : first.toDict()->getElements()) {
                    add(v.first);
                }
            }
        } else {
            throw MerkError("args passed to SetNode(ArgumentList) is more than 1");
        }
    } else if (args.size() == 0) {
        elements.emplace(Node(0));
        elements.erase(Node(0));
    }
    
    else {
        // (void)val;

        throw MerkError("Haven't Handled Set from ArgumentList: val is " + val.toString());
    }
    setType(NodeValueType::Set);
    
}

SetNode::SetNode(std::unordered_set<Node> init) : elements(init) {
    setType(NodeValueType::Set);
}

SetNode::SetNode(std::unordered_set<Node>&& init)
        : elements(std::move(init)) {
        setType(NodeValueType::Set);
    }

const std::unordered_set<Node>& SetNode::getElements() const {return elements; }

InstanceBoundNative::InstanceBoundNative(SharedPtr<ClassInstanceNode> node): instance(node->getInstance()) {}

InstanceBoundNative::InstanceBoundNative() = default;

void InstanceBoundNative::setInstance(SharedPtr<ClassInstance> thisInstance) { instance = thisInstance; };

SharedPtr<ClassInstance> InstanceBoundNative::getInstance() {
    if (auto current = instance.lock()) { return current; }
    return nullptr;
}

InstanceBoundNative::~InstanceBoundNative() {
    // if (getInstance()) {
    //     getInstance()->clear();
    // }
}



SharedPtr<NativeNode> ListNode::toNative() const {
    return makeShared<ListNode>(getElements());
}

SharedPtr<NativeNode> ArrayNode::toNative() const {
    return makeShared<ArrayNode>(getElements(), getNodeType());
}

SharedPtr<NativeNode> DictNode::toNative() const {
    return makeShared<DictNode>(getElements());
}

SharedPtr<NativeNode> SetNode::toNative() const {
    return makeShared<SetNode>(getElements());
}

SharedPtr<NativeNode> InstanceBoundNative::toNative() const {
    throw MerkError("InstanceBoundNative cannot return toNative()");
}

SharedPtr<NativeNode> HttpNode::toNative() const {
    throw MerkError("HttpNode cannot return toNative()");
}

SharedPtr<NativeNode> FileNode::toNative() const {
    throw MerkError("FileNode cannot return toNative()");
}





// SharedPtr<NodeBase> DataStructure::operator==(const NodeBase& other) const {
//     MARK_UNUSED_MULTI(other);
//     throw MerkError("Cannot Compare Base Type of DataStructure");
// }

// SharedPtr<NodeBase> DataStructure::operator==(const NodeBase& other) const {
//     return makeShared<BoolNode>(other.toString() == toString());
// }