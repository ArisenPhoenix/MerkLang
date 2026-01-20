#pragma once

#include <string>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <variant>
#include <optional>
#include <functional>
#include <unordered_map>
#include "core/TypesFWD.hpp"
#include "core/errors.h"

struct TypeNode;
using TypeRef = SharedPtr<const TypeNode>;

// struct DataTypeFlags {
//     bool isConst = false;
//     bool isMutable = true;
//     bool isStatic = false;

//     // Optional runtime tag/hint (keep for now if you want)
//     NodeValueType runtimeTag = NodeValueType::Any;

//     // The actual canonical type (this is what you compare)
//     TypeRef declaredType;  // from annotation (or Any)
//     TypeRef runtimeType;   // inferred from value node (optional cache)

//     bool isCallable = false;
//     bool isInstance = false;
//     String name = "";
//     String key = "";

//     size_t hash() const; // include declaredType hash (or pointer)
// };
using TypeSignatureId = uint32_t;

struct DataTypeFlags {
    bool isConst = false;
    bool isMutable = true;
    bool isStatic = false;
    NodeValueType type = NodeValueType::Any;
    mutable ResolvedType fullType;
    TypeSignatureId declaredSig = kInvalidTypeSignatureId; // canonical declared type
    TypeSignatureId inferredSig = kInvalidTypeSignatureId; // optional: what it became at runtime
    DataTypeFlags& mergeVarMetaFromDecl(const DataTypeFlags& decl);

    // Apply *runtime identity* based on an evaluated value node.
    // This should be used for valueNode.flags, not for VarNode.varFlags.
    DataTypeFlags& applyRuntimeIdentityFromValue(const DataTypeFlags& valueFlags);

    // Optional convenience: only apply the variable name without touching anything else.
    DataTypeFlags& applyDeclName(const String& n, const String& k = "");

    bool isCallable = false;
    bool isInstance = false;
    mutable String name = "";
    mutable String key = "";

    DataTypeFlags();

    DataTypeFlags(bool isConst, bool isMutable, bool isStatic, NodeValueType mainType, ResolvedType type);

    DataTypeFlags(bool isConst, bool isMutable, bool isStatic, ResolvedType type);

    DataTypeFlags(String& thisName, bool isConst, bool isMutable, bool isStatic, ResolvedType type);

    DataTypeFlags declarationMerge(DataTypeFlags);

    String toString() const;

    DataTypeFlags merge(DataTypeFlags other);

    DataTypeFlags merge(const std::unordered_map<String, String>& values);
    size_t hash() const;
    
};

class NodeBase: public std::enable_shared_from_this<NodeBase> {
public:
    mutable DataTypeFlags flags;

    virtual ~NodeBase() = default;
    virtual VariantType getValue() const = 0;
    virtual void setValue(const VariantType& v) = 0;

    virtual NodeValueType getType() const = 0;
    virtual SharedPtr<NodeBase> clone() const = 0;

    // Optional: virtual helpers

    virtual bool isInt() const;
    virtual int toInt() const;

    virtual bool isString() const;
    virtual String toString() const;

    virtual bool isChars() const;
    virtual char* toChars() const;

    virtual bool isBool() const;
    virtual bool toBool() const;

    virtual bool isValid() const;
    virtual bool isNumeric() const;

    virtual bool isNull() const;
    virtual bool isTruthy() const;

    virtual bool isDouble() const;
    virtual double toDouble() const;

    virtual bool isFloat() const;
    virtual float toFloat() const;

    bool isList() const;
    bool isArray() const;
    bool isDict() const;
    bool isSet() const;
    bool isNative() const;

    virtual SharedPtr<NativeNode> toNative() const;
    
    // Calculation Operators
    virtual SharedPtr<NodeBase> operator+(const NodeBase& other) const;
    virtual SharedPtr<NodeBase> operator-(const NodeBase& other) const;
    virtual SharedPtr<NodeBase> operator*(const NodeBase& other) const;
    virtual SharedPtr<NodeBase> operator/(const NodeBase& other) const;
    virtual SharedPtr<NodeBase> operator%(const NodeBase& other) const;

    // Mutating Operations
    virtual SharedPtr<NodeBase> operator+=(const NodeBase& other);
    virtual SharedPtr<NodeBase> operator-=(const NodeBase& other);
    virtual SharedPtr<NodeBase> operator*=(const NodeBase& other);
    virtual SharedPtr<NodeBase> operator/=(const NodeBase& other);

