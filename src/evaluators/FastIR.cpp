#include "core/evaluators/FastIR.hpp"

#include "ast/Ast.hpp"
#include "ast/AstControl.hpp"
#include "ast/AstFunction.hpp"
#include "core/Environments/Scope.hpp"
#include "core/Environments/StackScope.hpp"
#include "core/errors.h"

namespace FastIR {
namespace {

class Lowerer {
public:
    explicit Lowerer(Program& p) : p(p) {}

    bool lowerBlock(const CodeBlock& block) {
        const auto& children = block.getChildren();
        for (const auto& child : children) {
            if (!child) continue;
            if (!lowerStatement(*child)) return false;
        }
        return true;
    }

private:
    Program& p;

    int slotFor(const String& name) {
        for (size_t i = 0; i < p.slots.size(); ++i) {
            if (p.slots[i] == name) return static_cast<int>(i);
        }
        p.slots.push_back(name);
        return static_cast<int>(p.slots.size() - 1);
    }

    int constFor(int value) {
        p.constInts.push_back(value);
        return static_cast<int>(p.constInts.size() - 1);
    }

    void emit(OpCode op, int arg = 0) {
        p.code.push_back(Instr{op, arg});
    }

    bool fail(const String& why) {
        p.supported = false;
        p.unsupportedReason = why;
        return false;
    }

    bool lowerStatement(const BaseAST& st) {
        switch (st.getAstType()) {
            case AstType::VariableDeclaration: {
                const auto& vd = static_cast<const VariableDeclaration&>(st);
                if (!vd.getRawExpression()) return fail("VariableDeclaration missing expression");
                if (!lowerExpr(*vd.getRawExpression())) return false;
                emit(OpCode::StoreVarIntDecl, slotFor(vd.getName()));
                return true;
            }
            case AstType::VariableAssignment: {
                const auto& va = static_cast<const VariableAssignment&>(st);
                if (!va.getRawExpression()) return fail("VariableAssignment missing expression");
                if (!lowerExpr(*va.getRawExpression())) return false;
                emit(OpCode::StoreVarInt, slotFor(va.getName()));
                return true;
            }
            case AstType::WhileLoop: {
                const auto& wl = static_cast<const WhileLoop&>(st);
                const auto* cond = wl.getCondition();
                const auto* body = wl.getBody();
                if (!cond || !cond->getCondition()) return fail("WhileLoop missing condition");
                if (!body) return fail("WhileLoop missing body");

                const int loopStart = static_cast<int>(p.code.size());
                if (!lowerExpr(*cond->getCondition())) return false;
                const int jumpIfFalsePos = static_cast<int>(p.code.size());
                emit(OpCode::JumpIfFalse, -1);

                if (!lowerBlock(*body)) return false;
                emit(OpCode::Jump, loopStart);

                const int loopEnd = static_cast<int>(p.code.size());
                p.code[static_cast<size_t>(jumpIfFalsePos)].arg = loopEnd;
                return true;
            }
            case AstType::IfStatement: {
                const auto& ifs = static_cast<const IfStatement&>(st);
                if (!ifs.getCondition()) return fail("IfStatement missing condition");
                if (!ifs.getBody()) return fail("IfStatement missing body");
                if (!ifs.getElifs().empty()) {
                    return fail("IfStatement with elif is not supported in FastIR");
                }

                if (!lowerExpr(*ifs.getCondition())) return false;
                const int jFalsePos = static_cast<int>(p.code.size());
                emit(OpCode::JumpIfFalse, -1);

                if (!lowerBlock(*ifs.getBody())) return false;

                const auto& elseNode = ifs.getElse();
                if (elseNode && elseNode->getBody()) {
                    const int jEndPos = static_cast<int>(p.code.size());
                    emit(OpCode::Jump, -1);
                    const int elseStart = static_cast<int>(p.code.size());
                    p.code[static_cast<size_t>(jFalsePos)].arg = elseStart;

                    if (!lowerBlock(*elseNode->getBody())) return false;
                    const int endPos = static_cast<int>(p.code.size());
                    p.code[static_cast<size_t>(jEndPos)].arg = endPos;
                    return true;
                }

                const int endPos = static_cast<int>(p.code.size());
                p.code[static_cast<size_t>(jFalsePos)].arg = endPos;
                return true;
            }
            case AstType::CodeBlock: {
                const auto& cb = static_cast<const CodeBlock&>(st);
                return lowerBlock(cb);
            }
            case AstType::FunctionCall: {
                const auto& fc = static_cast<const FunctionCall&>(st);
                const String n = fc.getName();
                if (n == "print" || n == "DEBUG_LOG" || n == "showScope") {
                    // Side-effect-only debug/print calls are skipped in the int fast-path.
                    return true;
                }
                return fail("Unsupported function call in FastIR: " + n);
            }
            case AstType::NoOp:
                return true;
            default:
                return fail("Unsupported statement in FastIR: " + st.getAstTypeAsString());
        }
    }

