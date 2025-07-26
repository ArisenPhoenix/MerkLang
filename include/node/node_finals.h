#ifndef VARNODE_H
#define VARNODE_H

#include "core/types.h"
#include "core/node/node_numbers.h"
#include "core/node/node_strings.h"

// This is a considered type system considered for future use, specifically for statically typed variables

class LitNode : public Node {
private:
    std::unique_ptr<Node> wrappedNode;

public:
    bool isStatic;
    bool isisMutable;

    // ===== Constructors =====

    // Constructor with Explicit Type and Value
    LitNode(NodeValueType type, const NodeValue& value, bool staticFlag = false, bool isMutableFlag = false)
        : Node(type), isStatic(staticFlag), isisMutable(isMutableFlag) {
        
        // Create the correct type based on the `NodeValueType`
        switch (type) {
            case NodeValueType::Int:
                wrappedNode = std::make_unique<IntNode>(std::get<int>(value));
                break;
            case NodeValueType::Float:
                wrappedNode = std::make_unique<FloatNode>(std::get<float>(value));
                break;
            case NodeValueType::Double:
                wrappedNode = std::make_unique<DoubleNode>(std::get<double>(value));
                break;
            case NodeValueType::Long:
                wrappedNode = std::make_unique<LongNode>(std::get<long>(value));
                break;
            case NodeValueType::Char:
                wrappedNode = std::make_unique<CharNode>(std::get<char>(value));
                break;
            case NodeValueType::String:
                wrappedNode = std::make_unique<StringNode>(std::get<String>(value));
                break;
            case NodeValueType::Bool:
                wrappedNode = std::make_unique<BoolNode>(std::get<bool>(value));
                break;
            case NodeValueType::Number:
                wrappedNode = std::make_unique<NumberNode>(value);
                break;
            default:
                throw std::runtime_error("Unsupported type for LitNode.");
        }
    }

    // Constructor from another `Node`
    LitNode(const Node& other, bool staticFlag = false, bool isMutableFlag = false)
        : Node(other.getType()), wrappedNode(std::make_unique<DynamicNode>(other)), isStatic(staticFlag), isisMutable(isMutableFlag) {}

    // Copy constructor
    LitNode(const LitNode& other)
        : Node(other.getType()), wrappedNode(std::make_unique<DynamicNode>(*other.wrappedNode)),
          isStatic(other.isStatic), isisMutable(other.isisMutable) {}

    // Move constructor
    LitNode(LitNode&& other) noexcept
        : Node(other.getType()), wrappedNode(std::move(other.wrappedNode)),
          isStatic(other.isStatic), isisMutable(other.isisMutable) {}

    // ===== Getters =====
    NodeValue getValue() const override {
        return wrappedNode->getValue();
    }

    double getNumberValue() const override {
        return wrappedNode->getNumberValue();
    }

    String getStringValue() const override {
        return wrappedNode->getStringValue();
    }

    String toString() const override {
        return "LitNode(" + wrappedNode->toString() + ")";
    }
};

class VarNode : public Node {
private:
    std::unique_ptr<Node> wrappedNode;  // Holds the actual node type

public:
    bool isMutable;
    bool isStatic;
    bool isConst;

    // ===== Constructors =====
    
    // Default constructor (Uninitialized)
    VarNode()
        : Node(NodeValueType::Uninitialized), wrappedNode(std::make_unique<DynamicNode>()), isMutable(true), isStatic(false) {}

    // Constructor with Explicit Type and Value
    VarNode(NodeValueType type, const NodeValue& value, bool mutableFlag = true, bool staticFlag = false, bool isMutableFlag = false)
        : Node(type), isMutable(mutableFlag), isStatic(staticFlag) {
        
        // Create the specific node type based on `type`
        switch (type) {
            case NodeValueType::Int:
                wrappedNode = std::make_unique<IntNode>(std::get<int>(value));
                break;
            case NodeValueType::Float:
                wrappedNode = std::make_unique<FloatNode>(std::get<float>(value));
                break;
            case NodeValueType::Double:
                wrappedNode = std::make_unique<DoubleNode>(std::get<double>(value));
                break;
            case NodeValueType::Long:
                wrappedNode = std::make_unique<LongNode>(std::get<long>(value));
                break;
            case NodeValueType::Char:
                wrappedNode = std::make_unique<CharNode>(std::get<char>(value));
                break;
            case NodeValueType::String:
                wrappedNode = std::make_unique<StringNode>(std::get<String>(value));
                break;
            case NodeValueType::Bool:
                wrappedNode = std::make_unique<BoolNode>(std::get<bool>(value));
                break;
            case NodeValueType::Number:  // Special case for generalized number
                wrappedNode = std::make_unique<NumberNode>(value);
                break;
            default:
                throw std::runtime_error("Unsupported type for VarNode.");
        }
    }

    // Constructor for `DynamicNode`
    VarNode(const DynamicNode& dynamic, bool mutableFlag = true, bool staticFlag = false, bool isMutableFlag = false)
        : Node(dynamic.getType()), wrappedNode(std::make_unique<DynamicNode>(dynamic)), isMutable(mutableFlag), isStatic(staticFlag) {}

