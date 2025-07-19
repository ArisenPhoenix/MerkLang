#ifndef NODE_STRUCTURES_H
#define NODE_STRUCTURES_H

#include "core/types.h"
#include "core/node.h"


class ListNode {
private:
    Vector<Node> elements;
    NodeValueType type = NodeValueType::List;
    NodeValueType contains = NodeValueType::Any;

public:
    ListNode();
    virtual ~ListNode();
    explicit ListNode(Vector<Node> init);

    virtual void append(const Node& node);
    virtual const Vector<Node>& getElements() const;
    virtual Vector<Node>& getMutableElements();
    String getTypeAsString() const;

    virtual String toString() const; 
    friend ArrayNode;
};


class ArrayNode: public ListNode {
public:
    explicit ArrayNode(Vector<Node> init, NodeValueType type);

    void append(const Node& node) override;
    // const Vector<Node>& getElements() const override;
    // Vector<Node>& getMutableElements() override;

    String toString() const override;
};


#endif //NODE_STRUCTURES_H