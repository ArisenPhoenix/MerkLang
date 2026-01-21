#include "core/node/Node.hpp"
#include "core/node/NodeStructures.hpp"
#include "core/types.h"

#include "utilities/debugger.h"
#include "utilities/debugging_functions.h"
#include "core/errors.h"
#include "core/evaluators/TypeEvaluator.hpp"

#include <cmath>

static inline Node evalNotEqual(const Node& a, const Node& b) {
    // Null handling
    if (a.isNull() && b.isNull())  { return Node(false); }
    if (a.isNull() != b.isNull())  { return Node(true); }
    if (a.getType() != b.getType()) { return Node(true); }
    return Node(a.getValue() != b.getValue());
}



SharedPtr<NodeBase> AnyNode::applyAdd(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs + rhs;
}

SharedPtr<NodeBase> AnyNode::applySub(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs - rhs;
}

SharedPtr<NodeBase> AnyNode::applyMul(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs * rhs;
}

SharedPtr<NodeBase> AnyNode::applyDiv(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs / rhs;
}

SharedPtr<NodeBase> AnyNode::applyMod(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs % rhs;
}



void AnyNode::applyAddMut(NodeBase& lhs, const NodeBase& rhs) {
    lhs.setValue((lhs + rhs)->getValue());
}

void AnyNode::applySubMut(NodeBase& lhs, const NodeBase& rhs) {
    lhs.setValue((lhs - rhs)->getValue());
}

void AnyNode::applyMulMut(NodeBase& lhs, const NodeBase& rhs) {
    lhs.setValue((lhs * rhs)->getValue());
}

void AnyNode::applyDivMut(NodeBase& lhs, const NodeBase& rhs) {
    lhs.setValue((lhs / rhs)->getValue());
}

SharedPtr<NodeBase> AnyNode::applyEq(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs == rhs;
}

SharedPtr<NodeBase> AnyNode::applyNeq(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs != rhs;
}

SharedPtr<NodeBase> AnyNode::applyLt(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs < rhs;
}
SharedPtr<NodeBase> AnyNode::applyGt(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs > rhs;
}
SharedPtr<NodeBase> AnyNode::applyLe(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs <= rhs;
}
SharedPtr<NodeBase> AnyNode::applyGe(const NodeBase& lhs, const NodeBase& rhs) {
    return lhs >= rhs;
}

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
SharedPtr<NodeBase> AnyNode::operator+(const NodeBase& other) const {
    if (isNumeric() && other.isNumeric()) {
        return makeShared<AnyNode>(toInt() + other.toInt());
    }

    if (isString() && other.isString()) {
        return makeShared<AnyNode>(toString() + other.toString());
    } 
    throw MerkError("Cannot Add non String non Number types");     
}

