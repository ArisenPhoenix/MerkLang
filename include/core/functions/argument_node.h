#ifndef ARGUMENT_LIST_H
#define ARGUMENT_LIST_H

#include "core/types.h"
#include "core/node.h"
#include "core/functions/param_node.h"



class ParamList;

class ArgumentList {
private:
    Vector<Node> positionalArgs;
    std::unordered_map<String, Node> namedArgs;

public:
    ArgumentList() = default;

    void addPositionalArg(const Node& arg);
    void addNamedArg(const String& name, const Node& arg);

    Node getArg(size_t i) const;
    Node getNamedArg(const String& name) const;
    Vector<Node> getRemainingArgs(size_t start) const;

    size_t positionalCount() const;
    bool hasNamedArg(const String& name) const;

    Vector<Node> bindTo(const ParamList& params, bool allowDefaults = false) const;

    String toString() const;
};


#endif // ARGUMENT_LIST_H