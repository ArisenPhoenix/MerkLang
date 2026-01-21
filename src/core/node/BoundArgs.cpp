#include "core/node/BoundArgs.hpp"
// Flattened view for existing call code (if needed)
Vector<Node> BoundArgs::flatten() const {
    Vector<Node> out;
    out.reserve(fixed.size() + (hasVarargs ? 1 : 0));
    for (auto& b : fixed) out.push_back(b.node);
    if (hasVarargs) {
        out.push_back(Node::fromVariant(varargs));
    }
    return out;
}
