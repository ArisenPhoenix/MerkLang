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
    virtual std::size_t hash() const = 0;
    

    friend ArrayNode;
    friend ListNode;
};


class ListNode: public DataStructure {    

private:
    NodeList elements;
    

public:
    static String getOriginalVarName() {return "[__LIST__]";}
    ListNode();
    ~ListNode();
    explicit ListNode(ArgResultType init);

    virtual void append(const Node& node);
    virtual bool remove(const Node& value);
    virtual void insert(const Node& index, const Node& value);
    virtual Node pop(const Node& value);
    virtual const NodeList& getElements() const;
    virtual NodeList& getMutableElements();

    virtual String toString() const;
    virtual bool holdsValue() override;
    std::size_t hash() const override;
    friend ArrayNode;
};


class ArrayNode: public ListNode {

public:
    static String getOriginalVarName() {return "[__ARRAY__]";}
    explicit ArrayNode(ArgResultType init, NodeValueType type);

    void append(const Node& node) override;
    void insert(const Node& index, const Node& value) override;
    String toString() const override;
};




class DictNode: public DataStructure {    

private:
    String varName = "[__DICT__]";
    std::unordered_map<Node, Node> elements;
    NodeValueType type = NodeValueType::Dict;
    NodeValueType contains = NodeValueType::Any;
    

public:
    static String getOriginalVarName() {return "[__DICT__]";}
    String getVarName();
    DictNode();
    ~DictNode();
    explicit DictNode(ArgResultType init);
    void set(const Node& key, const Node& value);
    Node get(const Node& key, const Node& defaultReturn = Node());
    Node pop(const Node& value);
    void remove(const Node& key);
    virtual bool holdsValue() override;
    std::size_t hash() const override;
    
    const std::unordered_map<Node, Node>& getElements() const;
    std::unordered_map<Node, Node>& getMutableElements();

    String toString() const;

    void setVarName(String);
};



class SetNode: DataStructure {    

private:
    String varName = "[__SET__]";
    std::unordered_set<Node> elements;
    NodeValueType type = NodeValueType::Set;
    NodeValueType contains = NodeValueType::Any;
    

public:
    static String getOriginalVarName() {return "[__SET__]";}
    String getVarName();
    SetNode();
    ~SetNode();
    explicit SetNode(ArgResultType init);
    void add(const Node& value);
    void get(const Node& value);
    Node pop(const Node& value);
    void remove(const Node& value);
    Node has(const Node& value);
    virtual bool holdsValue() override;
    std::size_t hash() const override;
    
    const std::unordered_map<Node, Node>& getElements() const;
    std::unordered_map<Node, Node>& getMutableElements();

    String toString() const;

    void setVarName(String);
};



#endif //NODE_STRUCTURES_H



