#include "core/types.h"
#include "core/errors.h"

#include "core/callables/classes/node_structures.h"
#include "utilities/debugger.h"
#include "algorithm"

String DataStructure::getTypeAsString() const {
    return nodeTypeToString(dataType);
}


String DataStructure::toString() const {
    return nodeTypeToString(dataType);
}

ListNode::~ListNode() = default;
ListNode::ListNode() {setType(NodeValueType::List);}

ListNode::ListNode(Vector<Node> init) : elements(std::move(init)) {setType(NodeValueType::List);}
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

    auto position = std::find(elements.begin(), elements.end(), elements[index.toInt()]);
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



ArrayNode::ArrayNode(Vector<Node> init, NodeValueType nodeType)
: ListNode(init) {
    contains = nodeType;
    setType(NodeValueType::Array);
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

// DictNode::DictNode() {elements = {Node(0), Node(0)};}
// DictNode::~DictNode() = default;
// DictNode::DictNode() = default;

// DictNode::DictNode(Vector<Node> init) {
//     for (auto& var : init) {

//     }
// }


// SetNode::~SetNode() = default;