#include "utilities/debugger.h"
#include "core/callables/argument_node.h"
 


void ArgumentList::addPositionalArg(const Node& arg) {
    positionalArgs.push_back(arg);
}

void ArgumentList::addNamedArg(const String& name, const Node& arg) {
    namedArgs[name] = arg;
}

Node ArgumentList::getArg(size_t i) const {
    if (i >= positionalArgs.size()) {
        throw RunTimeError("Argument index out of range.");
    }
    return positionalArgs[i];
}

Node ArgumentList::getNamedArg(const String& name) const {
    auto it = namedArgs.find(name);
    if (it == namedArgs.end()) {
        throw RunTimeError("Named argument not found: " + name);
    }
    return it->second;
}

Vector<Node> ArgumentList::getRemainingArgs(size_t start) const {
    if (start >= positionalArgs.size()) return {};
    return Vector<Node>(positionalArgs.begin() + start, positionalArgs.end());
}

size_t ArgumentList::positionalCount() const {
    return positionalArgs.size();
}

bool ArgumentList::hasNamedArg(const String& name) const {
    return namedArgs.find(name) != namedArgs.end();
}

Vector<Node> ArgumentList::bindTo(const ParamList& params, bool allowDefaults) const {
    DEBUG_FLOW();

    Vector<Node> boundArgs;
    bool variadic = !params.empty() && params.back().isVarArgsParameter();
    size_t fixedCount = variadic ? params.size() - 1 : params.size();

    // Bind fixed parameters
    for (size_t i = 0; i < fixedCount; ++i) {
        const auto& param = params[i];
        if (i < positionalArgs.size()) {
            boundArgs.push_back(positionalArgs[i]);
        } else if (hasNamedArg(param.getName())) {
            boundArgs.push_back(getNamedArg(param.getName()));
        } else if (param.hasDefault() && allowDefaults) {
            boundArgs.push_back(Node(param.getDefaultValue()));
        } else {
            throw RunTimeError("Missing argument for parameter: " + param.getName());
        }
    }

    // Bind varargs (as Vector<Node>)
    if (variadic) {
        Vector<Node> varArgs;
        for (size_t i = fixedCount; i < positionalArgs.size(); ++i) {
            varArgs.push_back(positionalArgs[i]);
        }

        // Named varargs not supported for now (complex edge case)
        boundArgs.push_back(Node(varArgs));  // Wrap in Node later as a NativeList/etc
    }

    DEBUG_FLOW_EXIT();
    return boundArgs;
}

String ArgumentList::toString() const {
    String result = "Args(Positional: [";
    for (const auto& arg : positionalArgs) {
        result += arg.toString() + ", ";
    }
    result += "], Named: {";
    for (const auto& [key, val] : namedArgs) {
        result += key + ": " + val.toString() + ", ";
    }
    result += "})";
    return result;
}
