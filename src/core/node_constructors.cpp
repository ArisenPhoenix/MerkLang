#include "core/node.h"
#include "core/types.h"
#include "utilities/debugger.h"
#include "core/errors.h"
#include "core/scope.h"



// Overload operator<< for Node to display detailed information
std::ostream& operator<<(std::ostream& os, const Node& node) {
    os << node.nodeType << "(";
    os << "Value: ";

    // Display the value
    try {
        os << node.toString();
    } catch (const std::exception& e) {
        os << "<Error retrieving value>";
    }

    SharedPtr<Scope> scope = nullptr;
    bool isInstance = node.isInstance();
    // Display the type
    os << ", Type: " << (nodeTypeToString(node.getType()) + "(" + node.name + ")" + " FullType: " + node.getFullType().toString());
    if (node.getType() == NodeValueType::ClassInstance){
        auto instance = std::get<SharedPtr<ClassInstance>>(node.getValue());
        scope = instance->getInstanceScope();

        // if (scope->localFunctions.size() == 0) {scope->debugPrint(); scope->printChildScopes(); throw RunTimeError("There Are No Functions in this instance of " + node.nodeType); }   
    }

    else {
        if (isInstance) {
            throw MerkError("IsInstance is instance, but not value type");
        }
    }

    
    // Display metadata
    os << ", isConst: " << (node.isConst ? "true" : "false");
    os << ", isMutable: " << (node.isMutable ? "true" : "false");
    os << ", isStatic: " << (node.isStatic ? "true" : "false");
    os << ", isCallable: " << (node.isCallable ? "true" : "false");
    os << ", isInstance: " << (isInstance ? "true" : "false");
    os << ", isList: " << (node.isList() ? "true" : "false");
    

    os << ")";

    if (scope){
        os << "\n INSTANCE: " + node.name + " DATA START In Scope : " + "Scope(" + std::to_string(scope->getScopeLevel()) + ", " + scope->owner + ")\n";
        // scope->debugPrint();
        for (auto& [varName, var] : scope->getContext().getVariables()) {
            os << varName << " = " << var->toString() << "\n";
        }
        for (auto& [funcName, funcVec] : scope->localFunctions) {
            for (auto& func : funcVec) {
                os << func->getCallable()->toString() << "\n";
            }
        }

        // if (scope->localFunctions.size() == 0) { throw MerkError("There Are No Functions in this instance of " + node.nodeType); }

        for (auto& [className, cls] : scope->localClasses) {
            os << cls->getCallable()->toString() << "\n";
        }
        
        os << "\n INSTANCE DATA END\n\n";
    }
    return os;
}


void validateInstances(const Node& current, const Node& other, int number, bool startedAsInstance = false) {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    DEBUG_LOG(LogLevel::PERMISSIVE, "NUMBER 2", current, other);
    String numStr = std::to_string(number);
    if (std::holds_alternative<SharedPtr<ListNode>>(current.getValue())) {
        throw MerkError("Current holds a ListNode " + numStr);
    }

    if (std::holds_alternative<SharedPtr<ListNode>>(other.getValue())) {
        throw MerkError("Other holds a ListNode " + numStr);
    }
    

    if (current.isInstance()) {
        if (!other.isInstance()) {throw MerkError("Only One Is An Instance");}
        if (current.getType() != other.getType()) {
            throw MerkError("Is Not A List but: " + nodeTypeToString(current.getType()) + "&" + nodeTypeToString(other.getType()) + " in VarNode::setValue " + numStr);
        } 

        if (std::holds_alternative<SharedPtr<ListNode>>(current.getValue())) {throw MerkError("Is an Instance that contains a List " + numStr);}
        auto inst = std::get<SharedPtr<ClassInstance>>(current.getValue());
        if (inst->getInstanceScope()->localFunctions.size() == 0) {throw MerkError("Zero Functions in classInstance in VarNode::setValue " + numStr);}
    } else {throw MerkError("Saying Not An Instance" + numStr);}


    if (other.isInstance()) {
        if (!other.isInstance()) {throw MerkError("Only One Is An Instance");}
        if (current.getType() != other.getType()) {
            throw MerkError("Is Not A List but: " + nodeTypeToString(current.getType()) + "&" + nodeTypeToString(other.getType()) + " in VarNode::setValue " + numStr);
        } 

        if (std::holds_alternative<SharedPtr<ListNode>>(other.getValue())) {throw MerkError("Is an Instance that contains a List " + numStr);}
        auto inst = std::get<SharedPtr<ClassInstance>>(other.getValue());
        if (inst->getInstanceScope()->localFunctions.size() == 0) {throw MerkError("Zero Functions in classInstance in VarNode::setValue " + numStr);}
    } else {throw MerkError("Saying Not An Instance" + numStr);}

    if (startedAsInstance && !other.isInstance()) {throw MerkError("Is No Longer An Instance " + numStr);}

}

