#ifndef STRING_NODE_H
#define STRING_NODE_H

#include <iostream>
#include <string>

#include "core/types.h"
#include "core/node/node_numbers.h"

// This is a considered type system considered for future use, specifically for statically typed variables



// ======= CharNode =======
// Stores a single character
class CharNode : public virtual Node {
private:
    char value;

public:
    CharNode(char val) : Node(NodeValueType::Char), value(val) {}
    CharNode(const CharNode& other) : Node(other.getType()), value(std::get<char>(other.getValue())) {}

    NodeValue getValue() const override { return value;}

    String getStringValue() const { return String(1, value); }

    String toString() const override {
        return "CharNode('" + getStringValue() + "')";
    }
    
    void setValue(char newValue) {value = newValue;}

};

// ======= SimpleStringNode =======
// Stores a basic string
class SimpleStringNode : public virtual Node {
private:
    String value;

public:
    SimpleStringNode(const String& val) : Node(NodeValueType::String), value(val) {}

    NodeValue getValue() const override { return value; }

    String getStringValue() const { return value; }

    String toString() const override {
        return "SimpleStringNode(\"" + value + "\")";
    }
    void setValue(String newValue) {value = newValue;}
};



using StringValue = std::variant<char, String>; 

// ======= StringNode =======
// Bridges CharNode and SimpleStringNode
class StringNode : public CharNode, public SimpleStringNode {
private:
    StringValue value;

public:
    // Construct from a character
    StringNode(char val) : Node(NodeValueType::String), CharNode(val), SimpleStringNode(String(1, val)) {}

    // Construct from a string
    StringNode(const String& val) : Node(NodeValueType::String), CharNode(val[0]), SimpleStringNode(val) {}

    // Copy constructor
    StringNode(const StringNode& other) : Node(other), CharNode(other), SimpleStringNode(other) {}

    // Ensure getValue() always returns a full string
    NodeValue getValue() const override {
        return SimpleStringNode::getValue();
    }

    // Explicitly define getStringValue() for clarity
    String getStringValue() const {
        return SimpleStringNode::getStringValue();
    }

    String toString() const override {
        return "StringNode(\"" + getStringValue() + "\")";
    }

    void setValue(StringValue newValue) {value = newValue;}
};



// ======= DynamicNode =======
// Can store numbers, strings, or booleans dynamically
class DynamicNode : public NumberNode, public StringNode, public BoolNode {
private:
    NodeValue value;

public:
    // Constructors for different types
    DynamicNode(int val) : NumberNode(val), StringNode(""), BoolNode(false), value(val) {}
    DynamicNode(float val) : NumberNode(val), StringNode(""), BoolNode(false), value(val) {}
    DynamicNode(double val) : NumberNode(val), StringNode(""), BoolNode(false), value(val) {}
    DynamicNode(long val) : NumberNode(val), StringNode(""), BoolNode(false), value(val) {}

    DynamicNode(char val) : NumberNode(0), StringNode(val), BoolNode(false), value(val) {}
    DynamicNode(const String& val) : NumberNode(0), StringNode(val), BoolNode(false), value(val) {}

    DynamicNode(bool val) : NumberNode(0), StringNode(val ? "true" : "false"), BoolNode(val), value(val) {}


    DynamicNode(const DynamicNode& other) : NumberNode(0), StringNode(""), BoolNode(false), value(other.getValue()) {}
    DynamicNode(const NumberNode& other) : NumberNode(other), StringNode(""), BoolNode(false), value(other.getValue()) {}
    DynamicNode(const StringNode& other) : NumberNode(0), StringNode(other), BoolNode(false), value(other.getValue()) {}
    DynamicNode(const BoolNode& other) : NumberNode(0), StringNode(other.getStringValue()), BoolNode(other), value(other.getValue()) {}


    // Returns the stored value as a variant
    NodeValue getValue() const override { return value; }

    // Retrieve as a number (throws error if not numeric)
    double getNumberValue() const override {
        if (std::holds_alternative<int>(value)) return std::get<int>(value);
        if (std::holds_alternative<float>(value)) return std::get<float>(value);
        if (std::holds_alternative<double>(value)) return std::get<double>(value);
        if (std::holds_alternative<long>(value)) return std::get<long>(value);
        if (std::holds_alternative<bool>(value)) return std::get<bool>(value) ? 1.0 : 0.0;
        throw std::runtime_error("DynamicNode does not hold a numeric value.");
    }

    // Retrieve as a string (throws error if not a string)
    String getStringValue() const override {
        if (std::holds_alternative<String>(value)) return std::get<String>(value);
        if (std::holds_alternative<char>(value)) return String(1, std::get<char>(value));
        if (std::holds_alternative<bool>(value)) return std::get<bool>(value) ? "true" : "false";
        throw std::runtime_error("DynamicNode does not hold a string value.");
    }

    // Convert to string for debugging
    String toString() const override {
        if (std::holds_alternative<int>(value)) return "DynamicNode(Int: " + std::to_string(std::get<int>(value)) + ")";
        if (std::holds_alternative<float>(value)) return "DynamicNode(Float: " + std::to_string(std::get<float>(value)) + ")";
        if (std::holds_alternative<double>(value)) return "DynamicNode(Double: " + std::to_string(std::get<double>(value)) + ")";
        if (std::holds_alternative<long>(value)) return "DynamicNode(Long: " + std::to_string(std::get<long>(value)) + ")";
        if (std::holds_alternative<char>(value)) return "DynamicNode(Char: '" + String(1, std::get<char>(value)) + "')";
        if (std::holds_alternative<String>(value)) return "DynamicNode(String: \"" + std::get<String>(value) + "\")";
        if (std::holds_alternative<bool>(value)) return "DynamicNode(Bool: " + getStringValue() + ")";
        return "DynamicNode(Uninitialized)";
    }


    void setValue(NodeValue newValue) {value = newValue;}
};











#endif // STRING_NODE_H
