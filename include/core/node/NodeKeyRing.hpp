// NodeKeying.hpp
#pragma once
#include "core/node/Node.hpp"

using DictStorage = std::unordered_map<Node, Node, NodeKeyHash, NodeKeyEq>;


struct NodeKeyHash {
    size_t operator()(const Node& n) const noexcept {
        return n.valueHash();
    }
};

struct NodeKeyEq {
    bool operator()(const Node& a, const Node& b) const noexcept {
        return a.valueEquals(b);
    }
};