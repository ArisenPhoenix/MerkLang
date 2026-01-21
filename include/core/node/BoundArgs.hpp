#pragma once

#include "core/node/Node.hpp"

struct BoundArg {
    Node node;     
    String name;      
    bool wasDefault = false;
};

struct BoundArgs {
    Vector<BoundArg> fixed;  
    Vector<Node> varargs;     
    bool hasVarargs = false;
    Vector<Node> flatten() const;
};