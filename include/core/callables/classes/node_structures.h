#ifndef NODE_STRUCTURES_H
#define NODE_STRUCTURES_H

#include "core/types.h"
#include "core/node.h"


class DataStructure: public Node {
protected:
    NodeValueType dataType;
    NodeValueType contains;
public:
    DataStructure() = default;
    virtual String getTypeAsString() const;

    virtual String toString() const;

    void setType(NodeValueType thisType) {dataType = thisType;}
    NodeValueType getType() {return dataType;}


    void setContains(NodeValueType thisContains) {contains = thisContains;}
    NodeValueType getContains() {return contains;}
    virtual bool holdsValue() {return false;}

    friend ArrayNode;
    friend ListNode;
};


class ListNode: public DataStructure {    

private:
    Vector<Node> elements;
    

public:
    static String getOriginalVarName() {return "[__LIST__]";}
    ListNode();
    ~ListNode();
    explicit ListNode(Vector<Node> init);

    virtual void append(const Node& node);
    virtual bool remove(const Node& value);
    virtual void insert(const Node& index, const Node& value);
    virtual Node pop(const Node& value);
    virtual const Vector<Node>& getElements() const;
    virtual Vector<Node>& getMutableElements();

    virtual String toString() const;
    virtual bool holdsValue() override;
    friend ArrayNode;
};


class ArrayNode: public ListNode {

public:
    static String getOriginalVarName() {return "[__ARRAY__]";}
    explicit ArrayNode(Vector<Node> init, NodeValueType type);

    void append(const Node& node) override;
    void insert(const Node& index, const Node& value) override;
    String toString() const override;
};



// class DataStructureNode : public ClassInstanceNode {

// }

// class DictNode {    

// private:
//     String varName = "[__DICT__]";
//     std::unordered_map<Node, Node> elements;
//     NodeValueType type = NodeValueType::Dict;
//     NodeValueType contains = NodeValueType::Any;
    

// public:
//     static String getOriginalVarName() {return "[__DICT__]";}
//     String getVarName();
//     DictNode();
//     ~DictNode();
//     explicit DictNode(Vector<Node> init);
//     void set(const Node& key, const Node& value);
//     void get(const Node& key);
//     Node pop(const Node& value);
//     void remove(const Node& key);

    
//     const std::unordered_map<Node, Node>& getElements() const;
//     std::unordered_map<Node, Node>& getMutableElements();
//     String getTypeAsString() const;

//     String toString() const;

//     void setVarName(String);
// };



// class SetNode {    

// private:
//     String varName = "[__SET__]";
//     std::unordered_set<Node> elements;
//     NodeValueType type = NodeValueType::Set;
//     NodeValueType contains = NodeValueType::Any;
    

// public:
//     static String getOriginalVarName() {return "[__SET__]";}
//     String getVarName();
//     SetNode();
//     ~SetNode();
//     explicit SetNode(Vector<Node> init);
//     void add(const Node& value);
//     void get(const Node& value);
//     Node pop(const Node& value);
//     void remove(const Node& value);
//     Node has(const Node& value);

    
//     const std::unordered_map<Node, Node>& getElements() const;
//     std::unordered_map<Node, Node>& getMutableElements();
//     String getTypeAsString() const;

//     String toString() const;

//     void setVarName(String);
// };


#endif //NODE_STRUCTURES_H



