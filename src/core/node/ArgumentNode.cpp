#include "core/node/ArgumentNode.hpp"
#include "core/node/ParamNode.hpp"
#include "utilities/debugger.h"
#include "core/errors.h"
 


void ArgumentList::addPositionalArg(const Node& arg) {
    positionalArgs.push_back(arg);
}

void ArgumentList::addNamedArg(const String& key, const Node& arg) {
    namedArgs[key] = arg;
}

std::unordered_map<String, Node> ArgumentList::getNamedArgs() {
    return namedArgs;
}

Node ArgumentList::getArg(size_t i) const {
    if (i >= positionalArgs.size()) {throw RunTimeError("Argument index out of range.");}
    return positionalArgs[i];
}

Node ArgumentList::getNamedArg(const String& name) const {
    auto it = namedArgs.find(name);
    if (it == namedArgs.end()) {throw RunTimeError("Named argument not found: " + name);}
    return it->second;
}

Vector<Node> ArgumentList::getRemainingArgs(size_t start) const {
    if (start >= positionalArgs.size()) return {};
    return Vector<Node>(positionalArgs.begin() + start, positionalArgs.end());
}

size_t ArgumentList::positionalCount() const {
    return positionalArgs.size();
}

NodeList ArgumentList::getPositional() {
    return positionalArgs;
}


NodeList ArgumentList::getPositionalArgs() const {
    return positionalArgs;
}

bool ArgumentList::hasNamedArg(const String& name) const {
    return namedArgs.find(name) != namedArgs.end();
}


BoundArgs ArgumentList::bindToBound(const ParamList& params, bool allowDefaults) const {
    BoundArgs out;

    const bool variadic = !params.empty() && params.back().isVarArgsParameter();
    const size_t fixedCount = variadic ? (params.size() - 1) : params.size();

    // ---- fixed ----
    out.fixed.reserve(fixedCount);

    auto setFlagsFunc = [](Node& arg, ParamNode param) {
        if (!param.flags.name.empty()) {
            arg.setFlags(param.flags);
        }
    };

    for (size_t i = 0; i < fixedCount; ++i) {
        const auto& param = params[i];

        Node arg;
        bool wasDefault = false;


        if (i < positionalArgs.size()) {
            arg = positionalArgs[i];
            setFlagsFunc(arg, param);

        } else if (hasNamedArg(param.getName())) {
            arg = getNamedArg(param.getName());
            setFlagsFunc(arg, param);


        } else if (param.hasDefault() && allowDefaults) {
            arg = Node::fromVariant(param.getDefaultValue());
            setFlagsFunc(arg, param);
            wasDefault = true;
        } else {
            throw RunTimeError("Missing argument for parameter: " + param.getName());
        }

        out.fixed.push_back(BoundArg{arg, param.getName(), wasDefault});
    }

    // ---- varargs tail ----
    out.hasVarargs = variadic;

    if (variadic) {
        if (positionalArgs.size() < fixedCount) {
            throw MerkError("Too few arguments for variadic function.");
        }

        if (positionalArgs.size() > fixedCount) {
            out.varargs.reserve(positionalArgs.size() - fixedCount);
            for (size_t i = fixedCount; i < positionalArgs.size(); ++i) {
                out.varargs.push_back(positionalArgs[i]);
            }
        }

        // NOTE: Named varargs not supported here; same as your current comment.
        // You can later decide whether to permit passing extra named args into varargs.
    }

    return out;
}

Vector<Node> ArgumentList::bindTo(const ParamList& params, bool allowDefaults) const {
    DEBUG_FLOW();

    Vector<Node> boundArgs;
    bool variadic = !params.empty() && params.back().isVarArgsParameter();
    size_t fixedCount = variadic ? params.size() - 1 : params.size();
    if (params.size() > 0 && positionalArgs.size() == 0) {
        throw MerkError("NO ARGS");
    }
    auto setFlagsFunc = [](Node& arg, ParamNode param) {
        if (!param.flags.name.empty()) {
            arg.setFlags(param.flags);
        }
    };
    // Bind fixed parameters
    for (size_t i = 0; i < fixedCount; ++i) {
        const auto& param = params[i];
        if (i < positionalArgs.size()) {
            auto arg = positionalArgs[i];
            if (param.flags.name.empty()) {throw MerkError("Positional Arg " + std::to_string(i) + " Is EMPTY");}
            setFlagsFunc(arg, param);
            // if (arg.getFlags().name.empty()) {throw MerkError("Arg Name Is Empty");}
            boundArgs.push_back(arg);
        } else if (hasNamedArg(param.getName())) {
            boundArgs.push_back(getNamedArg(param.getName()));
        } else if (param.hasDefault() && allowDefaults) {
            boundArgs.push_back(Node::fromVariant(param.getDefaultValue()));
        } else {
            throw RunTimeError("Missing argument for parameter: " + param.getName());
        }
    }

    // Bind varargs (as Vector<Node>)
    if (variadic) {
        Vector<Node> varArgs;
        for (size_t i = fixedCount; i < positionalArgs.size(); ++i) {varArgs.push_back(positionalArgs[i]);}

        // Named varargs not supported for now
        boundArgs.push_back(Node::fromVariant(varArgs));
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




bool ArgumentList::hasNamedArgs() {
    return namedArgs.size();
}
size_t ArgumentList::positionalCount() {
    return positionalArgs.size();
}

size_t ArgumentList::size() {
    return positionalArgs.size();
}

size_t ArgumentList::size() const {
    return positionalArgs.size();
}


Node& ArgumentList::back() {
    if (positionalArgs.empty()) {
        throw MerkError("ArgumentList is empty when calling back");
    }
    return positionalArgs.back();
}
Node ArgumentList::back() const {
    if (positionalArgs.empty()) {
        throw MerkError("ArgumentList is empty when calling back");
    }
    return positionalArgs.back();
}


bool ArgumentList::empty() {
    return positionalArgs.empty() && namedArgs.empty();
}

const Node& ArgumentList::operator[](size_t index) const {
    if (index >= positionalArgs.size()) {
        throw MerkError("Parameter index out of range.");
    }
    return positionalArgs[index];
}

std::size_t ArgumentList::hash() const {
    std::size_t h;
    for (auto& arg : positionalArgs) {
        h += arg.hash();
    }

    for (auto& arg: namedArgs) {
        h += arg.second.hash() + std::hash<std::string>()(arg.first);
    }

    return h;
}






