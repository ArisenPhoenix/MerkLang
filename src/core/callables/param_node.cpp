#include "core/types.h"
#include "core/errors.h"
#include "core/node.h"
#include "utilities/debugger.h"
#include "core/callables/param_node.h"
#include "core/callables/argument_node.h"


// All Constructor, not actually used but is an idea still
ParamNode::ParamNode(const String& paramName, const VariantType& defaultValue, NodeValueType expectedType, bool isConst, bool isStatic, bool isMutable)
    : VarNode(defaultValue, isConst, isStatic, isMutable), paramName(paramName), hasDefaultValue(true) {
    DEBUG_FLOW(FlowLevel::MED);
    data.type = expectedType;
    if (expectedType != NodeValueType::Any){
        if (expectedType != getNodeValueType(defaultValue)) {
            DEBUG_LOG(LogLevel::ERROR, "Expected Type: ", nodeTypeToString(expectedType), "Provided Type: ", getNodeValueType(defaultValue));
            throw MerkError("Parameter Expected Type Does Not Match default type");
        }
    }
    validateExpectedType(data.type);
    DEBUG_FLOW_EXIT();
};


// Constructor for a parameter without a default value
ParamNode::ParamNode(const String& paramName, NodeValueType expectedType, bool isConst, bool isStatic)
    : VarNode(UninitializedType(), isConst, true, isStatic),
      paramName(paramName), hasDefaultValue(false) {
        defaultValue = UninitializedType();
        DEBUG_FLOW();
        data.type = expectedType;
        DEBUG_FLOW_EXIT();
}

// Constructor for a parameter with a default value
ParamNode::ParamNode(const String& paramName, const VariantType& defaultValue, bool isConst, bool isStatic)
    : VarNode(defaultValue, isConst, true, isStatic),
      paramName(paramName), defaultValue(defaultValue), hasDefaultValue(true) {
        DEBUG_FLOW();
        // defaultValue = UninitializedType();
        data.type = getNodeValueType(defaultValue);
        validateExpectedType(data.type);
        DEBUG_FLOW_EXIT();
}

// Constructor for a positional argument
ParamNode::ParamNode(NodeValueType expectedType, bool isConst, bool isStatic)
    : VarNode(UninitializedType(), isConst, true, isStatic), hasDefaultValue(false) {
        DEBUG_FLOW();
        data.type = expectedType;
        DEBUG_FLOW_EXIT();
}



void ParamNode::setIsVarArgsParam(bool isVarArgParam) {isVarArgs = isVarArgParam;};

// Set value (VariantType) with type checking
void ParamNode::setValue(VariantType newValue) {
    DEBUG_FLOW();
    if (isConst) {
        throw RunTimeError("Cannot reassign a constant parameter.");
    }

    if (data.type == NodeValueType::Any) {
        data.type = getNodeValueType(newValue);  // Allow dynamic typing
    } else {
        validateExpectedType(getNodeValueType(newValue));
    }

    VarNode::setValue(newValue);
    DEBUG_FLOW_EXIT();
}

// Set value (Node) with type checking
void ParamNode::setValue(Node newValue) {
    DEBUG_FLOW();
    if (data.type == NodeValueType::Uninitialized || data.type == NodeValueType::Any) {
        data.type = newValue.getType();
    } else {
        validateExpectedType(newValue.getType());
    }

    VarNode::setValue(newValue);
    DEBUG_FLOW_EXIT();
}

// Retrieve default value safely
VariantType ParamNode::getDefaultValue() const {
    DEBUG_FLOW();
    if (!hasDefaultValue) {
        throw RunTimeError("No default value set for parameter '" + paramName + "'.");
    }
    DEBUG_FLOW_EXIT();
    return defaultValue;
}

// Validate expected type
void ParamNode::validateExpectedType(NodeValueType expectedType) {
    DEBUG_FLOW();
    DEBUG_LOG(LogLevel::DEBUG, "Validating ParamNode Type -> Expected: ", nodeTypeToString(expectedType),
             " | Actual: ", nodeTypeToString(data.type));

    if (expectedType == NodeValueType::Any || data.type == NodeValueType::Any) {
        return;  // Allow `Any` without type enforcement
    }

    if (expectedType == NodeValueType::Null && data.type == NodeValueType::Null) {
        return;  // Allow explicitly nullable parameters
    }

    if (expectedType != NodeValueType::Uninitialized && expectedType != data.type) {
        throw RunTimeError("Type mismatch for parameter '" + paramName +
                           "': Expected " + nodeTypeToString(expectedType) +
                           ", but got " + nodeTypeToString(data.type) + ".");
    }

    DEBUG_FLOW_EXIT();
}
 
String ParamNode::toString() const {
    return "ParamNode(name: " + paramName + 
           ", Expected Type: " + nodeTypeToString(data.type) + 
           ", Value: " + VarNode::toString() + 
           ", Const: " + (isConst ? "true" : "false") +
           ", Static: " + (isStatic ? "true" : "false") +
           ", Mutable: " + (isMutable ? "true" : "false") +
           ", Default: " + (hasDefaultValue ? VarNode::toString() : "None") +  
           ", Uninitialized: " + (std::holds_alternative<UninitializedType>(defaultValue) ? highlight("true", Colors::bold_blue) : highlight("false", Colors::bold_blue)) + ")";
}


String ParamNode::toShortString() const {
    return paramName + ":" + VarNode::toString();
}

ParamNode ParamNode::copy() const {
    return *this;
}



// Add a parameter to the list
void ParamList::addParameter(const ParamNode& param) {
    DEBUG_FLOW();
    parameters.push_back(param);
    DEBUG_FLOW_EXIT();
}