void VarNode::setValue(const Node& other) {
    if (isConst) {throw MerkError("Cannot reassign a constant VarNode.");}
    if (isStatic && getType() != other.getType()) {throw MerkError("Cannot reassign a statically typed VarNode with a different type.");}
    
    // Only update the value, but not the metadata (isConst, isMutable, isStatic)
    data.fullType = other.getFullType();
    data.type = other.getType();
    if (getValue() == other.getValue()) {return;}
    data.value = other.getValue();
    
    DEBUG_LOG(LogLevel::DEBUG, "VarNode::setValue: Value updated, metadata remains unchanged.");
}


void Node::setValue(const Node& other) {
    if (isConst) { throw MerkError("Cannot reassign a constant Node."); }
    if (other.getType() == NodeValueType::Null) { throw MerkError("Cannot assign a Null Node to another Node."); }
    if (isStatic && getType() != other.getType()) { throw MerkError("Cannot reassign a statically-typed Node with a different type."); }
    if (*this == other) {throw MerkError("tried to set value to itself");}
    copyFlagsFrom(other);

    DEBUG_LOG(LogLevel::DEBUG, "Node::setValue: Value and metadata updated.");
}



void Node::copyFlagsFrom(const Node& other) {
    // if (other.isDict()) {throw MerkError("other Is A List");}
    // if (*this == other) {throw MerkError("Cannot set Node to itself");}
    
    if (*this != other) {
        // DEBUG_LOG(LogLevel::PERMISSIVE, "OTHER IS STRING: ", other);
        isConst = other.isConst;
        isMutable = other.isMutable;
        isStatic = getFullType().getBaseType().size() || isStatic;
        isCallable = other.isCallable;
        // if (getValue() == other.getValue()) {return;}
        name = other.name;
        nodeType = other.nodeType;
        key = other.key;
        
        data = other.data;

        setFullType(other.getFullType());
        if (data.type == NodeValueType::List && !std::holds_alternative<SharedPtr<ListNode>>(data.value)) {
            throw MerkError("When Copying Flags the NodeValueType is List, but the value is of type: " + nodeTypeToString(data.type));
        }
        updateClassInstance(*this);
    } 
    
    
    
}

void Node::transferOwnershipFrom(Node&& other) {
    isConst = other.isConst;
    isMutable = other.isMutable;
    isStatic = other.isStatic;
    isCallable = other.isCallable;
    name = std::move(other.name);
    nodeType = std::move(other.nodeType);
    // if (getValue() == other.getValue()) {return;}
    data = other.data;
    setFullType(other.getFullType());
    // if (data.type == NodeValueType::List && !std::holds_alternative<SharedPtr<ListNode>>(data.value)) {
    //     throw MerkError("When Copying Flags the NodeValueType is List, but the value is of type: " + nodeTypeToString(data.type));
    // }
    updateClassInstance(*this);
}


void Node::updateClassInstance(const Node& me) {
    if (isInstance()) {
        // if (nodeType == "List") {throw MerkError("Instance Is List, but not raw");}
        auto instance = toInstance();
        auto instanceNode = instance->getInstanceNode();
        instanceNode->isMutable = isMutable;
        instanceNode->isStatic = isStatic;
        instanceNode->isConst = isConst;
    }
    else {
        if (std::holds_alternative<SharedPtr<ClassInstance>>(me.data.value)) {
            data.value = me.data.value;
            data.type = NodeValueType::ClassInstance;
            DEBUG_LOG(LogLevel::PERMISSIVE, *this, "isClassInstance");
            throw MerkError("Instance Was Not Previously Copied Over in Node::updateClassInstance -> " + nodeType);
        }
        // else if (std::holds_alternative<SharedPtr<ListNode>>(me.data.value)) {
        //     data.value = me.data.value;
        //     data.type = NodeValueType::List;
        //     nodeType = "List";
        //     DEBUG_LOG(LogLevel::PERMISSIVE, *this, "isList");
        //     // throw MerkError("List Was Not Previously Copied Over in Node::updateClassInstance -> " + nodeType);
        // }
    }

    // throw MerkError("Not An Instance");


}

