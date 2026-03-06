#pragma once

#include "core/TypesFWD.hpp"
#include "core/evaluators/EvalResult.hpp"

class CodeBlock;

namespace FastIR {

enum class OpCode : uint8_t {
    PushConstInt,
    LoadVarInt,
    StoreVarIntDecl,
    StoreVarInt,
    AddInt,
    SubInt,
    MulInt,
    DivInt,
    ModInt,
    CmpLtInt,
    CmpLeInt,
    CmpGtInt,
    CmpGeInt,
    CmpEqInt,
    CmpNeInt,
    NegInt,
    NotBool,
    Jump,
    JumpIfFalse,
};
 
struct Instr {
    OpCode op{};
    int arg = 0;
};

struct Program {
    Vector<Instr> code;
    Vector<int> constInts;
    Vector<String> slots;
    bool supported = true;
    String unsupportedReason;
};

bool lowerCodeBlock(const CodeBlock& block, Program& outProgram);
EvalResult execute(const Program& program, SharedPtr<Scope> scope);

} // namespace FastIR

