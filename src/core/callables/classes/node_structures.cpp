#include "core/types.h"
#include "core/errors.h"

#include "core/callables/classes/node_structures.h"
#include "utilities/debugger.h"
#include "algorithm"
#include <functional>


String DataStructure::getTypeAsString() const {
    return nodeTypeToString(dataType);
}


String DataStructure::toString() const {
    return nodeTypeToString(dataType);
}

ListNode::~ListNode() = default;
ListNode::ListNode() {
    setType(NodeValueType::Array);
    data.type = NodeValueType::Array;
    dataType = NodeValueType::Array;}

ListNode::ListNode(ArgumentList init) : elements(std::move(init.getPositional())) {setType(NodeValueType::List);}
bool ListNode::holdsValue() {
    return elements.size() > 0;
}
void ListNode::append(const Node& node) {elements.push_back(node);}
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
    // if (index.toInt() + 1 != 1) { throw MerkError("Is Not An Int " + std::to_string(index.toInt()) ); }
    // if (elements.size() == 0) { throw MerkError("No elements"); }
    auto valToGet = !index.isValid() ? elements.size() - 1 : index.toInt();

    auto position = std::find(elements.begin(), elements.end(), elements[valToGet]);
    // DEBUG_LOG(LogLevel::PERMISSIVE, position.base()->toString());
    if (position != elements.end()) {
        auto val = Node(position->getValue());
        if (!val.isValid()) {throw MerkError("pop result is not valid");}
        DEBUG_LOG(LogLevel::PERMISSIVE, val.toString());
        elements.erase(position);
        return val;
    }

    throw MerkError("Returning Nothing");
    return Node();
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
    data.type = NodeValueType::Array;
    dataType = NodeValueType::Array;
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

DictNode::~DictNode() = default;
DictNode::DictNode(ArgumentList init) {
    dataType = NodeValueType::Dict;
    data.type = NodeValueType::Dict;
    setType(NodeValueType::Dict);
    if (init.size() > 0) {
        if (init.size() > 2) {throw MerkError("Too Many Values to unpack in for Dict init");}
        auto key = init.getPositional()[0];
        if (key.isDict()) {
            elements = key.toDict()->elements;
            return;
        }
        auto value = init.getPositional()[1];
        elements[key] = value;
    }
    

}
DictNode::DictNode() {
    dataType = NodeValueType::Dict;
    data.type = NodeValueType::Dict;
    setType(NodeValueType::Dict);
};

String DictNode::toString() const {
    String repr = getTypeAsString() + "{";
    for (auto& [key, val] : elements) {
        repr += key.toString() + " : " + val.toString(); 
    }
    repr += "}";
    return repr;
}

bool DictNode::holdsValue() {
    return elements.size() > 0;
}


void DictNode::set(const Node& key, const Node& value) {
    elements[key] = value;
}

Node DictNode::pop(const Node& key) {
    auto val = elements.find(key);
    return val->second;
}

void DictNode::remove(const Node& key) {
    elements.erase(key);
}


Node DictNode::get(const Node& key, const Node& defaultReturn) {
    auto val = elements.find(key);
    if (val->second.isValid()) {
        return val->second;
    }

    return defaultReturn;
}


SetNode::~SetNode() = default;


String SetNode::toString() const {
    String repr = getTypeAsString() + "{";
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

bool SetNode::holdsValue() {
    return elements.size() > 0;
}



std::size_t ListNode::hash() const {
    size_t finalHash = 0;
    for (auto& item : elements) {
        finalHash ^= item.hash() + 0x9e3779b9 + (finalHash << 6) + (finalHash >> 2);
    }
    return finalHash;
}



std::size_t DictNode::hash() const {
    size_t finalHash = 0;
    for (auto& [key, val] : elements) {
        std::size_t pairHash = key.hash() ^ (val.hash() << 1);
        finalHash ^= pairHash + 0x9e3779b9 + (finalHash << 6) + (finalHash >> 2);
    }

    return finalHash;
}


std::size_t SetNode::hash() const {
    size_t finalHash = 0;
    for (auto& val : elements) {
        finalHash += val.hash();
    }

    return finalHash;
}