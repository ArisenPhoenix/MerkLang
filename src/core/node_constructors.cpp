#include "core/node.h"
#include "core/types.h"
#include "core/callables/callable.h"
#include "core/callables/classes/class_base.h"
#include "core/callables/classes/node_structures.h"
#include "utilities/debugger.h"
#include "utilities/debugging_functions.h"
#include "core/errors.h"
#include "core/scope.h"

#include <limits>
#include <sstream>
#include <iostream>
#include <variant>
#include <tuple>
#include <functional>
#include <utility>
#include <cmath>





void Node::copyFlagsFrom(const Node& other) {
    isConst = other.isConst;
    isMutable = other.isMutable;
    isStatic = getFullType().getBaseType().size() || isStatic;
    isCallable = other.isCallable;
    name = other.name;
    nodeType = other.nodeType;
    data = other.data;
    setFullType(other.getFullType());
    updateClassInstance(*this);
}

void Node::transferOwnershipFrom(Node&& other) {
    isConst = other.isConst;
    isMutable = other.isMutable;
    isStatic = other.isStatic;
    isCallable = other.isCallable;
    name = std::move(other.name);
    nodeType = std::move(other.nodeType);
    data = other.data;

    updateClassInstance(*this);
}


void Node::updateClassInstance(const Node& me) {
    if (isInstance()) {
        auto instance = std::get<SharedPtr<ClassInstance>>(me.data.value);
        auto instanceNode = instance->getInstanceNode();
        instanceNode->isMutable = isMutable;
        instanceNode->isStatic = isStatic;
        instanceNode->isConst = isConst;
    }
    
}

void Node::applyTypeInfo(std::optional<NodeValueType> typeTag, const ResolvedType& fullType) {
    if (typeTag.has_value()) {
        data.type = typeTag.value();
        if (getFullType().getBaseType().size() < 1) {data.fullType = fullType;}
    } 
    
    else if (!getFullType().getBaseType().empty()) {
        data.fullType = fullType;
        data.type = stringToNodeType(getFullType().getBaseType());
    }

    // Fallback handling
    if (data.type == NodeValueType::UNKNOWN) {
        DEBUG_LOG(LogLevel::PERMISSIVE, "");
        DEBUG_LOG(LogLevel::PERMISSIVE, "UNKNOWN TYPE -> OUTPUT: ", getType());
        throw MerkError("data.type is Unknown");
        data.type = NodeValueType::Any;  // fallback
    }

    DEBUG_LOG(LogLevel::PERMISSIVE, "data.type is ", getType());
}





// Default Constructor
Node::Node() : data() {
    DEBUG_LOG(LogLevel::DEBUG, "===== Node was created with default initialization.");
}

// Copy Constructor (Handles VarNode properly)
Node::Node(const Node& other) {
    if (this == &other) {return;}
    copyFlagsFrom(other);
    // this->data = other.data;
    // isConst = other.isConst;
    // isMutable = other.isMutable;
    // isStatic = other.isStatic;
    // isCallable = other.isCallable;
    // name = other.name;
    // if (isInstance()) {
    //     auto instance = std::get<SharedPtr<ClassInstance>>(getValue());
    //     instance->getInstanceNode()->isMutable = this->isMutable;
    // }

    DEBUG_LOG(LogLevel::DEBUG, "===== Node was copy-constructed.");
}

// Move Constructor
Node::Node(Node&& other) noexcept {
    auto otherData = other.data;
    // this->data = std::move(other.data);
    // other.data.type = NodeValueType::Null; // Reset moved-from object
    // isConst = other.isConst;
    // isMutable = other.isMutable;
    // isStatic = other.isStatic;
    // isCallable = other.isCallable;
    // name = other.name;
    copyFlagsFrom(other);
    data = otherData;
}

// Copy Assignment Operator
Node& Node::operator=(const Node& other) {
    if (this != &other) {
        if (const VarNode* varNode = dynamic_cast<const VarNode*>(&other)) {
            *this = VarNode(*varNode); // Call VarNode's copy assignment operator
        } else {
            
            copyFlagsFrom(other);
            
            DEBUG_LOG(LogLevel::DEBUG, "===== Node was copy-assigned.");
        }
    }
    return *this;
}

