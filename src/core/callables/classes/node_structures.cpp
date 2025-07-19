#include "core/types.h"
#include "core/errors.h"

#include "core/callables/classes/node_structures.h"



ListNode::~ListNode() = default;
ListNode::ListNode() = default;
ListNode::ListNode(Vector<Node> init) : elements(std::move(init)) {}
void ListNode::append(const Node& node) { elements.push_back(node); }
const Vector<Node>& ListNode::getElements() const { return elements; }
Vector<Node>& ListNode::getMutableElements() { return elements; }
String ListNode::toString() const {
    // throw MerkError("Called ListNode::toString()");
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


String ListNode::getTypeAsString() const {
    return nodeTypeToString(type);
}


ArrayNode::ArrayNode(Vector<Node> init, NodeValueType nodeType)
    : ListNode(init) {
        type = nodeType;
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