void Node::applyTypeInfo(std::optional<NodeValueType> typeTag, const ResolvedType& fullType) {
    if (typeTag.has_value()) {
        data.type = typeTag.value();
        if (getFullType().getBaseType().size() < 1) {setFullType(fullType);}
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
        setFullType(fullType);
    }

    DEBUG_LOG(LogLevel::PERMISSIVE, "data.type is ", getType());
}





// Default Constructor
Node::Node() : data() {
    DEBUG_LOG(LogLevel::DEBUG, "===== Node was created with default initialization.");
}

// Copy Constructor (Handles VarNode properly)
Node::Node(const Node& other) {
    // DEBUG_LOG(LogLevel::PERMISSIVE, "OTHER IS: ", other);

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
    // auto otherData = other.data;
    // this->data = std::move(other.data);
    // other.data.type = NodeValueType::Null; // Reset moved-from object
    // isConst = other.isConst;
    // isMutable = other.isMutable;
    // isStatic = other.isStatic;
    // isCallable = other.isCallable;
    // name = other.name;
    // copyFlagsFrom(std::move(other));
    transferOwnershipFrom(std::move(other));
    // data = otherData;
}

// Node Copy Assignment Operator
Node& Node::operator=(const Node& other) {
    if (this != &other) {
        if (const VarNode* varNode = dynamic_cast<const VarNode*>(&other)) {
            *this = VarNode(*varNode); // Call VarNode's copy assignment operator
            // copyFlagsFrom(other);
        } else {
            // DEBUG_LOG(LogLevel::PERMISSIVE, "*THIS IS: ", *this, "ADDRESS: ", this, "STRING: ", toString());
            // DEBUG_LOG(LogLevel::PERMISSIVE, "&OTHER IS: ", other, "ADDRESS: ", other.toString());
            
            // DEBUG_LOG(LogLevel::PERMISSIVE, "*THIS IS: ", *this, "ADDRESS: ", this, "STRING: ", toString());
            // // auto val = VarNode(*varNode);

            // DEBUG_LOG(LogLevel::PERMISSIVE, "*NewVarNode IS: ", *this, "ADDRESS: ", this);
            // // *this = val;
            // // if (getValue().valueless_by_exception())
            // DEBUG_LOG(LogLevel::DEBUG, "===== Node was copy-assigned.");
        }
    }
    // DEBUG_LOG(LogLevel::PERMISSIVE, "*THIS IS: ", *this, "ADDRESS: ", this);
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
        transferOwnershipFrom(std::move(other));
        
        DEBUG_LOG(LogLevel::DEBUG, "===== Node was move-assigned.");
    }
    return *this;
}

// Constructor accepting a VariantType
Node::Node(const VariantType& value) {
    setInitialValue(value);
    updateClassInstance(*this);
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
    data.value = 0;
}

// Clone Method
Node* Node::clone() const {
    return new Node(*this);
}