    // Constructor for other node types
    VarNode(const NumberNode& other, bool mutableFlag = true, bool staticFlag = false, bool isMutableFlag = false)
        : Node(other.getType()), wrappedNode(std::make_unique<NumberNode>(other)), isMutable(mutableFlag), isStatic(staticFlag) {}
    VarNode(const StringNode& other, bool mutableFlag = true, bool staticFlag = false, bool isMutableFlag = false)
        : Node(other.getType()), wrappedNode(std::make_unique<StringNode>(other)), isMutable(mutableFlag), isStatic(staticFlag) {}
    VarNode(const BoolNode& other, bool mutableFlag = true, bool staticFlag = false, bool isMutableFlag = false)
        : Node(other.getType()), wrappedNode(std::make_unique<BoolNode>(other)), isMutable(mutableFlag), isStatic(staticFlag) {}

    // Copy constructor
    VarNode(const VarNode& other)
        : Node(other.getType()), wrappedNode(std::make_unique<DynamicNode>(*other.wrappedNode)),
          isMutable(other.isMutable), isStatic(other.isStatic) {}

    // Move constructor
    VarNode(VarNode&& other) noexcept
        : Node(other.getType()), wrappedNode(std::move(other.wrappedNode)),
          isMutable(other.isMutable), isStatic(other.isStatic) {}

    // ===== Getters =====
    NodeValue getValue() const override {
        return wrappedNode->getValue();
    }

    double getNumberValue() const override {
        return wrappedNode->getNumberValue();
    }

    String getStringValue() const override {
        return wrappedNode->getStringValue();
    }

    String toString() const override {
        return "VarNode(" + wrappedNode->toString() + ")";
    }

    void setSameTypeValue(const Node& newValue){
        if (wrappedNode->getType() == newValue.getType()) {
            if (wrappedNode->getValue() == newValue.getValue()){
                return;
            }
            

            if (auto* numberNode = dynamic_cast<NumberNode*>(wrappedNode.get())) {
                if (auto* newNumberNode = dynamic_cast<const NumberNode*>(&newValue)) {
                    numberNode->setValue(newNumberNode->getValue());  
                    return;
                }
            }

            else if (auto* stringNode = dynamic_cast<StringNode*>(wrappedNode.get())) {
                if (auto* newStringNode = dynamic_cast<const StringNode*>(&newValue)) {
                    stringNode->setValue(newStringNode->getStringValue());
                    return;
                }
            }

            else if (auto* charNode = dynamic_cast<CharNode*>(wrappedNode.get())) {
                if (auto* newCharNode = dynamic_cast<const CharNode*>(&newValue)) {
                    charNode->setValue(std::get<char>(newCharNode->getValue()));  // Extract char safely
                    return;
                }
            }

            else if (auto* boolNode = dynamic_cast<BoolNode*>(wrappedNode.get())) {
                if (auto* newBoolNode = dynamic_cast<const BoolNode*>(&newValue)) {
                    boolNode->setValue(std::get<bool>(newBoolNode->getValue()));  // Extract bool safely
                    return;
                }
            }


            else if (auto* floatNode = dynamic_cast<FloatNode*>(wrappedNode.get())) {
                if (auto* newFloatNode = dynamic_cast<const FloatNode*>(&newValue)) {
                    floatNode->setValue(std::get<bool>(newFloatNode->getValue()));  // Extract bool safely
                    return;
                }
            }

            else if (auto* longNode = dynamic_cast<FloatNode*>(wrappedNode.get())) {
                if (auto* newLongNode = dynamic_cast<const FloatNode*>(&newValue)) {
                    longNode->setValue(std::get<bool>(newLongNode->getValue()));  // Extract bool safely
                    return;
                }
            }

            else {
                setDifferentTypeValue(newValue);
            }
        }
    }


    void setDifferentTypeValue(const Node& newValue){
        // If the type is different, reassign with a new instance
        switch (newValue.getType()) {
            case NodeValueType::Int:
                wrappedNode = std::make_unique<IntNode>(std::get<int>(newValue.getValue()));
                break;
            case NodeValueType::Float:
                wrappedNode = std::make_unique<FloatNode>(std::get<float>(newValue.getValue()));
                break;
            case NodeValueType::Double:
                wrappedNode = std::make_unique<DoubleNode>(std::get<double>(newValue.getValue()));
                break;
            case NodeValueType::Long:
                wrappedNode = std::make_unique<LongNode>(std::get<long>(newValue.getValue()));
                break;
            case NodeValueType::Char:
                wrappedNode = std::make_unique<CharNode>(std::get<char>(newValue.getValue()));
                break;
            case NodeValueType::String:
                wrappedNode = std::make_unique<StringNode>(std::get<String>(newValue.getValue()));
                break;
            case NodeValueType::Bool:
                wrappedNode = std::make_unique<BoolNode>(std::get<bool>(newValue.getValue()));
                break;
            case NodeValueType::Number:
                wrappedNode = std::make_unique<NumberNode>(newValue.getValue());
                break;
            case NodeValueType::Uninitialized:
            case NodeValueType::Any:
            default:
                throw std::runtime_error("Unsupported or invalid type for VarNode assignment.");
        }
    }