    // Logic Operations
    virtual SharedPtr<NodeBase> operator==(const NodeBase& other) const;
    virtual SharedPtr<NodeBase> operator!=(const NodeBase& other) const;
    virtual SharedPtr<NodeBase> operator<(const NodeBase& other) const;
    virtual SharedPtr<NodeBase> operator>(const NodeBase& other) const;
    virtual SharedPtr<NodeBase> operator<=(const NodeBase& other) const;
    virtual SharedPtr<NodeBase> operator>=(const NodeBase& other) const;
    virtual void clear() = 0;

    NodeValueType getNodeType() const {return flags.type;}
    String getNodeTypeAsString() const {return nodeTypeToString(getNodeType());}

    void setFlags(DataTypeFlags newOnes);
    virtual std::size_t hash() const;
    virtual std::size_t strictHash() const;
    
};




class DataStructure;
class ListNode;
class ArrayNode;
class DictNode;
class SetNode;

class Node {
    SharedPtr<NodeBase> data;   // Polymorphic pointer, containing even instances
public:
    
    Node();
    explicit Node(SharedPtr<NodeBase> base);
    explicit Node(Vector<SharedPtr<CallableSignature>>);
    
    Node(const Node&) = default;
    Node(Node&&) noexcept = default;
    Node& operator=(const Node&);
    Node& operator=(Node&&) noexcept;

    VariantType getValue() const;
    void setValue(const VariantType& v);

    NodeValueType getType() const;
    Node clone() const;

    bool isInt();
    bool isInt() const;
    virtual int toInt() const;

    bool isFloat();
    bool isFloat() const;
    virtual float toFloat() const;

    bool isDouble();
    bool isDouble() const;
    virtual double toDouble() const;

    bool isString();
    bool isString() const;
    virtual String toString() const;

    bool isBool() const;
    virtual bool toBool() const;

    bool isNumeric() const;

    bool isValid() const;
    virtual bool isTruthy() const;

    bool isList() const;
    bool isArray() const;
    bool isDict() const;
    bool isSet() const;
    bool isNull() const;

    virtual bool isChars() const;
    virtual char* toChars() const;

    SharedPtr<ListNode> toList() const;
    SharedPtr<ArrayNode> toArray() const;
    SharedPtr<DictNode> toDict() const;
    SharedPtr<SetNode> toSet() const;

    virtual bool isNative() const;
    virtual bool isNative();

    virtual SharedPtr<NativeNode> toNative();
    virtual SharedPtr<NativeNode> toNative() const;

    virtual bool isInstance();
    virtual bool isInstance() const;
    virtual SharedPtr<ClassInstance> toInstance();
    virtual SharedPtr<ClassInstance> toInstance() const;

    virtual bool isCallable();
    virtual bool isCallable() const;

    virtual SharedPtr<Callable> toCallable();
    virtual SharedPtr<Callable> toCallable() const;

    virtual bool isFunctionNode();
    virtual bool isFunctionNode() const;
    virtual FunctionNode toFunctionNode();
    virtual FunctionNode toFunctionNode() const;

    // Calculating Operations
    Node operator+(const Node& other) const;
    Node operator-(const Node& other) const;
    Node operator*(const Node& other) const;
    Node operator/(const Node& other) const;
    Node operator%(const Node& other) const;

    // Mutating Operations
    Node operator+=(const Node& other) const;
    Node operator-=(const Node& other) const;
    Node operator*=(const Node& other) const;
    Node operator/=(const Node& other) const;
    
    // Logic Operations
    bool operator==(const Node& other) const;
    bool operator!=(const Node& other) const;
    bool operator<(const Node& other) const;
    bool operator>(const Node& other) const;
    bool operator<=(const Node& other) const;
    bool operator>=(const Node& other) const;

    DataTypeFlags& getFlags();
    const DataTypeFlags& getFlags() const;
    
    Node negate() const;
    explicit Node(int v);
    explicit Node(String v);
    explicit Node(bool v);
    explicit Node(float v);
    explicit Node(double v);
    explicit Node(NullType v);
    Node(const char* s);

    Node(SharedPtr<ListNode> v);
    Node(SharedPtr<ArrayNode> v);
    Node(SharedPtr<DictNode> v);
    Node(SharedPtr<SetNode> v);
    static Node fromVariant(VariantType v);
    SharedPtr<NodeBase> getInner();
    SharedPtr<NodeBase> getInner() const;

    void clear();

    NodeValueType getNodeType() const;
    String getTypeAsString() const;

    virtual std::size_t hash() const;
    virtual std::size_t strictHash() const;
    void setFlags(DataTypeFlags);
    
    friend std::ostream& operator<<(std::ostream& os, const Node& node);
    friend String operator+(const String& lhs, const Node& node);
};


namespace std {
    template<>
    struct hash<Node> {
        std::size_t operator()(const Node& n) const noexcept {
            return n.hash();
        }
    };
}



