#ifndef NODE_STRUCTURES_H
#define NODE_STRUCTURES_H

// #include "core/types.h"
#include "core/TypesFWD.hpp"

#include "core/node/Node.hpp"
#include <fstream>

class NativeNode: public NodeBase {
protected:
    NodeValueType dataType;
    NodeValueType contains;

public:
    virtual String getTypeAsString() const;

    virtual String toString() const override = 0;
    void setType(NodeValueType thisType);
    NodeValueType getType() const override;
    void setContains(NodeValueType thisContains);
    NodeValueType getContains();
    virtual bool holdsValue() = 0;
    virtual std::size_t hash() const override = 0;
    virtual int length() const;
    bool isNative() const {return true;}
    virtual SharedPtr<NativeNode> toNative() const override = 0;
    virtual SharedPtr<NodeBase> clone() const override;
    // SharedPtr<NodeBase> operator==(const NodeBase& other) const override = 0;
};


class DataStructure: public NativeNode {

public:
    DataStructure() = default;
    // virtual String getTypeAsString() const;

    // virtual String toString() const override;
    // void setType(NodeValueType thisType);
    // NodeValueType getType() const override;
    // void setContains(NodeValueType thisContains);
    // NodeValueType getContains();
    // virtual bool holdsValue() override {return false;}

    virtual int length() const = 0;
    virtual SharedPtr<NativeNode> toNative() const override = 0;

    virtual std::size_t hash() const override = 0;
    friend ArrayNode;
    friend ListNode;
    friend DictNode;
    friend SetNode;
    virtual SharedPtr<NodeBase> clone() const override = 0;
    // SharedPtr<NodeBase> operator==(const NodeBase& other) const override;

};

class ListNode: public DataStructure {    

private:
    NodeList elements;
public:
    static String getOriginalVarName() {return "[__LIST__]";}
    ListNode();
    ~ListNode();
    explicit ListNode(ArgumentList init);
    explicit ListNode(NodeList init);

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

    virtual VariantType getValue() const override;
    void setValue(const VariantType& v) override;
    virtual SharedPtr<NodeBase> clone() const override;
    void clear() override;
    int length() const override;

    virtual SharedPtr<NativeNode> toNative() const override;
    // SharedPtr<NodeBase> operator==(const NodeBase& other) const override;
};

class ArrayNode: public ListNode {
public:
    static String getOriginalVarName() {return "[__ARRAY__]";}
    explicit ArrayNode(ArgumentList init, NodeValueType type);
    explicit ArrayNode(NodeList init, NodeValueType type);

    void append(const Node& node) override;
    void insert(const Node& index, const Node& value) override;
    String toString() const override;
    void setValue(const VariantType& v) override;
    SharedPtr<NodeBase> clone() const override;  
    SharedPtr<NativeNode> toNative() const override;
    // SharedPtr<NodeBase> operator==(const NodeBase& other) const override;
};

class DictNode: public DataStructure {
private:
    std::unordered_map<Node, Node> elements;
    NodeValueType type = NodeValueType::Dict;
    NodeValueType contains = NodeValueType::Any;
public:
    static String getOriginalVarName() {return "[__DICT__]";}
    
    DictNode();
    ~DictNode();
    explicit DictNode(ArgumentList init);
    explicit DictNode(NodeMapU elements);
    void set(const Node& key, const Node& value);
    void set(const String& key, const Node value);
    void set(const char* key, Node value);

    Node get(const Node& key, const Node& defaultReturn = Node());
    Node pop(const Node& value);
    void remove(const Node& key);
    virtual bool holdsValue() override;
    std::size_t hash() const override;
    
    const std::unordered_map<Node, Node>& getElements() const;
    std::unordered_map<Node, Node>& getMutableElements();

    String toString() const override;
    
    VariantType getValue() const override;
    void setValue(const VariantType& v) override;
    SharedPtr<NodeBase> clone() const override;
    void clear() override;
    int length() const override;

    SharedPtr<NativeNode> toNative() const override;
    // SharedPtr<NodeBase> operator==(const NodeBase& other) const override;
};