// Move Assignment Operator
Node& Node::operator=(Node&& other) noexcept {
    if (this != &other) {
        // data = std::move(other.data);
        // isConst = other.isConst;
        // isMutable = other.isMutable;
        // isStatic = other.isStatic;
        // isCallable = other.isCallable;
        // name = other.name;
        copyFlagsFrom(other);
        
        DEBUG_LOG(LogLevel::DEBUG, "===== Node was move-assigned.");
    }
    return *this;
}

// Constructor accepting a VariantType
Node::Node(const VariantType& value) {
    setInitialValue(value);
    validateTypeAlignment();
}

// Constructor accepting a string value and type
Node::Node(const String& value, const String& typeStr) {
    setInitialValue(value, typeStr);
    validateTypeAlignment();
}

// Destructor
Node::~Node() {
    DEBUG_LOG(LogLevel::DEBUG, "===== Node was destroyed.");
    // data.value._M_reset();
}

// Clone Method
Node* Node::clone() const {
    return new Node(*this);
}

void Node::setFullType(ResolvedType fullRType) const {
    data.fullType.setBaseType(fullRType.getBaseType());
    data.fullType.setInner(fullRType.getInnerType());
}

// Default constructor
LitNode::LitNode() : Node() {
    nodeType = "LitNode";
    DEBUG_LOG(LogLevel::TRACE, "===== LitNode was created without initialization.");
}

// Constructor accepting a VariantType
LitNode::LitNode(const VariantType& value) : Node(value) {
    nodeType = "LitNode";
    DEBUG_LOG(LogLevel::TRACE, "===== LitNode was initialized with VariantType.");
}

// Constructor accepting a string value and type
LitNode::LitNode(const String& value, const String& typeStr) : Node(value, typeStr) {
    nodeType = "LitNode";
    DEBUG_LOG(LogLevel::TRACE, "===== LitNode was initialized with String and typeStr.");
}

// Constructor accepting another Node
LitNode::LitNode(const Node& parentNode) : Node(parentNode) {
    nodeType = "LitNode";
    DEBUG_LOG(LogLevel::TRACE, "===== LitNode was initialized from another Node.");
}

// Copy constructor
LitNode::LitNode(const LitNode& other) : Node(other) {
    nodeType = "LitNode";
    DEBUG_LOG(LogLevel::TRACE, "===== LitNode was copy-constructed.");
}

// Move constructor
LitNode::LitNode(LitNode&& other) noexcept : Node(std::move(other)) {
    nodeType = "LitNode";
    DEBUG_LOG(LogLevel::TRACE, "===== LitNode was move-constructed.");
}

LitNode& LitNode::operator=(const LitNode& other) {
    nodeType = "LitNode";
    if (this != &other) {
        Node::operator=(other);
    }
    return *this;
}

// Move assignment operator
LitNode& LitNode::operator=(LitNode&& other) noexcept {
    nodeType = "LitNode";

    if (this != &other) {
        Node::operator=(std::move(other));
    }
    return *this;
}











// VarNode Default Constructor
VarNode::VarNode() : Node() {
    nodeType = "VarNode";
    DEBUG_LOG(LogLevel::DEBUG, "===== VarNode was created without initialization.");
}

// VarNode Constructor accepting VariantType
VarNode::VarNode(const VariantType& value, bool isConst, bool isMutable, bool isStatic)
    : Node(value) {
    nodeType = "VarNode";
    this->isConst = isConst;
    this->isMutable = isMutable;
    this->isStatic = isStatic;
    updateClassInstance(*this);
}

// VarNode Constructor accepting String value and type
VarNode::VarNode(const String& value, const String& typeStr, bool isConst, bool isMutable, bool isStatic)
    : Node(value, typeStr) {
    nodeType = "VarNode";
    this->isConst = isConst;
    this->isMutable = isMutable;
    this->isStatic = isStatic;
    updateClassInstance(*this);
}

// VarNode Constructor accepting another Node
VarNode::VarNode(const Node& parentNode, bool isConst, bool isMutable, bool isStatic)
    : Node(parentNode) {
    nodeType = "VarNode";

    this->isConst = parentNode.isConst || isConst;
    this->isMutable = parentNode.isMutable || isMutable;
    this->isStatic = parentNode.isStatic || isStatic;
    this->isCallable = parentNode.isCallable;
    this->name = parentNode.name;
    this->nodeType = parentNode.nodeType;
    updateClassInstance(*this);
}