    bool lowerExpr(const ASTStatement& expr) {
        switch (expr.getAstType()) {
            case AstType::Literal: {
                const Node n = expr.evaluate(nullptr, nullptr);
                if (n.isInt()) {
                    emit(OpCode::PushConstInt, constFor(n.toInt()));
                    return true;
                }
                if (n.isBool()) {
                    emit(OpCode::PushConstInt, constFor(n.toBool() ? 1 : 0));
                    return true;
                }
                return fail("Only int/bool literals supported in FastIR");
            }
            case AstType::VariableReference: {
                const auto& vr = static_cast<const VariableReference&>(expr);
                emit(OpCode::LoadVarInt, slotFor(vr.getName()));
                return true;
            }
            case AstType::UnaryOperation: {
                const auto& u = static_cast<const UnaryOperation&>(expr);
                const ASTStatement* opnd = u.getOperand();
                if (!opnd) return fail("Unary operation missing operand");
                if (!lowerExpr(*opnd)) return false;
                if (u.getOperator() == "-") {
                    emit(OpCode::NegInt);
                    return true;
                }
                if (u.getOperator() == "!" || u.getOperator() == "not") {
                    emit(OpCode::NotBool);
                    return true;
                }
                return fail("Unsupported unary op in FastIR: " + u.getOperator());
            }
            case AstType::BinaryOperation: {
                const auto& b = static_cast<const BinaryOperation&>(expr);
                const auto* l = b.getLeftSide();
                const auto* r = b.getRightSide();
                if (!l || !r) return fail("Binary op missing side");
                if (!lowerExpr(*l)) return false;
                if (!lowerExpr(*r)) return false;

                const String& op = b.getOperator();
                if (op == "+") { emit(OpCode::AddInt); return true; }
                if (op == "-") { emit(OpCode::SubInt); return true; }
                if (op == "*") { emit(OpCode::MulInt); return true; }
                if (op == "/") { emit(OpCode::DivInt); return true; }
                if (op == "%") { emit(OpCode::ModInt); return true; }
                if (op == "<") { emit(OpCode::CmpLtInt); return true; }
                if (op == "<=") { emit(OpCode::CmpLeInt); return true; }
                if (op == ">") { emit(OpCode::CmpGtInt); return true; }
                if (op == ">=") { emit(OpCode::CmpGeInt); return true; }
                if (op == "==") { emit(OpCode::CmpEqInt); return true; }
                if (op == "!=") { emit(OpCode::CmpNeInt); return true; }
                return fail("Unsupported binary op in FastIR: " + op);
            }
            default:
                return fail("Unsupported expr in FastIR: " + expr.getAstTypeAsString());
        }
    }
};

int pop1(Vector<int>& stack) {
    if (stack.empty()) throw MerkError("FastIR stack underflow");
    const int v = stack.back();
    stack.pop_back();
    return v;
}

} // namespace

bool lowerCodeBlock(const CodeBlock& block, Program& outProgram) {
    outProgram = Program{};
    Lowerer lowerer(outProgram);
    return lowerer.lowerBlock(block);
}

EvalResult execute(const Program& program, SharedPtr<Scope> scope) {
    if (!scope) throw MerkError("FastIR execute: scope is null");
    if (!program.supported) throw MerkError("FastIR execute: program unsupported");

    Vector<int> stack;
    stack.reserve(256);

    struct BoundIntSlot {
        Frame::FrameCell* owned = nullptr;
        VarNode* borrowed = nullptr;
    };

    Vector<BoundIntSlot> boundSlots;
    SharedPtr<StackScope> stackScope = std::dynamic_pointer_cast<StackScope>(scope);
    if (stackScope) {
        boundSlots.resize(program.slots.size());
        Frame& frame = stackScope->getFrame();
        for (size_t i = 0; i < program.slots.size(); ++i) {
            const String& name = program.slots[i];

            if (auto* cell = frame.getOwnedCell(name)) {
                if (cell->kind != Frame::SlotKind::Int) {
                    int v = 0;
                    if (cell->toNode().isInt()) v = cell->toNode().toInt();
                    else if (cell->toNode().isBool()) v = cell->toNode().toBool() ? 1 : 0;
                    cell->kind = Frame::SlotKind::Int;
                    cell->intValue = v;
                }
                boundSlots[i].owned = cell;
                continue;
            }

            if (VarNode* slot = frame.getSlot(name)) {
                if (slot->getValueNode().isInt() || slot->getValueNode().isBool()) {
                    boundSlots[i].borrowed = slot;
                    continue;
                }
                throw MerkError("FastIR prebind requires int/bool variable: " + name);
            }

            DataTypeFlags flags;
            flags.name = name;
            boundSlots[i].owned = frame.ensureOwnedIntCell(name, 0, &flags);
        }
    }

    int pc = 0;
    while (pc >= 0 && pc < static_cast<int>(program.code.size())) {
        const Instr& ins = program.code[static_cast<size_t>(pc)];
        switch (ins.op) {
            case OpCode::PushConstInt:
                stack.push_back(program.constInts[static_cast<size_t>(ins.arg)]);
                ++pc;
                break;

            case OpCode::LoadVarInt: {
                const size_t idx = static_cast<size_t>(ins.arg);
                if (stackScope) {
                    const auto& bs = boundSlots[idx];
                    if (bs.owned) {
                        if (bs.owned->kind != Frame::SlotKind::Int) {
                            throw MerkError("FastIR bound owned slot is non-int");
                        }
                        stack.push_back(bs.owned->intValue);
                    } else if (bs.borrowed) {
                        const Node& n = bs.borrowed->getValueNode();
                        if (n.isInt()) stack.push_back(n.toInt());
                        else if (n.isBool()) stack.push_back(n.toBool() ? 1 : 0);
                        else throw MerkError("FastIR borrowed slot requires int/bool");
                    } else {
                        throw MerkError("FastIR unbound slot");
                    }
                } else {
                    const String& name = program.slots[idx];
                    int out = 0;
                    if (scope->tryReadInt(name, out)) {
                        stack.push_back(out);
                    } else {
                        auto& v = scope->getVariable(name).getValueNode();
                        if (v.isInt()) stack.push_back(v.toInt());
                        else if (v.isBool()) stack.push_back(v.toBool() ? 1 : 0);
                        else throw MerkError("FastIR LoadVarInt requires int/bool variable: " + name);
                    }
                }
                ++pc;
                break;
            }

            case OpCode::StoreVarIntDecl: {
                const size_t idx = static_cast<size_t>(ins.arg);
                const int value = pop1(stack);
                if (stackScope) {
                    auto& bs = boundSlots[idx];
                    if (bs.owned) {
                        bs.owned->kind = Frame::SlotKind::Int;
                        bs.owned->intValue = value;
                        if (bs.owned->materialized) {
                            bs.owned->materialized->setValue(Node(value));
                        }
                    } else if (bs.borrowed) {
                        bs.borrowed->setValue(Node(value));
                    } else {
                        throw MerkError("FastIR unbound slot on decl store");
                    }
                } else {
                    const String& name = program.slots[idx];
                    if (scope->hasVariable(name)) {
                        if (!scope->tryWriteInt(name, value)) {
                            scope->updateVariable(name, Node(value));
                        }
                    } else {
                        DataTypeFlags flags;
                        scope->declareVariable(name, makeUnique<VarNode>(Node(value), flags));
                    }
                }
                ++pc;
                break;
            }

            case OpCode::StoreVarInt: {
                const size_t idx = static_cast<size_t>(ins.arg);
                const int value = pop1(stack);
                if (stackScope) {
                    auto& bs = boundSlots[idx];
                    if (bs.owned) {
                        bs.owned->kind = Frame::SlotKind::Int;
                        bs.owned->intValue = value;
                        if (bs.owned->materialized) {
                            bs.owned->materialized->setValue(Node(value));
                        }
                    } else if (bs.borrowed) {
                        bs.borrowed->setValue(Node(value));
                    } else {
                        throw MerkError("FastIR unbound slot on store");
                    }
                } else {
                    const String& name = program.slots[idx];
                    if (!scope->tryWriteInt(name, value)) {
                        scope->updateVariable(name, Node(value));
                    }
                }
                ++pc;
                break;
            }

            case OpCode::AddInt: { const int b = pop1(stack); const int a = pop1(stack); stack.push_back(a + b); ++pc; break; }
            case OpCode::SubInt: { const int b = pop1(stack); const int a = pop1(stack); stack.push_back(a - b); ++pc; break; }
            case OpCode::MulInt: { const int b = pop1(stack); const int a = pop1(stack); stack.push_back(a * b); ++pc; break; }
            case OpCode::DivInt: {
                const int b = pop1(stack);
                const int a = pop1(stack);
                if (b == 0) throw MerkError("FastIR division by zero");
                stack.push_back(a / b);
                ++pc;
                break;
            }
            case OpCode::ModInt: {
                const int b = pop1(stack);
                const int a = pop1(stack);
                if (b == 0) throw MerkError("FastIR modulo by zero");
                stack.push_back(a % b);
                ++pc;
                break;
            }
            case OpCode::CmpLtInt: { const int b = pop1(stack); const int a = pop1(stack); stack.push_back(a < b ? 1 : 0); ++pc; break; }
            case OpCode::CmpLeInt: { const int b = pop1(stack); const int a = pop1(stack); stack.push_back(a <= b ? 1 : 0); ++pc; break; }
            case OpCode::CmpGtInt: { const int b = pop1(stack); const int a = pop1(stack); stack.push_back(a > b ? 1 : 0); ++pc; break; }
            case OpCode::CmpGeInt: { const int b = pop1(stack); const int a = pop1(stack); stack.push_back(a >= b ? 1 : 0); ++pc; break; }
            case OpCode::CmpEqInt: { const int b = pop1(stack); const int a = pop1(stack); stack.push_back(a == b ? 1 : 0); ++pc; break; }
            case OpCode::CmpNeInt: { const int b = pop1(stack); const int a = pop1(stack); stack.push_back(a != b ? 1 : 0); ++pc; break; }
            case OpCode::NegInt: {
                const int v = pop1(stack);
                stack.push_back(-v);
                ++pc;
                break;
            }
            case OpCode::NotBool: {
                const int v = pop1(stack);
                stack.push_back(v == 0 ? 1 : 0);
                ++pc;
                break;
            }

            case OpCode::Jump:
                pc = ins.arg;
                break;

            case OpCode::JumpIfFalse: {
                const int cond = pop1(stack);
                if (cond == 0) pc = ins.arg;
                else ++pc;
                break;
            }
        }
    }

    return EvalResult::Normal(Node());
}

} // namespace FastIR
