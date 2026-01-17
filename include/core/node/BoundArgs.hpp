#pragma once

#include "core/node/Node.hpp"

struct BoundArg {
    Node node;         // actual bound value
    String name;       // param name (empty if positional-only param)
    bool wasDefault = false;
};

struct BoundArgs {
    Vector<BoundArg> fixed;   // one per fixed param, in param order
    Vector<Node> varargs;     
    bool hasVarargs = false;

    // Flattened view for existing call code (if needed)
    Vector<Node> flatten() const;
};