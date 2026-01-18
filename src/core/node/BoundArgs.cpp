#include "core/node/BoundArgs.hpp"
// Flattened view for existing call code (if needed)
Vector<Node> BoundArgs::flatten() const {
    Vector<Node> out;
    out.reserve(fixed.size() + (hasVarargs ? 1 : 0));
    for (auto& b : fixed) out.push_back(b.node);

    // Represent varargs as a single trailing argument containing a Vector<Node>,
    // matching ParamList::bindArguments(ArgumentList&) and ArgumentList::bindToBound.
    if (hasVarargs) {
        out.push_back(Node::fromVariant(varargs));
    }
    return out;
}