void ParamList::verifyArguments(Vector<Node> args) {
    DEBUG_FLOW(FlowLevel::MED);

    bool variadic = !parameters.empty() && parameters.back().isVarArgsParameter();
    size_t fixedParamCount = variadic ? parameters.size() - 1 : parameters.size();

    // Check fixed arguments
    for (size_t i = 0; i < fixedParamCount; ++i) {
        if (i >= args.size() && parameters[i].hasDefault()) {
            continue;
        }

        if (i >= args.size()) {
            throw MerkError("Missing argument for parameter: " + parameters[i].getName());
        }

        parameters[i].setValue(args[i]);
    }

    // Handle variadic parameter
    if (variadic) {
        if (args.size() < fixedParamCount) {
            throw MerkError("Too few arguments for variadic function.");
        }

        // Copy variadic arguments to a new vector to store in the param
        Vector<Node> variadicArgs(args.begin() + fixedParamCount, args.end());

        // Instead of assigning a Node, you’ll store it directly in the NativeFunction
        // This just verifies count and leaves interpretation to the function
        parameters.back().setValue(UninitializedType());  // Optional: clear placeholder
    }

    DEBUG_FLOW_EXIT();
}



void ParamList::verifyArguments(ArgumentList& args) {
    DEBUG_FLOW(FlowLevel::MED);

    bool variadic = !parameters.empty() && parameters.back().isVarArgsParameter();
    size_t fixedParamCount = variadic ? parameters.size() - 1 : parameters.size();

    // Check fixed arguments
    for (size_t i = 0; i < fixedParamCount; ++i) {
        const auto& param = parameters[i];

        Node arg;
        if (i < args.positionalCount()) {
            arg = args.getArg(i);
        } else if (args.hasNamedArg(param.getName())) {
            arg = args.getNamedArg(param.getName());
        } else if (param.hasDefault()) {
            arg = Node(param.getDefaultValue());
        } else {
            throw MerkError("Missing argument for parameter: " + param.getName());
        }

        // paramaters[i].setValue(arg.getValue());
        parameters[i].setValue(arg);
        // param.setValueFromVariant(arg.getValue());
    }

    // Handle variadic parameter
    if (variadic) {
        if (args.size() < fixedParamCount) {
            throw MerkError("Too few arguments for variadic function.");
        }

        // Copy variadic arguments to a new vector to store in the param
        Vector<Node> variadicArgs(args.begin() + fixedParamCount, args.end());

        // Instead of assigning a Node, you’ll store it directly in the NativeFunction
        // This just verifies count and leaves interpretation to the function
        parameters.back().setValue(UninitializedType());  // Optional: clear placeholder
    }

    DEBUG_FLOW_EXIT();
}


// Retrieve parameter by index
const ParamNode& ParamList::operator[](size_t index) const {
    if (index >= parameters.size()) {
        throw MerkError("Parameter index out of range.");
    }
    return parameters[index];
}

// Retrieve parameter by name
const ParamNode* ParamList::getByName(const String& name) const {
    for (const auto& param : parameters) {
        if (param.getName() == name) {
            return &param;
        }
    }
    return nullptr;
}

// Debugging: Convert to string
String ParamList::toString() const {
    String output = "ParamList(";
    for (const auto& param : parameters) {
        output += param.toString() + ", ";
    }
    output += ")";
    return output;
}

String ParamList::toShortString() const {
    String output;
    size_t length = parameters.size() - 1;
    for (const auto& param: parameters) {
        output += param.toShortString();
        if (param.getName() != parameters[length].getName()){
            output += ", ";
        }
    }
    output += ")";
    return output;
}

void ParamList::resetToDefaults() {
    for (auto& param : parameters) {
        if (param.hasDefault()) {
            param.setValue(param.getDefaultValue()); // Restore default
        } else {
            param.setValue(UninitializedType()); // Reset to uninitialized
        }
    }
}

Vector<NodeValueType> ParamList::getParameterTypes() {
    Vector<NodeValueType> paramTypes;
    for (auto& param : parameters){
        paramTypes.emplace_back(param.getType());
    }
    return paramTypes;
}

ParamNode& ParamList::back() {return parameters.back();}
ParamNode ParamList::back() const {return parameters.back();}

void ParamList::printAST(std::ostream& os, int indent) const {
    for (const auto& param: parameters){
        printIndent(os, indent);        
        os << param.toString() << std::endl;
    }
}

Vector<String> ParamList::getNames() const {
    Vector<String> paramNames;
    for (auto param : parameters){
        paramNames.emplace_back(param.getName());
    }
    return paramNames;
}


ParamList ParamList::clone() {
    if (parameters.size() == 0) {
        DEBUG_LOG(LogLevel::DEBUG, "ParamList is empty. Nothing to clone.");
        return ParamList();
        // throw MerkError("ParamList::clone() failed: parameters vector is null. (NON CONST)");
    }
    ParamList params;
    for (auto& param : parameters) {
        params.addParameter(param.copy());
    }

    return params;
};

ParamList ParamList::clone() const {
    if (parameters.data() == nullptr) {
        throw MerkError("ParamList::clone() failed: parameters vector is null.");
    }
    ParamList params;
    for (auto& param : parameters) {
        params.addParameter(param.copy());
    }

    return params;
};



void ParamList::erase(size_t index) {
    if (index < parameters.size()) {
        parameters.erase(parameters.begin() + index);
    } else {
        throw MerkError("ParamList::erase: index out of range");
    }
}

bool ParamList::eraseByName(const String& name) {
    auto it = std::find_if(parameters.begin(), parameters.end(),
                           [&](const ParamNode& param) {
                               return param.getName() == name;
                           });
    if (it != parameters.end()) {
        parameters.erase(it);
        return true;
    }
    return false;
}


bool ParamList::empty() const { return parameters.empty(); }

ParamList::~ParamList() {
    parameters.clear();
}