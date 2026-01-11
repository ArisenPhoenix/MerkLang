#include <variant>
#include "core/node/Node.hpp"
#include "core/TypesFWD.hpp"
using StringVariant = std::variant<String, char>;
using NumberVariant = std::variant<int, double, long, float>;


class TextNode: public NodeBase {
    StringVariant value;
};

class NumberNode: public NodeBase {
    NumberVariant value;
};