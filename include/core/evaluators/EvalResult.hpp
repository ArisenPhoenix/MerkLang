#pragma once
#include <cstdint>
#include "core/TypesFWD.hpp"
#include "core/node/Node.hpp"

enum class ControlFlow : uint8_t {
    None = 0,
    Return,
    Break,
    Continue,
    Throw,
};

struct EvalResult {
    ControlFlow flow = ControlFlow::None;
    Node value;                 // meaningful for Return/Throw, otherwise usually Null

    static EvalResult Normal(Node v = Node()) {
        return { ControlFlow::None, std::move(v) };
    }

    static EvalResult Return(Node v) {
        return { ControlFlow::Return, std::move(v) };
    }

    static EvalResult Break() {
        return { ControlFlow::Break, Node() };
    }

    static EvalResult Continue() {
        return { ControlFlow::Continue, Node() };
    }

    static EvalResult Throw(Node err) {
        return { ControlFlow::Throw, std::move(err) };
    }

    bool isControl() const { return flow != ControlFlow::None; }
    bool isReturn()  const { return flow == ControlFlow::Return; }
    bool isBreak()   const { return flow == ControlFlow::Break; }
    bool isContinue()const { return flow == ControlFlow::Continue; }
    bool isThrow()   const { return flow == ControlFlow::Throw; }
};