void Node::setFullType(ResolvedType fullRType) const {
    data.fullType.setBaseType(fullRType.getBaseType());
    data.fullType.setInner(fullRType.getInnerType());
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
// VarNode::VarNode(const Node &parentNode, bool isConst = false, bool isMutable = true, bool isStatic = false)
// VarNode Constructor accepting another Node
VarNode::VarNode(const Node& parentNode, bool isConst, bool isMutable, bool isStatic)
    : Node(parentNode) {
    nodeType = parentNode.nodeType;
    // nodeType = "VarNode";

    this->isConst = parentNode.isConst || isConst;
    this->isMutable = parentNode.isMutable || isMutable;
    this->isStatic = parentNode.isStatic || isStatic;
    this->isCallable = parentNode.isCallable;
    // this->name = parentNode.name;
    // this->nodeType = parentNode.nodeType;
    // this->data.value = parentNode.getValue();
    // this->data.type = parentNode.getType();
    // updateClassInstance(*this);
}

// VarNode Copy Constructor
VarNode::VarNode(const VarNode& other) : Node(other) {
    // DEBUG_LOG(LogLevel::PERMISSIVE, "OTHER IS: ", other);
    if (other != *this) {
        this->isConst = other.isConst;
        this->isMutable = other.isMutable;
        this->isStatic = other.isStatic;
        this->isCallable = other.isCallable;
        this->name = other.name;
        this->nodeType = other.nodeType;
        this->key = other.key;
        

        // copyFlagsFrom(other);
        nodeType = "VarNode";
    } 
    
}

// VarNode Move Constructor
VarNode::VarNode(VarNode&& other) noexcept : Node(std::move(other)) {
    // this->isConst = other.isConst;
    // this->isMutable = other.isMutable;
    // this->isStatic = other.isStatic;
    // this->isCallable = other.isCallable;
    // this->name = other.name;
    // this->nodeType = other.nodeType;
    transferOwnershipFrom(std::move(other));
    updateClassInstance(*this);
}

// Copy Assignment Operator
VarNode& VarNode::operator=(const VarNode& other) {
    if (this != &other) {
        Node::operator=(other);
    }

    // updateClassInstance(*this);
    return *this;
}

// Move Assignment Operator
VarNode& VarNode::operator=(VarNode&& other) noexcept {
    if (this != &other) {
        Node::operator=(std::move(other));
    }
    // updateClassInstance(*this);

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
    this->isStatic = isStatic && typeTag.has_value();

    // applyTypeInfo(typeTag, getFullType());
    if (typeTag.has_value()) {
        this->data.type = typeTag.value_or(NodeValueType::Any);
        auto theType = stringToNodeType(typeStr);
    if (theType != NodeValueType::UNKNOWN) {data.type = theType; data.fullType.setBaseType(typeStr);}
    }
    
    updateClassInstance(*this);
    validateTypeAlignment();
}

// For Variable Name Part and ResolvedVariable Construction
VarNode::VarNode(VarNode& parent, bool isConst, bool isMutable, std::optional<NodeValueType> typeTag, bool isStatic)
    : Node(parent) {
    nodeType = "VarNode";
    // if (parent == *this) {throw MerkError("Parent is Same as This");}
    if (parent != *this) { // this would otherwise throw on print
        copyFlagsFrom(parent);
        this->isStatic = typeTag.has_value() || isStatic;
        this->isConst = isConst;
        this->isMutable = isMutable;

        if (typeTag.has_value()) {
            auto theType = typeTag.value_or(NodeValueType::Any);
            data.type = theType;
            if (theType == NodeValueType::UNKNOWN) {
                data.type = NodeValueType::Any;
            }
        }
        updateClassInstance(*this);
        validateTypeAlignment();
    }
    
}

VarNode::VarNode(const String value, const String& typeStr, bool isConst, bool isMutable, ResolvedType fullType, bool isStatic)
    : Node(value, typeStr) {
        nodeType = "VarNode";
        this->isConst = isConst;
        this->isMutable = isMutable;
        this->isStatic = fullType.getBaseType().size() || isStatic; 
        this->data.fullType = fullType;
        
        data.type = stringToNodeType(fullType.getBaseType());
        if (data.type == NodeValueType::UNKNOWN) {throw MerkError("data.type is Unknown");  data.type = NodeValueType::Any;}   // Temporary solution for user defined types
        updateClassInstance(*this);
        validateTypeAlignment();
}

VarNode::VarNode(VarNode& parent, bool isConst, bool isMutable, ResolvedType fullType, bool isStatic)
    : Node (parent) {
        nodeType = "VarNode";
        this->isConst = isConst;
        this->isMutable = isMutable;
        this->isStatic = fullType.getBaseType().size() || isStatic; // && parent.data.type != NodeValueType::Uninitialized;
        this->data.type = parent.getType();
        this->setFullType(fullType);
        
        if (data.type == NodeValueType::UNKNOWN) {DEBUG_LOG(LogLevel::PERMISSIVE, "UNKNOWN TYPE OUTPUT: ", data.type); throw MerkError("data.type is Unknown"); data.type = NodeValueType::Any;}   // Temporary solution for user defined types
        updateClassInstance(*this);

        validateTypeAlignment();
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