class NullNode : public NodeBase {
    NullType value = Null;
    
public:
    bool instantiatedWithNull = false;
    explicit NullNode();
    explicit NullNode(NullType v);
    explicit NullNode(VariantType v);

    VariantType getValue() const override;
    void setValue(const VariantType& v) override;

    NodeValueType getType() const;
    SharedPtr<NodeBase> clone() const;

    bool isNull() const override;

    int toInt() const override;

    String toString() const override;

    bool toBool() const override;

    bool isValid() const override;

    void clear() override;

    bool isNumeric() const override;


    // Calculation Operators
    SharedPtr<NodeBase> operator+(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator-(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator*(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator/(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator%(const NodeBase& other) const override;

    // Mutating Operations
    SharedPtr<NodeBase> operator+=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator-=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator*=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator/=(const NodeBase& other) override;

    // Logic Operations
    SharedPtr<NodeBase> operator==(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator!=(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator<(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator>(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator<=(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator>=(const NodeBase& other) const override;
};


class IntNode : public NodeBase {
    int value;
public:
    explicit IntNode(int v);
    IntNode(VariantType v);

    VariantType getValue() const override;
    void setValue(const VariantType& v) override;

    NodeValueType getType() const;
    SharedPtr<NodeBase> clone() const;

    bool isInt() const override;
    int toInt() const override;
    double toDouble() const override;
    float toFloat() const override;

    String toString() const override;

    bool isBool() const override;
    bool toBool() const override;
    
    bool isTruthy() const override;

    bool isValid() const override;
    bool isNumeric() const override;
    // Calculation Operators
    SharedPtr<NodeBase> operator+(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator-(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator*(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator/(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator%(const NodeBase& other) const override;

    // Mutating Operations
    SharedPtr<NodeBase> operator+=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator-=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator*=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator/=(const NodeBase& other) override;

    // Logic Operations
    SharedPtr<NodeBase> operator==(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator!=(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator<(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator>(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator<=(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator>=(const NodeBase& other) const override;

    void clear() override;
};


class FloatNode : public NodeBase {
    float value;
public:
    explicit FloatNode(float v);
    FloatNode(VariantType v);

    VariantType getValue() const override;
    void setValue(const VariantType& v) override;

    NodeValueType getType() const;
    SharedPtr<NodeBase> clone() const;

    int toInt() const override;
    double toDouble() const override;
    float toFloat() const override;
    bool isFloat() const override;

    String toString() const override;
    bool toBool() const override;
    bool isTruthy() const override;
    bool isValid() const override;
    bool isNumeric() const override;
    // Calculation Operators
    SharedPtr<NodeBase> operator+(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator-(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator*(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator/(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator%(const NodeBase& other) const override;

    // Mutating Operations
    SharedPtr<NodeBase> operator+=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator-=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator*=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator/=(const NodeBase& other) override;

    // Logic Operations
    SharedPtr<NodeBase> operator==(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator!=(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator<(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator>(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator<=(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator>=(const NodeBase& other) const override;

    void clear() override;
};


class DoubleNode : public NodeBase {
    double value;
public:
    explicit DoubleNode(double v);
    DoubleNode(VariantType v);

    VariantType getValue() const override;
    void setValue(const VariantType& v) override;

    NodeValueType getType() const;
    SharedPtr<NodeBase> clone() const;

    int toInt() const override;
    double toDouble() const override;
    float toFloat() const override;
    bool isDouble() const override;

    String toString() const override;
    bool toBool() const override;
    bool isTruthy() const override;
    bool isValid() const override;
    bool isNumeric() const override;
    // Calculation Operators
    SharedPtr<NodeBase> operator+(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator-(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator*(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator/(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator%(const NodeBase& other) const override;

    // Mutating Operations
    SharedPtr<NodeBase> operator+=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator-=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator*=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator/=(const NodeBase& other) override;

    // Logic Operations
    SharedPtr<NodeBase> operator==(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator!=(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator<(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator>(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator<=(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator>=(const NodeBase& other) const override;

    void clear() override;
};

class StringNode : public NodeBase {
    String value;
public:
    explicit StringNode(String v);

    VariantType getValue() const override;
    void setValue(const VariantType& v) override;

    NodeValueType getType() const;
    SharedPtr<NodeBase> clone() const;

    int toInt() const override;

    bool isString() const override;
    String toString() const override;

    bool isBool() const override;
    bool toBool() const override;

    bool isNumeric() const override;
    bool isValid() const override;
    bool isTruthy() const override;

    // Calculation Operators
    SharedPtr<NodeBase> operator+(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator-(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator*(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator/(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator%(const NodeBase& other) const override;

    // Mutating Operations
    SharedPtr<NodeBase> operator+=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator-=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator*=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator/=(const NodeBase& other) override;

    // Logic Operations
    SharedPtr<NodeBase> operator==(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator!=(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator<(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator>(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator<=(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator>=(const NodeBase& other) const override;
    void clear() override;
};

class CharNode : public NodeBase {
    char* value;
public:
    explicit CharNode(char v);
    CharNode(VariantType v);
    VariantType getValue() const override;
    void setValue(const VariantType& v) override;

    NodeValueType getType() const;
    SharedPtr<NodeBase> clone() const;

    int toInt() const override;

    bool isString() const override;
    String toString() const override;
    bool isChars() const override;
    char* toChars() const override;

    bool isBool() const override;
    bool toBool() const override;

    bool isNumeric() const override;
    bool isValid() const override;
    bool isTruthy() const override;

    // Calculation Operators
    SharedPtr<NodeBase> operator+(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator-(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator*(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator/(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator%(const NodeBase& other) const override;

    // Mutating Operations
    SharedPtr<NodeBase> operator+=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator-=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator*=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator/=(const NodeBase& other) override;

    // Logic Operations
    SharedPtr<NodeBase> operator==(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator!=(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator<(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator>(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator<=(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator>=(const NodeBase& other) const override;
    void clear() override;
};


class BoolNode : public NodeBase {
    bool value;
public:
    explicit BoolNode(bool v);
    BoolNode(VariantType v);
    VariantType getValue() const override;
    void setValue(const VariantType& v) override;

    NodeValueType getType() const;
    SharedPtr<NodeBase> clone() const;

    int toInt() const override;

    String toString() const override;

    bool isInt() const override;
    bool isBool() const override;
    bool toBool() const override;

    bool isNumeric() const override;
    bool isValid() const override;

    bool isTruthy() const override;

    // Calculation Operators
    SharedPtr<NodeBase> operator+(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator-(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator*(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator/(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator%(const NodeBase& other) const override;

    // Mutating Operations
    SharedPtr<NodeBase> operator+=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator-=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator*=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator/=(const NodeBase& other) override;

    // Logic Operations
    SharedPtr<NodeBase> operator==(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator!=(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator<(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator>(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator<=(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator>=(const NodeBase& other) const override;

    void clear() override;
};


enum class CoerceMode : uint8_t { Strict, Permissive };

class DynamicNode: public NodeBase {
    VariantType value;
public:
    static NodeValueType inferTypeFromString (String& valueStr, String& typeStr);
    static NodeValueType getNodeTypeFromString(String& type);
    static std::pair<VariantType, NodeValueType> getCoercedStringAndType(const String& value, String& typeStr);
    static std::pair<VariantType, NodeValueType> validateAndCopy(const VariantType& value, NodeValueType type);
    static NodeValueType getTypeFromValue(const VariantType& value);
    static String forceToString(VariantType value);
    static float forceToFloat(VariantType value);
    static double forceToDouble(VariantType value);
    static char* forceToChar(VariantType value);
    static bool forceToBool(VariantType value);
    static int forceToInt(VariantType value);
    static const char* forceToCString(const VariantType& v, String& scratch);
    
    template <typename T>
    static T parseExact(std::string_view s, const char* what);

    template <typename T>
    static T forceTo(const VariantType& v, CoerceMode mode = CoerceMode::Permissive);

    template <typename T>
    static T forceTo(const Node&, CoerceMode mode = CoerceMode::Permissive);


    static String forceToString(Node& value);
    static float forceToFloat(Node& value);
    static double forceToDouble(Node& value);
    static char* forceToChar(Node& value);
    static bool forceToBool(Node& value);
    static int forceToInt(Node& value);

    static SharedPtr<NodeBase> dispatch(VariantType val, NodeValueType type, bool coerce = false);
    static Node dispatchNode(VariantType val, String typeStr, bool coerce = false);
    static DynamicNode fromVariant(VariantType v);

    static SharedPtr<NodeBase> applyAdd(const NodeBase& lhs, const NodeBase& rhs);
    static SharedPtr<NodeBase> applySub(const NodeBase& lhs, const NodeBase& rhs);
    static SharedPtr<NodeBase> applyMul(const NodeBase& lhs, const NodeBase& rhs);
    static SharedPtr<NodeBase> applyDiv(const NodeBase& lhs, const NodeBase& rhs);
    static SharedPtr<NodeBase> applyMod(const NodeBase& lhs, const NodeBase& rhs);

    // Mutating
    static void applyAddMut(NodeBase& target, const NodeBase& rhs);
    static void applySubMut(NodeBase& target, const NodeBase& rhs);
    static void applyMulMut(NodeBase& target, const NodeBase& rhs);
    static void applyDivMut(NodeBase& target, const NodeBase& rhs);

    // Comparison
    static SharedPtr<NodeBase> applyEq(const NodeBase& lhs, const NodeBase& rhs);
    static SharedPtr<NodeBase> applyNeq(const NodeBase& lhs, const NodeBase& rhs);
    static SharedPtr<NodeBase> applyLt(const NodeBase& lhs, const NodeBase& rhs);
    static SharedPtr<NodeBase> applyGt(const NodeBase& lhs, const NodeBase& rhs);
    static SharedPtr<NodeBase> applyLe(const NodeBase& lhs, const NodeBase& rhs);
    static SharedPtr<NodeBase> applyGe(const NodeBase& lhs, const NodeBase& rhs);
    static void validateMutability(const NodeBase&);
    static void validateMutability(const Node&);


    DynamicNode();
    explicit DynamicNode(const VariantType);
    explicit DynamicNode(SharedPtr<NodeBase>);
    explicit DynamicNode(const NodeBase&);
    explicit DynamicNode(NodeBase&&);
    explicit DynamicNode(Node&);
    explicit DynamicNode(const Node&);
    VariantType getValue() const override;
    void setValue(const VariantType& v) override;

    NodeValueType getType() const;
    SharedPtr<NodeBase> clone() const;

    bool isInt() const override;
    int toInt() const override;

    bool isString() const override;
    String toString() const override;

    bool isBool() const override;
    bool toBool() const override;

    bool isValid() const override;
    bool isNumeric() const override;

    // Calculation Operators
    SharedPtr<NodeBase> operator+(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator-(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator*(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator/(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator%(const NodeBase& other) const override;

    // Mutating Operations
    SharedPtr<NodeBase> operator+=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator-=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator*=(const NodeBase& other) override;
    SharedPtr<NodeBase> operator/=(const NodeBase& other) override;

    // Logic Operations
    SharedPtr<NodeBase> operator==(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator!=(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator<(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator>(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator<=(const NodeBase& other) const override;
    SharedPtr<NodeBase> operator>=(const NodeBase& other) const override;

    void clear() override;
};


class NodeWrapper {
    Node valueNode;

public:
    virtual ~NodeWrapper() = default;
    NodeWrapper();
    NodeWrapper(Node value);
    Node& getValueNode();
    const Node& getValueNode() const;
    const DataTypeFlags& getFlags() const;

    void setFlags(DataTypeFlags newOnes);
    String toString() const;
    int toInt() const;
    bool toBool() const;
    
    virtual void setValue(Node other);
    friend class VarNode;
    friend class LitNode;

    bool isBool() const;
    bool isInt() const;
    bool isString() const;
    bool isNumeric() const;

    bool isList() const;
    bool isArray() const;
    bool isDict() const;
    bool isSet() const;

    bool isTruthy() const;

    virtual bool getIsMutable();
    virtual bool getIsStatic();
    virtual bool getIsConst();

    

    void clear();
    bool isValid();
    bool isValid() const;
};




class VarNode: public NodeWrapper {
    DataTypeFlags varFlags;
    String staticType = "Any";
public:
    VarNode();
    VarNode(Node node, bool isC, bool isMut, ResolvedType t, bool isStatic);
    VarNode(Node defaultValue, bool isConst, bool isStatic, bool isMutable);
    VarNode(VariantType, bool isConst, bool isStatic, bool isMutable);
    VarNode(Node node, DataTypeFlags);

    void setValue(Node other) override;

    DataTypeFlags getVarFlags();

    bool getIsMutable() override;
    bool getIsStatic() override;
    bool getIsConst() override;
    Node& pullValue();
    String varString() const;

    bool isFunctionNode() const;
    FunctionNode toFunctionNode() const;

    bool isList() const;

    VarNode(const VarNode& other);

    VarNode(const Node value);

    // Move Constructor
    VarNode(VarNode&& other) noexcept;

    // Copy Assignment Operator
    VarNode& operator=(const VarNode& other);

    // Move Assignment Operator
    VarNode& operator=(VarNode&& other) noexcept;
    // VarNode& operator=(VarNode other);

    UniquePtr<VarNode> uniqClone();
};
 

class LitNode: public NodeWrapper {
public:
    LitNode();
    LitNode(const String& value, const String& type);
    LitNode(SharedPtr<DataStructure>);
};