SharedPtr<NodeBase> AnyNode::operator-(const NodeBase& other) const {
    if (isNumeric() && other.isNumeric()) {
        return makeShared<AnyNode>(toInt() - other.toInt());
    }

    if (isString() && other.isString()) {
        return makeShared<AnyNode>(Node(toString()) - Node(other.toString()));
    } 
    throw MerkError("Cannot Add non String non Number types");
}
SharedPtr<NodeBase> AnyNode::operator*(const NodeBase& other) const {
    if (isNumeric() && other.isNumeric()) {
        return makeShared<AnyNode>(toInt() * other.toInt());
    }

    if (isString() && other.isString()) {
        return makeShared<AnyNode>(Node(toString()) * Node(other.toString()));
    }

    if (isString() && other.isNumeric()) {
        return makeShared<AnyNode>(Node(toString()) * Node::fromVariant(other.getValue()));
    }
    throw MerkError("Cannot Add non String non Number types"); 
}
SharedPtr<NodeBase> AnyNode::operator/(const NodeBase& other) const {
    if (!isNumeric()) {throw MerkError("Cannot Divide " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");}
    return makeShared<AnyNode>(toInt() / other.toInt());
}


SharedPtr<NodeBase> AnyNode::operator%(const NodeBase& other) const {
    if (!isNumeric() || !other.isNumeric()) {throw MerkError("Attempted Modulus With " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");}
    return makeShared<AnyNode>(toInt() % other.toInt());
}

// Mutating Operations
SharedPtr<NodeBase> AnyNode::operator+=(const NodeBase& other) {
    if ((isNumeric() && other.isNumeric()) || (isString() && other.isString())) {
        return makeShared<AnyNode>(Node::fromVariant(value) + Node::fromVariant(other.getValue())); 
    }
    throw MerkError("Cannot Add " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");
}
SharedPtr<NodeBase> AnyNode::operator-=(const NodeBase& other) {
    if ((isNumeric() && other.isNumeric()) || (isString() && other.isString())) {
        return makeShared<AnyNode>(Node::fromVariant(value) - Node::fromVariant(other.getValue())); 
    }
    throw MerkError("Cannot Subtract " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");
}
SharedPtr<NodeBase> AnyNode::operator*=(const NodeBase& other) {
    if ((isNumeric() && other.isNumeric()) || (isString() && other.isString())) {
        return makeShared<AnyNode>(Node::fromVariant(value) * Node::fromVariant(other.getValue())); 
    }
    throw MerkError("Cannot Multiply " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");
}
SharedPtr<NodeBase> AnyNode::operator/=(const NodeBase& other) {
    if ((isNumeric() && other.isNumeric()) || (isString() && other.isString())) {
        return makeShared<AnyNode>(Node::fromVariant(value) / Node::fromVariant(other.getValue())); 
    }
    throw MerkError("Cannot Subtract " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");
}

// Logic Operations
SharedPtr<NodeBase> AnyNode::operator==(const NodeBase& other) const {
    return makeShared<BoolNode>(getValue() == other.getValue()); 
}
SharedPtr<NodeBase> AnyNode::operator!=(const NodeBase& other) const {
    return makeShared<BoolNode>(getValue() != other.getValue()); 
}
SharedPtr<NodeBase> AnyNode::operator<(const NodeBase& other) const {
    return makeShared<AnyNode>(toInt() < other.toInt());
}
SharedPtr<NodeBase> AnyNode::operator>(const NodeBase& other) const {
    return makeShared<AnyNode>(toInt() > other.toInt());
}
SharedPtr<NodeBase> AnyNode::operator<=(const NodeBase& other) const {
    return makeShared<AnyNode>(toInt() <= other.toInt());
}
SharedPtr<NodeBase> AnyNode::operator>=(const NodeBase& other) const {
    return makeShared<AnyNode>(toInt() >= other.toInt());
}














// Calculation Operators
SharedPtr<NodeBase> NullNode::operator+(const NodeBase& other) const {
    if (isNumeric() && other.isNumeric()) {
        return makeShared<AnyNode>(toInt() + other.toInt());
    }

    if (isString() && other.isString()) {
        return makeShared<AnyNode>(toString() + other.toString());
    } 
    throw MerkError("Cannot Add non String non Number types");     
}

SharedPtr<NodeBase> NullNode::operator-(const NodeBase& other) const {
    if (isNumeric() && other.isNumeric()) {
        return makeShared<AnyNode>(toInt() - other.toInt());
    }

    if (isString() && other.isString()) {
        return makeShared<AnyNode>(Node(toString()) - Node(other.toString()));
    } 
    throw MerkError("Cannot Add non String non Number types");
}
SharedPtr<NodeBase> NullNode::operator*(const NodeBase& other) const {
    if (isNumeric() && other.isNumeric()) {
        return makeShared<AnyNode>(toInt() * other.toInt());
    }

    if (isString() && other.isString()) {
        return makeShared<AnyNode>(Node(toString()) * Node(other.toString()));
    }

    if (isString() && other.isNumeric()) {
        return makeShared<AnyNode>(Node(toString()) * Node::fromVariant(other.getValue()));
    }
    throw MerkError("Cannot Add non String non Number types"); 
}
SharedPtr<NodeBase> NullNode::operator/(const NodeBase& other) const {
    if (!isNumeric()) {throw MerkError("Cannot Divide " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");}
    return makeShared<AnyNode>(toInt() / other.toInt());
}


SharedPtr<NodeBase> NullNode::operator%(const NodeBase& other) const {
    if (!isNumeric() || !other.isNumeric()) {throw MerkError("Attempted Modulus With " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");}
    return makeShared<AnyNode>(toInt() % other.toInt());
}

// Mutating Operations
SharedPtr<NodeBase> NullNode::operator+=(const NodeBase& other) {
    if ((isNumeric() && other.isNumeric()) || (isString() && other.isString())) {
        return makeShared<AnyNode>(Node(value) + Node::fromVariant(other.getValue())); 
    }
    throw MerkError("Cannot Add " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");
}
SharedPtr<NodeBase> NullNode::operator-=(const NodeBase& other) {
    if ((isNumeric() && other.isNumeric()) || (isString() && other.isString())) {
        return makeShared<AnyNode>(Node(value) - Node::fromVariant(other.getValue())); 
    }
    throw MerkError("Cannot Subtract " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");
}
SharedPtr<NodeBase> NullNode::operator*=(const NodeBase& other) {
    if ((isNumeric() && other.isNumeric()) || (isString() && other.isString())) {
        return makeShared<AnyNode>(Node(value) * Node::fromVariant(other.getValue())); 
    }
    throw MerkError("Cannot Multiply " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");
}
SharedPtr<NodeBase> NullNode::operator/=(const NodeBase& other) {
    if ((isNumeric() && other.isNumeric()) || (isString() && other.isString())) {
        return makeShared<AnyNode>(Node(value) / Node::fromVariant(other.getValue())); 
    }
    throw MerkError("Cannot Subtract " + getNodeTypeAsString() + " And " + other.getNodeTypeAsString() + "Types");
}

// Logic Operations
SharedPtr<NodeBase> NullNode::operator==(const NodeBase& other) const {
    throw MerkError("Attempted == on NullNode::operator== -> " + nodeTypeToString(TypeEvaluator::getTypeFromValue(value)) + " Other " + nodeTypeToString(TypeEvaluator::getTypeFromValue(other.getValue())));
}
SharedPtr<NodeBase> NullNode::operator!=(const NodeBase& other) const {
    return makeShared<AnyNode>(toInt() != other.toInt());
}
SharedPtr<NodeBase> NullNode::operator<(const NodeBase& other) const {
    return makeShared<AnyNode>(toInt() < other.toInt());
}
SharedPtr<NodeBase> NullNode::operator>(const NodeBase& other) const {
    return makeShared<AnyNode>(toInt() > other.toInt());
}
SharedPtr<NodeBase> NullNode::operator<=(const NodeBase& other) const {
    return makeShared<AnyNode>(toInt() <= other.toInt());
}
SharedPtr<NodeBase> NullNode::operator>=(const NodeBase& other) const {
    return makeShared<AnyNode>(toInt() >= other.toInt());
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
    // throw MerkError("Invalid op == for Type " + flags.toString() + " DATA: " + toString() + " Determined as type: " + nodeTypeToString(TypeEvaluator::getTypeFromValue(getValue())) + " OTHER IS " + other.flags.toString() + " DATA: " + other.toString() + " Determined type as" + nodeTypeToString(TypeEvaluator::getTypeFromValue(other.getValue())));
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
    auto val = this->data->getValue();
    if (TypeEvaluator::getTypeFromValue(val) != getType()) {
        throw MerkError("Cannot Convert from type " + nodeTypeToString(getType()) + " to " + data->getNodeTypeAsString());
    }
    return *this;
}
    // return Node((*data) += (*other.data)); }
Node Node::operator-=(const Node& other) const {
    if (!data) {throw MerkError("Cannot perform += on null value");}
    *this->data -= (*other.data);
    auto val = this->data->getValue();
    if (TypeEvaluator::getTypeFromValue(val) != getType()) {
        throw MerkError("Cannot Convert from type " + nodeTypeToString(getType()) + " to " + data->getNodeTypeAsString());
    }
    return *this;
}
Node Node::operator*=(const Node& other) const {
    if (!data) {throw MerkError("Cannot perform += on null value");}
    *this->data *= (*other.data);
    auto val = this->data->getValue();
    if (TypeEvaluator::getTypeFromValue(val) != getType()) {
        throw MerkError("Cannot Convert from type " + nodeTypeToString(getType()) + " to " + data->getNodeTypeAsString());
    }
    return *this;
}
Node Node::operator/=(const Node& other) const {
    if (!data) {throw MerkError("Cannot perform += on null value");}
    *this->data /= (*other.data);
    auto val = this->data->getValue();
    if (TypeEvaluator::getTypeFromValue(val) != getType()) {
        throw MerkError("Cannot Convert from type " + nodeTypeToString(getType()) + " to " + data->getNodeTypeAsString());
    }
    return *this;
}

bool Node::operator==(const Node& other) const {
    return !evalNotEqual(*this, other).toBool(); 
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
    AnyNode::applyAddMut(*this, other);
    return shared_from_this();
}

SharedPtr<NodeBase> BoolNode::operator-=(const NodeBase& other) {
    AnyNode::applySubMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> BoolNode::operator*=(const NodeBase& other) {
    AnyNode::applyMulMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> BoolNode::operator/=(const NodeBase& other) {
    AnyNode::applyDivMut(*this, other);
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
    AnyNode::applyAddMut(*this, other);
    return shared_from_this();
}

SharedPtr<NodeBase> StringNode::operator-=(const NodeBase& other) {
    AnyNode::applySubMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> StringNode::operator*=(const NodeBase& other) {
    AnyNode::applyMulMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> StringNode::operator/=(const NodeBase& other) {
    AnyNode::applyDivMut(*this, other);
    return shared_from_this();
}

// Logic Operations
SharedPtr<NodeBase> StringNode::operator==(const NodeBase& other) const {
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
    AnyNode::applyAddMut(*this, other);
    return shared_from_this();
}

SharedPtr<NodeBase> CharNode::operator-=(const NodeBase& other) {
    AnyNode::applySubMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> CharNode::operator*=(const NodeBase& other) {
    AnyNode::applyMulMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> CharNode::operator/=(const NodeBase& other) {
    AnyNode::applyDivMut(*this, other);
    return shared_from_this();
}

// Logic Operations
SharedPtr<NodeBase> CharNode::operator==(const NodeBase& other) const {
    return makeShared<BoolNode>(value == other.toChar());
}
SharedPtr<NodeBase> CharNode::operator!=(const NodeBase& other) const {
    return makeShared<BoolNode>(value != other.toChar());
}
SharedPtr<NodeBase> CharNode::operator<(const NodeBase& other) const {
    return makeShared<BoolNode>(value < other.toChar());
}
SharedPtr<NodeBase> CharNode::operator>(const NodeBase& other) const {
    return makeShared<BoolNode>(value > other.toChar());
}
SharedPtr<NodeBase> CharNode::operator<=(const NodeBase& other) const {
    return makeShared<BoolNode>(value <= other.toChar());
}
SharedPtr<NodeBase> CharNode::operator>=(const NodeBase& other) const {
    return makeShared<BoolNode>(value >= other.toChar());
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
    AnyNode::applyAddMut(*this, other);
    return shared_from_this();
}

SharedPtr<NodeBase> IntNode::operator-=(const NodeBase& other) {
    AnyNode::applySubMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> IntNode::operator*=(const NodeBase& other) {
    AnyNode::applyMulMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> IntNode::operator/=(const NodeBase& other) {
    AnyNode::applyDivMut(*this, other);
    return shared_from_this();
}

// Logic Operations
SharedPtr<NodeBase> IntNode::operator==(const NodeBase& other) const {
    if (!other.isNumeric()) {
        return makeShared<BoolNode>(false);
    }
    return makeShared<BoolNode>(value == other.toInt());
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
    AnyNode::applyAddMut(*this, other);
    return shared_from_this();
}

SharedPtr<NodeBase> FloatNode::operator-=(const NodeBase& other) {
    AnyNode::applySubMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> FloatNode::operator*=(const NodeBase& other) {
    AnyNode::applyMulMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> FloatNode::operator/=(const NodeBase& other) {
    AnyNode::applyDivMut(*this, other);
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
    AnyNode::applyAddMut(*this, other);
    return shared_from_this();
}

SharedPtr<NodeBase> DoubleNode::operator-=(const NodeBase& other) {
    AnyNode::applySubMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> DoubleNode::operator*=(const NodeBase& other) {
    AnyNode::applyMulMut(*this, other);
    return shared_from_this();
}
SharedPtr<NodeBase> DoubleNode::operator/=(const NodeBase& other) {
    AnyNode::applyDivMut(*this, other);
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
