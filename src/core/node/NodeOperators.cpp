#include "core/node/Node.hpp"
#include "core/node/NodeStructures.hpp"
#include "core/types.h"

#include "utilities/debugger.h"
#include "utilities/debugging_functions.h"

#include <cmath>

static inline Node evalNotEqual(const Node& a, const Node& b) {
    // Null handling
    if (a.isNull() && b.isNull()) return Node(false);
    if (a.isNull() != b.isNull()) return Node(true);

    // If both are non-null:
    // - compare by type then value
    if (a.getType() != b.getType()) return Node(true);

    // then compare underlying variant / or specialized compare
    return Node(a.getValue() != b.getValue());
}



SharedPtr<NodeBase> DynamicNode::applyAdd(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs + rhs;
}

SharedPtr<NodeBase> DynamicNode::applySub(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs - rhs;
}

SharedPtr<NodeBase> DynamicNode::applyMul(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs * rhs;
}

SharedPtr<NodeBase> DynamicNode::applyDiv(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs / rhs;
}

SharedPtr<NodeBase> DynamicNode::applyMod(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs % rhs;
}



void DynamicNode::applyAddMut(NodeBase& lhs, const NodeBase& rhs) {
    lhs.setValue((lhs + rhs)->getValue());
}

void DynamicNode::applySubMut(NodeBase& lhs, const NodeBase& rhs) {
    lhs.setValue((lhs - rhs)->getValue());
}

void DynamicNode::applyMulMut(NodeBase& lhs, const NodeBase& rhs) {
    lhs.setValue((lhs * rhs)->getValue());
}

void DynamicNode::applyDivMut(NodeBase& lhs, const NodeBase& rhs) {
    lhs.setValue((lhs / rhs)->getValue());
}

SharedPtr<NodeBase> DynamicNode::applyEq(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs == rhs;
}

SharedPtr<NodeBase> DynamicNode::applyNeq(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs != rhs;
}

SharedPtr<NodeBase> DynamicNode::applyLt(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs < rhs;
}
SharedPtr<NodeBase> DynamicNode::applyGt(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs > rhs;
}
SharedPtr<NodeBase> DynamicNode::applyLe(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs <= rhs;
}
SharedPtr<NodeBase> DynamicNode::applyGe(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs >= rhs;
}



// ... same pattern for others




// Overload operator<< for Node to display detailed information
std::ostream& operator<<(std::ostream& os, const Node& node) {
    os << "(";
    // Display the value
    try {
        os << node.toString();
    } catch (const std::exception& e) {
        os << "<Error retrieving value>";
    }

    SharedPtr<Scope> scope = nullptr;
    os << ", Type: " << (nodeTypeToString(node.getType()) + "(" + node.toString() + ")" + " FullType: " + node.getFlags().fullType.toString());
    
    // Display metadata
    os << ", isConst: " << (node.getFlags().isConst ? "true" : "false");
    os << ", isMutable: " << (node.getFlags().isMutable ? "true" : "false");
    os << ", isStatic: " << (node.getFlags().isStatic ? "true" : "false");
    os << ")";
    return os;
}

String operator+(const String& lhs, const Node& node) {
    std::ostringstream oss;
    oss << lhs << node.toString();
    return oss.str();
}

String operator+(const Node& node, const String& rhs) {
    std::ostringstream oss;
    oss << node.toString() << rhs;
    return oss.str();
}



// Calculation Operators
SharedPtr<NodeBase> DynamicNode::operator+(const NodeBase& other) const {
    if (isNumeric() && other.isNumeric()) {
        return makeShared<DynamicNode>(toInt() + other.toInt());
    }

    if (isString() && other.isString()) {
        return makeShared<DynamicNode>(toString() + other.toString());
    } 
    throw MerkError("Cannot Add non String non Number types");     
}

