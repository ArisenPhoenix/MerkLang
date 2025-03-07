#ifndef NODE_NUMBERS_H
#define NODE_NUMBERS_H

#include <iostream>
#include <string>
#include <stdexcept>

#include "core/types.h"

// This is a considered type system considered for future use, specifically for statically typed variables



// Use `std::variant` to hold different types
using NodeValue = std::variant<int, float, double, long, char, String, bool>;

class Node {
protected:
    NodeValueType type;  // Stores the type of node

public:
    Node(NodeValueType t = NodeValueType::Uninitialized) : type(t) {}

    virtual ~Node() = default;  // Ensure proper cleanup in derived classes

    // Get the type of the node
    NodeValueType getType() const { return type; }

    // Virtual function to get value (can return different types)
    virtual NodeValue getValue() const = 0;

    // Virtual function for debugging output
    virtual String toString() const = 0;

    // Helper methods for getting numeric or string values
    virtual double getNumberValue() const {
        if (std::holds_alternative<int>(getValue())) return std::get<int>(getValue());
        if (std::holds_alternative<float>(getValue())) return std::get<float>(getValue());
        if (std::holds_alternative<double>(getValue())) return std::get<double>(getValue());
        if (std::holds_alternative<long>(getValue())) return std::get<long>(getValue());
        throw std::runtime_error("Node does not hold a numeric value.");
    }

    virtual String getStringValue() const {
        if (std::holds_alternative<String>(getValue())) return std::get<String>(getValue());
        if (std::holds_alternative<char>(getValue())) return String(1, std::get<char>(getValue()));
        throw std::runtime_error("Node does not hold a string value.");
    }

    // Operator Overloading for Debugging
    friend std::ostream& operator<<(std::ostream& os, const Node& node) {
        os << node.toString();
        return os;
    }

    virtual void setValue() = 0;
};

// ======= IntNode =======
class IntNode : public virtual Node {
private:
    int value;

public:
    IntNode(int val) : Node(NodeValueType::Int), value(val) {}
    IntNode(const IntNode& other) : Node(other.getType()), value(std::get<int>(other.getValue())) {}


    NodeValue getValue() const override { return value; }

    String getStringValue() const override { return std::to_string(value); }

    String toString() const override {
        return "IntNode(" + std::to_string(value) + ")";
    }

    void setValue(int newValue) {value = newValue;}
};

// ======= FloatNode =======
class FloatNode : public virtual Node {
private:
    float value;

public:
    FloatNode(float val) : Node(NodeValueType::Float), value(val) {}
    FloatNode(const FloatNode& other) : Node(other.getType()), value(std::get<float>(other.getValue())) {}


    NodeValue getValue() const override { return value; }

    String getStringValue() const override { return std::to_string(value); }

    String toString() const override {
        return "FloatNode(" + std::to_string(value) + ")";
    }

    void setValue(float newValue) {value = newValue;}
};

// ======= DoubleNode =======
class DoubleNode : public virtual Node {
private:
    double value;

public:
    DoubleNode(double val) : Node(NodeValueType::Double), value(val) {}
    DoubleNode(const DoubleNode& other) : Node(other.getType()), value(std::get<double>(other.getValue())) {}

    NodeValue getValue() const override { return value; }

    String getStringValue() const override { return std::to_string(value); }

    String toString() const override {
        return "DoubleNode(" + std::to_string(value) + ")";
    }
    void setValue(double newValue) {value = newValue;}
};

// ======= LongNode =======
class LongNode : public virtual Node {
private:
    long value;

public:
    LongNode(long val) : Node(NodeValueType::Long), value(val) {}
    LongNode(const LongNode& other) : Node(other.getType()), value(std::get<long>(other.getValue())) {}

    NodeValue getValue() const override { return value; }

    String getStringValue() const override { return std::to_string(value); }

    String toString() const override {
        return "LongNode(" + std::to_string(value) + ")";
    }
};

// ======= NumberNode =======
// Generalized wrapper for any number type
class NumberNode : public virtual Node {
private:
    NodeValue value;

public:
    // Constructors for different number types
    NumberNode(int val) : Node(NodeValueType::Int), value(val) {}
    NumberNode(float val) : Node(NodeValueType::Float), value(val) {}
    NumberNode(double val) : Node(NodeValueType::Double), value(val) {}
    NumberNode(long val) : Node(NodeValueType::Long), value(val) {}
    NumberNode(const NumberNode& other) : Node(other.getType()), value(other.getValue()) {}

    NodeValue getValue() const override { return value; }

    double getNumberValue() const override {
        if (std::holds_alternative<int>(value)) return std::get<int>(value);
        if (std::holds_alternative<float>(value)) return std::get<float>(value);
        if (std::holds_alternative<double>(value)) return std::get<double>(value);
        if (std::holds_alternative<long>(value)) return std::get<long>(value);
        throw std::runtime_error("NumberNode does not hold a numeric value.");
    }

    String getStringValue() const override {
        return std::visit([](const auto& arg) -> String {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, int> ||
                        std::is_same_v<T, float> ||
                        std::is_same_v<T, double> ||
                        std::is_same_v<T, long>) {
                return std::to_string(arg);
            } else {
                throw std::runtime_error("NumberNode does not contain a numeric value.");
            }
        }, value);
    }

    String toString() const override {
        return "NumberNode(" + getStringValue() + ")";
    }

    void setValue(NodeValue newValue) {value = newValue;}
};


// ======= NumberNode =======
class BoolNode : public virtual Node {
private:
    bool value;

public:
    BoolNode(bool val) : Node(NodeValueType::Bool), value(val) {}
    BoolNode(const BoolNode& other) : Node(other.getType()), value(std::get<bool>(other.getValue())) {}
    BoolNode(const NumberNode& other) : Node(NodeValueType::Bool) {
    value = other.getNumberValue() != 0;  // Convert nonzero numbers to `true`, zero to `false`
    }
    // BoolNode(const StringNode& other) : Node(NodeValueType::Bool) {
    //     std::string strVal = other.getStringValue();
    //     if (strVal == "true" || strVal == "1") value = true;
    //     else if (strVal == "false" || strVal == "0") value = false;
    //     else throw std::runtime_error("Invalid string representation for BoolNode: " + strVal);
    // }

    BoolNode(const std::string& strVal) : Node(NodeValueType::Bool) {
        if (strVal == "true" || strVal == "1") value = true;
        else if (strVal == "false" || strVal == "0") value = false;
        else throw std::runtime_error("Invalid string representation for BoolNode: " + strVal);
    }
    BoolNode(int num) : Node(NodeValueType::Bool), value(num != 0) {}
    BoolNode(float num) : Node(NodeValueType::Bool), value(num != 0.0f) {}
    BoolNode(double num) : Node(NodeValueType::Bool), value(num != 0.0) {}
    BoolNode(long num) : Node(NodeValueType::Bool), value(num != 0L) {}
    NodeValue getValue() const override { return value; }

    double getNumberValue() const override {
        return value ? 1.0 : 0.0;
    }

    String getStringValue() const override {
        return value ? "true" : "false";
    }

    String toString() const override {
        return "BoolNode(" + getStringValue() + ")";
    }

    void setValue(bool newValue) {value = newValue;}
};


#endif // NODE_NUMBERS_H