class SetNode: public DataStructure {    
private:
    std::unordered_set<Node> elements;
    NodeValueType type = NodeValueType::Set;
    NodeValueType contains = NodeValueType::Any;

public:
    static String getOriginalVarName() {return "[__SET__]";}
    String getVarName();
    SetNode();
    ~SetNode();
    explicit SetNode(ArgumentList init);
    explicit SetNode(std::unordered_set<Node> elements);
    explicit SetNode(std::unordered_set<Node>&& init);
    explicit SetNode(VariantType& val);
    void add(const Node& value);
    Node get(const Node& value); 
    void remove(const Node& value);
    Node has(const Node& value);
    virtual bool holdsValue() override;
    std::size_t hash() const override;
    
    const std::unordered_set<Node>& getElements() const;
    std::unordered_set<Node>& getMutableElements();

    String toString() const override;
    SharedPtr<NodeBase> clone() const override;

    VariantType getValue() const override;
    void setValue(const VariantType& v) override;
    void clear() override;
    int length() const override;

    SharedPtr<NativeNode> toNative() const override;
    // SharedPtr<NodeBase> operator==(const NodeBase& other) const override;
};


class InstanceBoundNative: public NativeNode {
private:
    WeakPtr<ClassInstance> instance;

public:
    InstanceBoundNative(SharedPtr<ClassInstanceNode>);
    InstanceBoundNative();
    ~InstanceBoundNative();
    SharedPtr<ClassInstance> getInstance();
    void setInstance(SharedPtr<ClassInstance>);
    String toString() const override = 0;
    std::size_t hash() const override = 0;

    VariantType getValue() const override = 0;
    void setValue(const VariantType& v) override = 0;

    SharedPtr<NodeBase> clone() const override = 0;
    void clear() override = 0;
    bool holdsValue() override = 0;

    SharedPtr<NativeNode> toNative() const override;
};

class HttpNode : public InstanceBoundNative {
public:
    static String getOriginalVarName() { return "[__HTTP__]"; }
    HttpNode();
    HttpNode(SharedPtr<ClassInstanceNode>);
    ~HttpNode();

    
    NodeValueType getType() const override { return NodeValueType::Http; }

    

    Node send(); // performs the HTTP request and returns a response Node

    String toString() const override;
    std::size_t hash() const override;

    VariantType getValue() const override;
    void setValue(const VariantType& v) override;

    SharedPtr<NodeBase> clone() const override;
    void clear() override;
    bool holdsValue() override;

    SharedPtr<NativeNode> toNative() const override;

    // HttpNode(const Node& url, const Node& method = Node("GET"));

    // void setHeader(const Node& key, const Node& value);
    // Node getHeader(const Node& key);

    // void setBody(const DictNode& bodyContent);
    // DictNode getBody() const;

    // void setUrl(const Node& newUrl);
    // Node getUrl() const;
};


class FileNode : public InstanceBoundNative {
public:
    FileNode() = default;
    explicit FileNode(SharedPtr<ClassInstanceNode> self)
      : InstanceBoundNative(self) {}

    ~FileNode() override { closeQuiet(); }

    NodeValueType getType() const override { return NodeValueType::File; }

    // NodeBase API
    String toString() const override { return "[FileNode]"; }
    std::size_t hash() const override { return 0; }
    VariantType getValue() const override { return "[FILE]"; }
    void setValue(const VariantType&) override { /* not assignable */ }
    SharedPtr<NodeBase> clone() const override { return makeShared<FileNode>(); }
    void clear() override { closeQuiet(); }
    bool holdsValue() override { return true; }

    // File ops
    void open();
    void close();
    bool isOpen() const { return stream.is_open(); }
    String readAll();
    String read(size_t n);
    void write(String s);
    void writeBytes(const std::vector<uint8_t>& b);
    void seek(std::streamoff pos, std::ios_base::seekdir dir);
    std::streamoff tell();
    static bool exists(const String& path);
    static std::uintmax_t sizeOf(const String& path);
    static void removeFile(const String& path);

    SharedPtr<NativeNode> toNative() const override;

private:
    std::fstream stream;
    bool binary = false;

    // Pulls "path" and "mode" from the bound instance’s scope
    void loadConfigFromInstance();
    void closeQuiet() { if (stream.is_open()) stream.close(); }
};



#endif //NODE_STRUCTURES_H