        // ===== Set Value (Respects Static & isMutable Constraints) =====
    void setValue(const Node& newValue) {
        if (!isMutable) {
            throw std::runtime_error("Error: Attempted to reassign an isMutable variable.");
        }

        if (isStatic && wrappedNode->getType() != newValue.getType()) {
            throw std::runtime_error("Error: Cannot change type of a statically declared variable.");
        } else if (isStatic && wrappedNode->getType() == newValue.getType()){
            // If the new value is of the same class type, use its setter method
            setSameTypeValue(newValue);
        }

        setDifferentTypeValue(newValue);
    }

};

class ParamNode : public VarNode {
private:
    NodeValueType expectedType;  // Stores expected type (for static enforcement)

public:
    // ===== Constructors =====

    // Default constructor (Uninitialized, must be assigned before use)
    ParamNode() 
        : VarNode(NodeValueType::Uninitialized, {}, true, false, false), expectedType(NodeValueType::Uninitialized) {}

    // Constructor with explicit type enforcement (for static params)
    ParamNode(NodeValueType type, bool isMutable = true, bool stat = false)
        : VarNode(type, {}, true, stat, isMutable), expectedType(type) {}

    // Constructor with a default value (like function parameters)
    ParamNode(const LitNode& defaultValue, bool isMutable = true, bool stat = false)
        : VarNode(defaultValue.getType(), defaultValue.getValue(), true, stat, isMutable), expectedType(defaultValue.getType()) {}

    // Constructor accepting `VarNode` (allows copying an already assigned parameter)
    ParamNode(const VarNode& other, bool isMutable = true, bool stat = false) 
        : VarNode(other.getType(), other.getValue(), true, stat, isMutable), expectedType(other.getType()) {}

    // Copy constructor (ensures constraints remain consistent)
    ParamNode(const ParamNode& other)
        : VarNode(other), expectedType(other.expectedType) {}

    // ===== Setters =====
    void setValue(const Node& newValue) {
        if (!isMutable) {
            throw std::runtime_error("Error: Attempted to reassign an isMutable parameter.");
        }

        if (isStatic && expectedType != NodeValueType::Uninitialized && expectedType != newValue.getType()) {
            throw std::runtime_error("Error: Cannot change type of a statically declared parameter.");
        }

        VarNode::setValue(newValue);
    }

    // ===== Getters =====
    bool hasDefault() const { return expectedType != NodeValueType::Uninitialized; }
    NodeValueType getExpectedType() const { return expectedType; }

    String toString() const override {
        String result = "ParamNode(" + VarNode::toString();
        if (hasDefault()) result += " [Default Value]";
        if (!isMutable) result += " [isMutable]";
        if (isStatic) result += " [Static]";
        return result + ")";
    }
};




class ParamVector {
private:
    Vector<ParamNode> params;

public:
    // ===== Constructors =====
    
    ParamVector() = default;  // Default constructor

    // Constructor from initializer list
    ParamVector(std::initializer_list<ParamNode> paramList) : params(paramList) {}

    // Copy constructor
    ParamVector(const ParamVector& other) : params(other.params) {}

    // Move constructor
    ParamVector(ParamVector&& other) noexcept : params(std::move(other.params)) {}

    // Copy assignment
    ParamVector& operator=(const ParamVector& other) {
        if (this != &other) params = other.params;
        return *this;
    }

    // Move assignment
    ParamVector& operator=(ParamVector&& other) noexcept {
        if (this != &other) params = std::move(other.params);
        return *this;
    }

    // ===== Adding Parameters =====
    
    void addParam(const ParamNode& param) {
        params.push_back(param);
    }

    void addParams(std::initializer_list<ParamNode> paramList) {
        params.insert(params.end(), paramList);
    }

    // ===== Getters =====
    
    size_t size() const {
        return params.size();
    }

    const ParamNode& getParam(size_t index) const {
        if (index >= params.size()) throw std::out_of_range("ParamVector index out of range.");
        return params[index];
    }

    Vector<ParamNode> getParams() const {
        return params;
    }

    // ===== Argument Validation =====
    
    bool validateArguments(const Vector<Node>& args) const {
        if (args.size() > params.size()) return false;  // Too many arguments
        
        for (size_t i = 0; i < params.size(); ++i) {
            const ParamNode& param = params[i];

            if (i < args.size()) {
                // Validate type for provided argument
                NodeValueType argType = args[i].getType();

                if (param.isStatic && param.getExpectedType() != argType) {
                    return false;  // Static type mismatch
                }
            } else if (!param.hasDefault()) {
                return false;  // Missing required parameter
            }
        }

        return true;  // All checks passed
    }

    // ===== Debugging Output =====
    
    String toString() const {
        String result = "ParamVector: [";
        for (const auto& param : params) {
            result += param.toString() + ", ";
        }
        if (!params.empty()) result.pop_back(), result.pop_back();  // Remove last comma
        return result + "]";
    }
};





#endif // VARNODE_H
