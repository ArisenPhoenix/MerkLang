#include "core/node/Node.hpp"
#include "core/callables/classes/ClassBase.hpp"
#include "core/callables/functions/Function.hpp"
#include "core/callables/Callable.hpp"
#include "core/node/Node.hpp"
#include "core/node/NodeStructures.hpp"
#include "core/types.h"
#include "core/Scope.hpp"
#include <charconv>
#include "core/evaluators/TypeEvaluator.hpp"


template <typename T>
T get(VariantType v) { return std::get<T>(v); }


SharedPtr<NodeBase> AnyNode::dispatch(VariantType val, NodeValueType type, bool coerce) {
    (void)coerce;
    DEBUG_FLOW(FlowLevel::MED);

    if (type == NodeValueType::UNKNOWN) {
        if (std::holds_alternative<String>(val)) {
            auto actualVal = std::get<String>(val);
            if (actualVal == "Variable" || actualVal == "Argument" || actualVal == "FunctionCall" || actualVal == "ClassMethodCall" || actualVal == "Parameter") {
                return makeShared<StringNode>(actualVal);
            } else {throw MerkError("Type Unknwon Is Not Of the Exceptions");}
        } else {throw MerkError("Cannot Handle Type UNKNWON, valType: " + nodeTypeToString(TypeEvaluator::getTypeFromValue(val)));}
    }
    
    if (type == NodeValueType::Null) {
        return makeShared<NullNode>();
    }

    auto valType = TypeEvaluator::getTypeFromValue(val);
    auto passedIsANumber = type == NodeValueType::Int || type == NodeValueType::Number || type == NodeValueType::Long || type == NodeValueType::Double || type == NodeValueType::Float;
    if (valType == type || (valType == NodeValueType::Number && passedIsANumber) || type == NodeValueType::ClassInstance) {
        switch (type)
        {
        case NodeValueType::Number:
            {
                switch (valType)
                {
                case NodeValueType::Int:
                    DEBUG_FLOW_EXIT();
                    return makeShared<IntNode>(std::get<int>(val));
                
                default:
                    throw MerkError("Cannot Cast " + nodeTypeToString(valType) + " to " + nodeTypeToString(type));
                }
            }
        case NodeValueType::Int:
            DEBUG_FLOW_EXIT();
            return makeShared<IntNode>(std::get<int>(val));
        case NodeValueType::String:
            DEBUG_FLOW_EXIT();
            if (!std::holds_alternative<String>(val)) {
                return makeShared<StringNode>(TypeEvaluator::as<String>(val));
            }
            return makeShared<StringNode>(std::get<String>(val));

        case NodeValueType::Bool:
            DEBUG_FLOW_EXIT();
            return makeShared<BoolNode>(std::get<bool>(val));
        case NodeValueType::Double:
            DEBUG_FLOW_EXIT();
            return makeShared<DoubleNode>(std::get<double>(val));
        case NodeValueType::Float:
            DEBUG_FLOW_EXIT();
            return makeShared<FloatNode>(std::get<float>(val));
        case NodeValueType::DataStructure:
        case NodeValueType::Dict:
            throw MerkError("AnyNode::dispatch found Dict | DataStructure");
        case NodeValueType::Callable:
            {
                auto call = std::get<SharedPtr<Callable>>(val);
                throw MerkError("Tried to create Callable " + call->toString());
            }
        default:
            DEBUG_FLOW_EXIT();
            throw MerkError("Not A Currently Supported Type: " + nodeTypeToString(type));
        }
    }
    DEBUG_FLOW_EXIT();
    throw MerkError("Not A supported Type " + nodeTypeToString(valType) + ". Tried to dispatch to: " + nodeTypeToString(type));
}

Node AnyNode::dispatchNode(VariantType value, String typeStr, bool coerce) {
    DEBUG_FLOW(FlowLevel::MED);

    auto nodeType = TypeEvaluator::getNodeTypeFromString(typeStr);
    if (nodeType == NodeValueType::UNKNOWN) {throw MerkError("The Type of " + nodeTypeToString(nodeType, false) + " for Starting Lit Value Is: " + TypeEvaluator::to<String>(value));}
    auto nodeBase = AnyNode::dispatch(value, nodeType, coerce);
    auto node = Node(nodeBase);
    DEBUG_FLOW_EXIT();
    return node;
}

VariantType AnyNode::getValue() const { return value; }

void AnyNode::setValue(const VariantType& v)  {
    DEBUG_FLOW(FlowLevel::PERMISSIVE);
    auto type = TypeEvaluator::getTypeFromValue(v);
    value = v;
    flags.type = type;
    flags.fullType.setBaseType(nodeTypeToString(type, false));
    DEBUG_FLOW_EXIT();
}

NodeValueType AnyNode::getType() const { return NodeValueType::Any; }

void AnyNode::validateMutability(const NodeBase& value) {
    if (!value.flags.isMutable) {
        throw MerkError("Cannot mutate immutable value");
    }
}

void AnyNode::validateMutability(const Node& value) {
    if (!value.getFlags().isMutable) {
        throw MerkError("Cannot mutate immutable value");
    }
}

SharedPtr<NodeBase> AnyNode::clone() const {
    // auto val = getValue();
    // if (std::holds_alternative<SharedPtr<Callable>>(val)) {
    //     auto callable = std::get<SharedPtr<Callable>>(val);
    //     auto node = this->getValue();
    //     switch (TypeEvaluator::getTypeFromValue(callable)) {
    //         case NodeValueType::ClassInstance:
    //             return std::static_pointer_cast<ClassInstance>(callable)->clone();
    //         default:
    //             return makeShared<DynamicNode>(*this);
    //     }
    // } else if (std::holds_alternative<SharedPtr<NativeNode>>(val)) {
    //     throw MerkError("Cloning a DataStructure");
    // } else {
    //     auto node = makeShared<DynamicNode>(*this); 
    //     node->flags = flags;
    //     return node;
    // }

    auto node = makeShared<AnyNode>(*this); 
    node->flags = flags;
    return node;

    throw MerkError("Calling Clone on DynamicNode holding " + nodeTypeToString(TypeEvaluator::getTypeFromValue(getValue())));
    
}
 
int AnyNode::toInt() const {  
    if (isInt()) {
        return std::get<int>(value);
    } else if (isBool()) {
        return toBool();
    } else if (isNumeric()) {
        return TypeEvaluator::to<int>(value);
    }

    throw MerkError("Cannnot cast " + nodeTypeToString(getType()) + " to " + "Int");
}
bool AnyNode::isString() const {return std::holds_alternative<String>(value);}
bool AnyNode::isBool() const {return std::holds_alternative<bool>(value);}
bool AnyNode::isValid() const { return TypeEvaluator::getTypeFromValue(value) == getNodeType(); }
String AnyNode::toString() const {
    switch (TypeEvaluator::getTypeFromValue(value))
    {
    case NodeValueType::ClassInstance:
    case NodeValueType::Callable:
        return std::static_pointer_cast<ClassInstance>(std::get<SharedPtr<Callable>>(value))->toString();    
    default:
        return TypeEvaluator::to<String>(value); 
    }
    
}
bool AnyNode::toBool() const { 
    NodeValueType ofThis = TypeEvaluator::getTypeFromValue(value);
    auto node = AnyNode::dispatchNode(value, nodeTypeToString(ofThis, false));
    return node.toBool();
}
bool AnyNode::isNumeric() const { return isInt() || isFloat() || isDouble();}
bool AnyNode::isInt() const {return std::holds_alternative<int>(value);}