SharedPtr<NodeBase> DynamicNode::operator-(const NodeBase& other) const {
    if (isNumeric() && other.isNumeric()) {
        return makeShared<DynamicNode>(toInt() - other.toInt());
    }

    if (isString() && other.isString()) {
        return makeShared<DynamicNode>(Node(toString()) - Node(other.toString()));
    } 
    throw MerkError("Cannot Add non String non Number types");
}
SharedPtr<NodeBase> DynamicNode::operator*(const NodeBase& other) const {
    if (isNumeric() && other.isNumeric()) {
        return makeShared<DynamicNode>(toInt() * other.toInt());
    }

    if (isString() && other.isString()) {
        return makeShared<DynamicNode>(Node(toString()) * Node(other.toString()));
    }

    if (isString() && other.isNumeric()) {
        return makeShared<DynamicNode>(Node(toString()) * Node::fromVariant(other.getValue()));
    }
    throw MerkError("Cannot Add non String non Number types"); 
}
SharedPtr<NodeBase> DynamicNode::operator/(const NodeBase& other) const {
    if (!isNumeric()) {throw MerkError("Cannot Divide " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");}
    return makeShared<DynamicNode>(toInt() / other.toInt());
}


SharedPtr<NodeBase> DynamicNode::operator%(const NodeBase& other) const {
    if (!isNumeric() || !other.isNumeric()) {throw MerkError("Attempted Modulus With " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");}
    return makeShared<DynamicNode>(toInt() % other.toInt());
}

// Mutating Operations
SharedPtr<NodeBase> DynamicNode::operator+=(const NodeBase& other) {
    if ((isNumeric() && other.isNumeric()) || (isString() && other.isString())) {
        return makeShared<DynamicNode>(Node::fromVariant(value) + Node::fromVariant(other.getValue())); 
    }
    throw MerkError("Cannot Add " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");
}
SharedPtr<NodeBase> DynamicNode::operator-=(const NodeBase& other) {
    if ((isNumeric() && other.isNumeric()) || (isString() && other.isString())) {
        return makeShared<DynamicNode>(Node::fromVariant(value) - Node::fromVariant(other.getValue())); 
    }
    throw MerkError("Cannot Subtract " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");
}
SharedPtr<NodeBase> DynamicNode::operator*=(const NodeBase& other) {
    if ((isNumeric() && other.isNumeric()) || (isString() && other.isString())) {
        return makeShared<DynamicNode>(Node::fromVariant(value) * Node::fromVariant(other.getValue())); 
    }
    throw MerkError("Cannot Multiply " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");
}
SharedPtr<NodeBase> DynamicNode::operator/=(const NodeBase& other) {
    if ((isNumeric() && other.isNumeric()) || (isString() && other.isString())) {
        return makeShared<DynamicNode>(Node::fromVariant(value) / Node::fromVariant(other.getValue())); 
    }
    throw MerkError("Cannot Subtract " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");
}

// Logic Operations
SharedPtr<NodeBase> DynamicNode::operator==(const NodeBase& other) const {
    // throw MerkError("Attempting Equality Check With DynamicNode Holding " + toString() + "OTHER HOLDING " + other.toString());
    return makeShared<BoolNode>(getValue() == other.getValue()); 
    // throw MerkError("Attempted == on DynamicNode::operator== -> " + nodeTypeToString(DynamicNode::getTypeFromValue(value)) + " Other " + nodeTypeToString(DynamicNode::getTypeFromValue(other.getValue())));
    // return makeShared<DynamicNode>(toInt() == other.toInt());
}
SharedPtr<NodeBase> DynamicNode::operator!=(const NodeBase& other) const {
    return makeShared<BoolNode>(getValue() != other.getValue()); 
}
SharedPtr<NodeBase> DynamicNode::operator<(const NodeBase& other) const {
    return makeShared<DynamicNode>(toInt() < other.toInt());
}
SharedPtr<NodeBase> DynamicNode::operator>(const NodeBase& other) const {
    return makeShared<DynamicNode>(toInt() > other.toInt());
}
SharedPtr<NodeBase> DynamicNode::operator<=(const NodeBase& other) const {
    return makeShared<DynamicNode>(toInt() <= other.toInt());
}
SharedPtr<NodeBase> DynamicNode::operator>=(const NodeBase& other) const {
    return makeShared<DynamicNode>(toInt() >= other.toInt());
}














// Calculation Operators
SharedPtr<NodeBase> NullNode::operator+(const NodeBase& other) const {
    if (isNumeric() && other.isNumeric()) {
        return makeShared<DynamicNode>(toInt() + other.toInt());
    }

    if (isString() && other.isString()) {
        return makeShared<DynamicNode>(toString() + other.toString());
    } 
    throw MerkError("Cannot Add non String non Number types");     
}

SharedPtr<NodeBase> NullNode::operator-(const NodeBase& other) const {
    if (isNumeric() && other.isNumeric()) {
        return makeShared<DynamicNode>(toInt() - other.toInt());
    }

    if (isString() && other.isString()) {
        return makeShared<DynamicNode>(Node(toString()) - Node(other.toString()));
    } 
    throw MerkError("Cannot Add non String non Number types");
}
SharedPtr<NodeBase> NullNode::operator*(const NodeBase& other) const {
    if (isNumeric() && other.isNumeric()) {
        return makeShared<DynamicNode>(toInt() * other.toInt());
    }

    if (isString() && other.isString()) {
        return makeShared<DynamicNode>(Node(toString()) * Node(other.toString()));
    }

    if (isString() && other.isNumeric()) {
        return makeShared<DynamicNode>(Node(toString()) * Node::fromVariant(other.getValue()));
    }
    throw MerkError("Cannot Add non String non Number types"); 
}
SharedPtr<NodeBase> NullNode::operator/(const NodeBase& other) const {
    if (!isNumeric()) {throw MerkError("Cannot Divide " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");}
    return makeShared<DynamicNode>(toInt() / other.toInt());
}


SharedPtr<NodeBase> NullNode::operator%(const NodeBase& other) const {
    if (!isNumeric() || !other.isNumeric()) {throw MerkError("Attempted Modulus With " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");}
    return makeShared<DynamicNode>(toInt() % other.toInt());
}

// Mutating Operations
SharedPtr<NodeBase> NullNode::operator+=(const NodeBase& other) {
    if ((isNumeric() && other.isNumeric()) || (isString() && other.isString())) {
        return makeShared<DynamicNode>(Node(value) + Node::fromVariant(other.getValue())); 
    }
    throw MerkError("Cannot Add " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");
}
SharedPtr<NodeBase> NullNode::operator-=(const NodeBase& other) {
    if ((isNumeric() && other.isNumeric()) || (isString() && other.isString())) {
        return makeShared<DynamicNode>(Node(value) - Node::fromVariant(other.getValue())); 
    }
    throw MerkError("Cannot Subtract " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");
}
SharedPtr<NodeBase> NullNode::operator*=(const NodeBase& other) {
    if ((isNumeric() && other.isNumeric()) || (isString() && other.isString())) {
        return makeShared<DynamicNode>(Node(value) * Node::fromVariant(other.getValue())); 
    }
    throw MerkError("Cannot Multiply " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");
}
SharedPtr<NodeBase> NullNode::operator/=(const NodeBase& other) {
    if ((isNumeric() && other.isNumeric()) || (isString() && other.isString())) {
        return makeShared<DynamicNode>(Node(value) / Node::fromVariant(other.getValue())); 
    }
    throw MerkError("Cannot Subtract " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");
}

// Logic Operations
SharedPtr<NodeBase> NullNode::operator==(const NodeBase& other) const {
    // throw MerkError("Attempting Equality Check With DynamicNode Holding " + toString() + "OTHER HOLDING " + other.toString());
    throw MerkError("Attempted == on NullNode::operator== -> " + nodeTypeToString(DynamicNode::getTypeFromValue(value)) + " Other " + nodeTypeToString(DynamicNode::getTypeFromValue(other.getValue())));
    // return makeShared<DynamicNode>(toInt() == other.toInt());
}
SharedPtr<NodeBase> NullNode::operator!=(const NodeBase& other) const {
    return makeShared<DynamicNode>(toInt() != other.toInt());
}
SharedPtr<NodeBase> NullNode::operator<(const NodeBase& other) const {
    return makeShared<DynamicNode>(toInt() < other.toInt());
}
SharedPtr<NodeBase> NullNode::operator>(const NodeBase& other) const {
    return makeShared<DynamicNode>(toInt() > other.toInt());
}
SharedPtr<NodeBase> NullNode::operator<=(const NodeBase& other) const {
    return makeShared<DynamicNode>(toInt() <= other.toInt());
}
SharedPtr<NodeBase> NullNode::operator>=(const NodeBase& other) const {
    return makeShared<DynamicNode>(toInt() >= other.toInt());
}



















// Operators 
SharedPtr<NodeBase> NodeBase::operator+(const NodeBase& other) const {(void)other; throw MerkError("Invalid op +"); }
SharedPtr<NodeBase> NodeBase::operator-(const NodeBase& other) const {(void)other; throw MerkError("Invalid op -"); }
SharedPtr<NodeBase> NodeBase::operator*(const NodeBase& other) const {(void)other; throw MerkError("Invalid op *"); }
SharedPtr<NodeBase> NodeBase::operator/(const NodeBase& other) const {(void)other; throw MerkError("Invalid op /"); }
SharedPtr<NodeBase> NodeBase::operator%(const NodeBase& other) const {(void)other; throw MerkError("Invalid op %"); }

SharedPtr<NodeBase> NodeBase::operator==(const NodeBase& other) const {
    (void)other;
    // (void)other; 
    // throw MerkError("Invalid op == for Type " + flags.toString() + " DATA: " + toString() + " Determined as type: " + nodeTypeToString(DynamicNode::getTypeFromValue(getValue())) + " OTHER IS " + other.flags.toString() + " DATA: " + other.toString() + " Determined type as" + nodeTypeToString(DynamicNode::getTypeFromValue(other.getValue())));
    // return makeShared<BoolNode>(true);
    // throw MerkError("NodeBase::operator==, See Above ");
    return makeShared<BoolNode>(other.toString() == toString());
}

// SharedPtr<NodeBase> NodeBase::operator==(const NodeBase& other) const {(void)other; return makeShared<BoolNode>(false);}
SharedPtr<NodeBase> NodeBase::operator!=(const NodeBase& other) const {(void)other; throw MerkError("Invalid op !="); }
SharedPtr<NodeBase> NodeBase::operator<(const NodeBase& other) const {(void)other; throw MerkError("Invalid op <"); }
SharedPtr<NodeBase> NodeBase::operator>(const NodeBase& other) const {(void)other; throw MerkError("Invalid op >"); }
SharedPtr<NodeBase> NodeBase::operator<=(const NodeBase& other) const {(void)other; throw MerkError("Invalid op <="); }
SharedPtr<NodeBase> NodeBase::operator>=(const NodeBase& other) const {(void)other; throw MerkError("Invalid op >="); }


SharedPtr<NodeBase> NodeBase::operator+=(const NodeBase& other) {(void)other; throw MerkError("Invalid op +="); }
SharedPtr<NodeBase> NodeBase::operator-=(const NodeBase& other) {(void)other; throw MerkError("Invalid op -="); }
SharedPtr<NodeBase> NodeBase::operator*=(const NodeBase& other) {(void)other; throw MerkError("Invalid op *="); }
SharedPtr<NodeBase> NodeBase::operator/=(const NodeBase& other) {(void)other; throw MerkError("Invalid op /="); }





Node Node::operator+(const Node& other) const { return Node((*data) + (*other.data)); }
Node Node::operator-(const Node& other) const { return Node((*data) - (*other.data)); }
Node Node::operator*(const Node& other) const {return Node((*data) * (*other.data)); }
Node Node::operator/(const Node& other) const {return Node((*data) / (*other.data)); }
Node Node::operator%(const Node& other) const {return Node((*data) % (*other.data));}

Node Node::operator+=(const Node& other) const {
    if (!data) {throw MerkError("Cannot perform += on null value");}
    *this->data += (*other.data);
    return *this;
}
    // return Node((*data) += (*other.data)); }
Node Node::operator-=(const Node& other) const {return Node((*data) -= (*other.data)); }
Node Node::operator*=(const Node& other) const {return Node((*data) *= (*other.data)); }
Node Node::operator/=(const Node& other) const {return Node((*data) /= (*other.data)); }

bool Node::operator==(const Node& other) const {
    // if (toString() == "null") {
    //     return false;
    // }
    // return Node((*data) == (*other.data)).toBool(); 
    return !evalNotEqual(*this, other).toBool(); 
    
    // throw MerkError("Attempting Equality Check With Node Holding " + toString()); 
}
bool Node::operator!=(const Node& other) const {return evalNotEqual(*this, other).toBool(); /*return Node((*data) != (*other.data)).toBool();*/ }
bool Node::operator<(const Node& other) const {return Node((*data) < (*other.data)).toBool(); }
bool Node::operator>(const Node& other) const {return Node((*data) > (*other.data)).toBool(); }
bool Node::operator<=(const Node& other) const {return Node((*data) <= (*other.data)).toBool(); }
bool Node::operator>=(const Node& other) const {return Node((*data) >= (*other.data)).toBool(); }




// Calculation Operators
SharedPtr<NodeBase> BoolNode::operator+(const NodeBase& other) const {
    return makeShared<IntNode>(value + other.toInt());
}
SharedPtr<NodeBase> BoolNode::operator-(const NodeBase& other) const {
    throw MerkError("Attempted Bool - " + nodeTypeToString(other.getType()));
}
SharedPtr<NodeBase> BoolNode::operator*(const NodeBase& other) const {
    throw MerkError("Attempted Bool * " + nodeTypeToString(other.getType()));
}
SharedPtr<NodeBase> BoolNode::operator/(const NodeBase& other) const {
    throw MerkError("Attempted Bool / " + nodeTypeToString(other.getType()));
}


SharedPtr<NodeBase> BoolNode::operator%(const NodeBase& other) const {
    throw MerkError("Attempted Bool % " + nodeTypeToString(other.getType()));
}

// Mutating Operations
SharedPtr<NodeBase> BoolNode::operator+=(const NodeBase& other) {
    DynamicNode::applyAddMut(*this, other);
    return shared_from_this();
}

SharedPtr<NodeBase> BoolNode::operator-=(const NodeBase& other) {
    DynamicNode::applySubMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> BoolNode::operator*=(const NodeBase& other) {
    DynamicNode::applyMulMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> BoolNode::operator/=(const NodeBase& other) {
    DynamicNode::applyDivMut(*this, other);
    return shared_from_this();
}

// Logic Operations
SharedPtr<NodeBase> BoolNode::operator==(const NodeBase& other) const {
    return makeShared<BoolNode>(value == other.toBool());
}
SharedPtr<NodeBase> BoolNode::operator!=(const NodeBase& other) const {
    return makeShared<BoolNode>(this->toBool() != other.toBool());
}
SharedPtr<NodeBase> BoolNode::operator<(const NodeBase& other) const {
    return makeShared<BoolNode>(toInt() < other.toInt());
}
SharedPtr<NodeBase> BoolNode::operator>(const NodeBase& other) const {
    return makeShared<BoolNode>(toInt() > other.toInt());
}
SharedPtr<NodeBase> BoolNode::operator<=(const NodeBase& other) const {
    return makeShared<BoolNode>(toInt() <= other.toInt());
}
SharedPtr<NodeBase> BoolNode::operator>=(const NodeBase& other) const {
    return makeShared<BoolNode>(toInt() >= other.toInt());
}



// Calculation Operators
SharedPtr<NodeBase> StringNode::operator+(const NodeBase& other) const {
    if (!other.isString()) {throw MerkError("Attempted String + " + nodeTypeToString(other.getType()));}
    return makeShared<StringNode>(value + other.toString());
}
SharedPtr<NodeBase> StringNode::operator-(const NodeBase& other) const {
    if (!other.isString()) {throw MerkError("Attempted String - " + nodeTypeToString(other.getType()));}
    auto thisStr = toString();
    auto sub = other.toString();
    if (sub.size() < thisStr.size()) {
        size_t pos = thisStr.find(sub);
        if (pos != std::string::npos) {
            thisStr.erase(pos, sub.length());  // erase from pos, length of sub
        }
    }
    return makeShared<StringNode>(thisStr);
}
SharedPtr<NodeBase> StringNode::operator*(const NodeBase& other) const {
    if (!other.isInt()) { throw MerkError("Attempted String * " + nodeTypeToString(other.getType())); }
    auto times = other.toInt();
    auto thisStr = toString();
    if (times == 0) { thisStr.clear(); }
    
    else {
        std::string base = thisStr;
        for (int count = 1; count < times; ++count) {
            thisStr += base;
        }
    }
    return makeShared<StringNode>(thisStr);
}
SharedPtr<NodeBase> StringNode::operator/(const NodeBase& other) const {
    if (!other.isString()) {throw MerkError("Attempted String / " + nodeTypeToString(other.getType()));}
    auto thisStr = toString();
    auto sub = other.toString();
    if (sub.size() < thisStr.size()) {
        size_t pos;
        while ((pos = thisStr.find(sub)) != std::string::npos) {
            thisStr.erase(pos, sub.length());
        }
    }
    return makeShared<StringNode>(thisStr);
}


SharedPtr<NodeBase> StringNode::operator%(const NodeBase& other) const {
    throw MerkError("Attempted String % " + nodeTypeToString(other.getType()));
}

// Mutating Operations
SharedPtr<NodeBase> StringNode::operator+=(const NodeBase& other) {
    DynamicNode::applyAddMut(*this, other);
    return shared_from_this();
}

SharedPtr<NodeBase> StringNode::operator-=(const NodeBase& other) {
    DynamicNode::applySubMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> StringNode::operator*=(const NodeBase& other) {
    DynamicNode::applyMulMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> StringNode::operator/=(const NodeBase& other) {
    DynamicNode::applyDivMut(*this, other);
    return shared_from_this();
}

// Logic Operations
SharedPtr<NodeBase> StringNode::operator==(const NodeBase& other) const {
    // throw MerkError("Attempting Equality Check With StringNode Holding " + toString() + "OTHER HOLDING " + other.toString());

    return makeShared<BoolNode>(value == other.toString());
}
SharedPtr<NodeBase> StringNode::operator!=(const NodeBase& other) const {
    return makeShared<BoolNode>(value != other.toString());
}
SharedPtr<NodeBase> StringNode::operator<(const NodeBase& other) const {
    return makeShared<BoolNode>(value < other.toString());
}
SharedPtr<NodeBase> StringNode::operator>(const NodeBase& other) const {
    return makeShared<BoolNode>(value > other.toString());
}
SharedPtr<NodeBase> StringNode::operator<=(const NodeBase& other) const {
    return makeShared<BoolNode>(value <= other.toString());
}
SharedPtr<NodeBase> StringNode::operator>=(const NodeBase& other) const {
    return makeShared<BoolNode>(value >= other.toString());
}



// Calculation Operators
SharedPtr<NodeBase> CharNode::operator+(const NodeBase& other) const {
    if (!other.isString()) {throw MerkError("Attempted String + " + nodeTypeToString(other.getType()));}
    return makeShared<CharNode>(value + other.toString());
}
SharedPtr<NodeBase> CharNode::operator-(const NodeBase& other) const {
    if (!other.isString()) {throw MerkError("Attempted String - " + nodeTypeToString(other.getType()));}
    auto thisStr = toString();
    auto sub = other.toString();
    if (sub.size() < thisStr.size()) {
        size_t pos = thisStr.find(sub);
        if (pos != std::string::npos) {
            thisStr.erase(pos, sub.length());  // erase from pos, length of sub
        }
    }
    return makeShared<CharNode>(thisStr);
}
SharedPtr<NodeBase> CharNode::operator*(const NodeBase& other) const {
    if (!other.isInt()) { throw MerkError("Attempted String * " + nodeTypeToString(other.getType())); }
    auto times = other.toInt();
    auto thisStr = toString();
    if (times == 0) { thisStr.clear(); }
    
    else {
        std::string base = thisStr;
        for (int count = 1; count < times; ++count) {
            thisStr += base;
        }
    }
    return makeShared<CharNode>(thisStr);
}
SharedPtr<NodeBase> CharNode::operator/(const NodeBase& other) const {
    if (!other.isString()) {throw MerkError("Attempted String / " + nodeTypeToString(other.getType()));}
    auto thisStr = toString();
    auto sub = other.toString();
    if (sub.size() < thisStr.size()) {
        size_t pos;
        while ((pos = thisStr.find(sub)) != std::string::npos) {
            thisStr.erase(pos, sub.length());
        }
    }
    return makeShared<CharNode>(thisStr);
}


SharedPtr<NodeBase> CharNode::operator%(const NodeBase& other) const {
    throw MerkError("Attempted String % " + nodeTypeToString(other.getType()));
}

// Mutating Operations
SharedPtr<NodeBase> CharNode::operator+=(const NodeBase& other) {
    DynamicNode::applyAddMut(*this, other);
    return shared_from_this();
}

SharedPtr<NodeBase> CharNode::operator-=(const NodeBase& other) {
    DynamicNode::applySubMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> CharNode::operator*=(const NodeBase& other) {
    DynamicNode::applyMulMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> CharNode::operator/=(const NodeBase& other) {
    DynamicNode::applyDivMut(*this, other);
    return shared_from_this();
}

// Logic Operations
SharedPtr<NodeBase> CharNode::operator==(const NodeBase& other) const {
    return makeShared<BoolNode>(value == other.toChars());
}
SharedPtr<NodeBase> CharNode::operator!=(const NodeBase& other) const {
    return makeShared<BoolNode>(value != other.toChars());
}
SharedPtr<NodeBase> CharNode::operator<(const NodeBase& other) const {
    return makeShared<BoolNode>(value < other.toChars());
}
SharedPtr<NodeBase> CharNode::operator>(const NodeBase& other) const {
    return makeShared<BoolNode>(value > other.toChars());
}
SharedPtr<NodeBase> CharNode::operator<=(const NodeBase& other) const {
    return makeShared<BoolNode>(value <= other.toChars());
}
SharedPtr<NodeBase> CharNode::operator>=(const NodeBase& other) const {
    return makeShared<BoolNode>(value >= other.toChars());
}



// Calculation Operators
SharedPtr<NodeBase> IntNode::operator+(const NodeBase& other) const {
    return makeShared<IntNode>(value + other.toInt());
}
SharedPtr<NodeBase> IntNode::operator-(const NodeBase& other) const {
    return makeShared<IntNode>(value - other.toInt());
}
SharedPtr<NodeBase> IntNode::operator*(const NodeBase& other) const {
    return makeShared<IntNode>(value * other.toInt());
}
SharedPtr<NodeBase> IntNode::operator/(const NodeBase& other) const {
    return makeShared<IntNode>(value / other.toInt());
}
SharedPtr<NodeBase> IntNode::operator%(const NodeBase& other) const {
    return makeShared<IntNode>(value % other.toInt());
}

// Mutating Operations
SharedPtr<NodeBase> IntNode::operator+=(const NodeBase& other) {
    DynamicNode::applyAddMut(*this, other);
    return shared_from_this();
}

SharedPtr<NodeBase> IntNode::operator-=(const NodeBase& other) {
    DynamicNode::applySubMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> IntNode::operator*=(const NodeBase& other) {
    DynamicNode::applyMulMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> IntNode::operator/=(const NodeBase& other) {
    DynamicNode::applyDivMut(*this, other);
    return shared_from_this();
}

// SharedPtr<NodeBase> IntNode::operator/=(const NodeBase& other) {
//     auto val = value;
//     val /= other.toInt();
//     return makeShared<IntNode>(val);
// }

// Logic Operations
SharedPtr<NodeBase> IntNode::operator==(const NodeBase& other) const {
    // throw MerkError("Attempting Equality Check With IntNode Holding " + toString() + "OTHER HOLDING " + other.toString());
    if (!other.isNumeric()) {
        return makeShared<BoolNode>(false);
    }
    return makeShared<BoolNode>(value == other.toInt());
    // throw MerkError("Attempting Equality Check With IntNode Holding " + toString() + " OTHER HOLDING " + other.toString() + " Meta " + other.flags.toString());
    // return makeShared<BoolNode>(value == other.toInt());
}
SharedPtr<NodeBase> IntNode::operator!=(const NodeBase& other) const {
    return makeShared<BoolNode>(value != other.toInt());
}
SharedPtr<NodeBase> IntNode::operator<(const NodeBase& other) const {
    return makeShared<BoolNode>(value < other.toInt());
}
SharedPtr<NodeBase> IntNode::operator>(const NodeBase& other) const {
    return makeShared<BoolNode>(value > other.toInt());
}
SharedPtr<NodeBase> IntNode::operator<=(const NodeBase& other) const {
    return makeShared<BoolNode>(value <= other.toInt());
}
SharedPtr<NodeBase> IntNode::operator>=(const NodeBase& other) const {
    return makeShared<BoolNode>(value >= other.toInt());
}


// Calculation Operators
SharedPtr<NodeBase> FloatNode::operator+(const NodeBase& other) const {
    return makeShared<FloatNode>(value + other.toDouble());
}
SharedPtr<NodeBase> FloatNode::operator-(const NodeBase& other) const {
    return makeShared<FloatNode>(value - other.toDouble());
}
SharedPtr<NodeBase> FloatNode::operator*(const NodeBase& other) const {
    return makeShared<FloatNode>(value * other.toDouble());
}
SharedPtr<NodeBase> FloatNode::operator/(const NodeBase& other) const {
    return makeShared<FloatNode>(value / other.toDouble());
}
SharedPtr<NodeBase> FloatNode::operator%(const NodeBase& other) const {
    double rhs = other.toDouble();
    return makeShared<FloatNode>(std::fmod(value, rhs));
}

// Mutating Operations
SharedPtr<NodeBase> FloatNode::operator+=(const NodeBase& other) {
    DynamicNode::applyAddMut(*this, other);
    return shared_from_this();
}

SharedPtr<NodeBase> FloatNode::operator-=(const NodeBase& other) {
    DynamicNode::applySubMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> FloatNode::operator*=(const NodeBase& other) {
    DynamicNode::applyMulMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> FloatNode::operator/=(const NodeBase& other) {
    DynamicNode::applyDivMut(*this, other);
    return shared_from_this();
}

// Logic Operations
SharedPtr<NodeBase> FloatNode::operator==(const NodeBase& other) const {
    return makeShared<BoolNode>(value == other.toDouble());
}
SharedPtr<NodeBase> FloatNode::operator!=(const NodeBase& other) const {
    return makeShared<BoolNode>(value != other.toDouble());
}
SharedPtr<NodeBase> FloatNode::operator<(const NodeBase& other) const {
    return makeShared<BoolNode>(value < other.toDouble());
}
SharedPtr<NodeBase> FloatNode::operator>(const NodeBase& other) const {
    return makeShared<BoolNode>(value > other.toDouble());
}
SharedPtr<NodeBase> FloatNode::operator<=(const NodeBase& other) const {
    return makeShared<BoolNode>(value <= other.toDouble());
}
SharedPtr<NodeBase> FloatNode::operator>=(const NodeBase& other) const {
    return makeShared<BoolNode>(value >= other.toDouble());
}

// Calculation Operators
SharedPtr<NodeBase> DoubleNode::operator+(const NodeBase& other) const {
    return makeShared<DoubleNode>(value + other.toDouble());
}
SharedPtr<NodeBase> DoubleNode::operator-(const NodeBase& other) const {
    return makeShared<DoubleNode>(value - other.toDouble());
}
SharedPtr<NodeBase> DoubleNode::operator*(const NodeBase& other) const {
    return makeShared<DoubleNode>(value * other.toDouble());
}
SharedPtr<NodeBase> DoubleNode::operator/(const NodeBase& other) const {
    return makeShared<DoubleNode>(value / other.toDouble());
}
SharedPtr<NodeBase> DoubleNode::operator%(const NodeBase& other) const {
    double rhs = other.toDouble();
    return makeShared<DoubleNode>(std::fmod(value, rhs));
}

// Mutating Operations
SharedPtr<NodeBase> DoubleNode::operator+=(const NodeBase& other) {
    DynamicNode::applyAddMut(*this, other);
    return shared_from_this();
}

SharedPtr<NodeBase> DoubleNode::operator-=(const NodeBase& other) {
    DynamicNode::applySubMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> DoubleNode::operator*=(const NodeBase& other) {
    DynamicNode::applyMulMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> DoubleNode::operator/=(const NodeBase& other) {
    DynamicNode::applyDivMut(*this, other);
    return shared_from_this();
}

// Logic Operations
SharedPtr<NodeBase> DoubleNode::operator==(const NodeBase& other) const {
    return makeShared<BoolNode>(value == other.toDouble());
}
SharedPtr<NodeBase> DoubleNode::operator!=(const NodeBase& other) const {
    return makeShared<BoolNode>(value != other.toDouble());
}
SharedPtr<NodeBase> DoubleNode::operator<(const NodeBase& other) const {
    return makeShared<BoolNode>(value < other.toDouble());
}
SharedPtr<NodeBase> DoubleNode::operator>(const NodeBase& other) const {
    return makeShared<BoolNode>(value > other.toDouble());
}
SharedPtr<NodeBase> DoubleNode::operator<=(const NodeBase& other) const {
    return makeShared<BoolNode>(value <= other.toDouble());
}
SharedPtr<NodeBase> DoubleNode::operator>=(const NodeBase& other) const {
    return makeShared<BoolNode>(value >= other.toDouble());
}
