#ifndef ARGUMENT_LIST_H
#define ARGUMENT_LIST_H

#include "core/types.h"
#include "core/node.h"
#include "core/callables/param_node.h"



class ParamList;

class ArgumentList: public Node {
private:
    NodeList positionalArgs;
    std::unordered_map<String, Node> namedArgs;

public:
    ArgumentList() = default;

    void addPositionalArg(const Node& arg);
    void addNamedArg(const String& name, const Node& arg);

    Node getArg(size_t i) const;
    Node getNamedArg(const String& name) const;
    NodeList getRemainingArgs(size_t start) const;
    NodeList getPositional();

    size_t positionalCount() const;
    bool hasNamedArg(const String& name) const;

    NodeList bindTo(const ParamList& params, bool allowDefaults = false) const;

    String toString() const;

    bool hasNamedArgs(String name);
    size_t positionalCount();
    auto begin() { return positionalArgs.begin(); }
    auto end() { return positionalArgs.end(); }
    auto begin() const { return positionalArgs.begin(); }
    auto end() const { return positionalArgs.end(); }

    // Add cbegin() and cend() for const iteration, c++ can be a pain
    auto cbegin() const { return positionalArgs.cbegin(); }
    auto cend() const { return positionalArgs.cend(); }

    Node& back();
    Node back() const;

    // Access parameters by index
    const Node& operator[](size_t index) const;
    bool empty();
    size_t size();
};



#endif // ARGUMENT_LIST_H