// VarNode Copy Constructor
VarNode::VarNode(const VarNode& other) : Node(other) {
    

    this->isConst = other.isConst;
    this->isMutable = other.isMutable;
    this->isStatic = other.isStatic;
    this->isCallable = other.isCallable;
    this->name = other.name;
    this->nodeType = other.nodeType;

    copyFlagsFrom(other);
    nodeType = "VarNode";
    updateClassInstance(*this);
}

// VarNode Move Constructor
VarNode::VarNode(VarNode&& other) noexcept : Node(std::move(other)) {
    this->isConst = other.isConst;
    this->isMutable = other.isMutable;
    this->isStatic = other.isStatic;
    this->isCallable = other.isCallable;
    this->name = other.name;
    this->nodeType = other.nodeType;
    updateClassInstance(*this);
}

// Copy Assignment Operator
VarNode& VarNode::operator=(const VarNode& other) {
    if (this != &other) {
        Node::operator=(other);
    }

    updateClassInstance(*this);
    return *this;
}

// Move Assignment Operator
VarNode& VarNode::operator=(VarNode&& other) noexcept {
    if (this != &other) {
        Node::operator=(std::move(other));
    }
    updateClassInstance(*this);

    return *this;
}

VarNode* VarNode::clone() const {
    return new VarNode(*this);
}



// For Variable Name Part Construction
VarNode::VarNode(const String value, const String& typeStr, bool isConst, bool isMutable, std::optional<NodeValueType> typeTag, bool isStatic)
    : Node(value, typeStr) {
    nodeType = "VarNode";
    this->isConst = isConst;
    this->isMutable = isMutable;
    applyTypeInfo(typeTag, getFullType());

    this->isStatic = isStatic && typeTag.has_value();
    if (typeTag.has_value()){
        this->data.type = typeTag.value_or(NodeValueType::Any);
    }

    validateTypeAlignment();
}

// For Variable Name Part and ResolvedVariable Construction
VarNode::VarNode(VarNode& parent, bool isConst, bool isMutable, std::optional<NodeValueType> typeTag, bool isStatic)
    :Node(parent) {
    nodeType = "VarNode";
    copyFlagsFrom(parent);
    this->isStatic = typeTag.has_value() || isStatic;
    this->isConst = isConst;
    this->isMutable = isMutable;

    validateTypeAlignment();
}

VarNode::VarNode(const String value, const String& typeStr, bool isConst, bool isMutable, ResolvedType fullType, bool isStatic)
    : Node(value, typeStr) {
        nodeType = "VarNode";
        this->isConst = isConst;
        this->isMutable = isMutable;
        this->isStatic = fullType.getBaseType().size() || isStatic; 
        this->data.fullType = fullType;

        
        DEBUG_LOG(LogLevel::PERMISSIVE, "VarNode FullType: ", fullType.getBaseType(), "Type: ", typeStr);
        data.type = stringToNodeType(fullType.getBaseType());
        if (data.type == NodeValueType::UNKNOWN) {throw MerkError("data.type is Unknown");  data.type = NodeValueType::Any;}   // Temporary solution for user defined types
        
        DEBUG_LOG(LogLevel::PERMISSIVE, "data.type is ", data.type);
        validateTypeAlignment();
}

VarNode::VarNode(VarNode& parent, bool isConst, bool isMutable, ResolvedType fullType, bool isStatic)
    :Node (parent) {
        nodeType = "VarNode";
        this->isConst = isConst;
        this->isMutable = isMutable;
        this->isStatic = fullType.getBaseType().size() || isStatic; // && parent.data.type != NodeValueType::Uninitialized;
        applyTypeInfo(std::nullopt, fullType);

        this->data.type = parent.getType();
        
        if (data.type == NodeValueType::UNKNOWN) {DEBUG_LOG(LogLevel::PERMISSIVE, "UNKNOWN TYPE OUTPUT: ", data.type); throw MerkError("data.type is Unknown"); data.type = NodeValueType::Any;}   // Temporary solution for user defined types
        
        DEBUG_LOG(LogLevel::PERMISSIVE, "VarNode FullType: ", fullType.getBaseType(), "Type: ", parent.getTypeAsString());
        DEBUG_LOG(LogLevel::PERMISSIVE, "data.type is ", data.type);
        updateClassInstance(*this);

        validateTypeAlignment();
    }