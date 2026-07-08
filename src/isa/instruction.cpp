#include "plab/isa/instruction.hpp"

#include <string>

namespace plab::isa {

Format formatOf(Opcode op) noexcept {
  switch (op) {
    case Opcode::ADD:
    case Opcode::SUB:
    case Opcode::AND:
    case Opcode::OR:
    case Opcode::XOR:
    case Opcode::SLL:
    case Opcode::SRL:
    case Opcode::SLT:
      return Format::R;
    case Opcode::ADDI:
    case Opcode::ANDI:
    case Opcode::ORI:
    case Opcode::XORI:
    case Opcode::SLTI:
    case Opcode::LW:
    case Opcode::SW:
      return Format::I;
    case Opcode::BEQ:
    case Opcode::BNE:
    case Opcode::BLT:
      return Format::B;
    case Opcode::J:
    case Opcode::JAL:
    case Opcode::RET:
      return Format::J;
  }
  return Format::R;  // unreachable for valid opcodes
}

std::string_view mnemonicOf(Opcode op) noexcept {
  switch (op) {
    case Opcode::ADD: return "add";
    case Opcode::SUB: return "sub";
    case Opcode::AND: return "and";
    case Opcode::OR: return "or";
    case Opcode::XOR: return "xor";
    case Opcode::SLL: return "sll";
    case Opcode::SRL: return "srl";
    case Opcode::SLT: return "slt";
    case Opcode::ADDI: return "addi";
    case Opcode::ANDI: return "andi";
    case Opcode::ORI: return "ori";
    case Opcode::XORI: return "xori";
    case Opcode::SLTI: return "slti";
    case Opcode::LW: return "lw";
    case Opcode::SW: return "sw";
    case Opcode::BEQ: return "beq";
    case Opcode::BNE: return "bne";
    case Opcode::BLT: return "blt";
    case Opcode::J: return "j";
    case Opcode::JAL: return "jal";
    case Opcode::RET: return "ret";
  }
  return "???";
}

bool writesRegister(Opcode op) noexcept {
  switch (op) {
    // R-type and I-type ALU ops + loads write rd; jal writes the link register.
    case Opcode::ADD:
    case Opcode::SUB:
    case Opcode::AND:
    case Opcode::OR:
    case Opcode::XOR:
    case Opcode::SLL:
    case Opcode::SRL:
    case Opcode::SLT:
    case Opcode::ADDI:
    case Opcode::ANDI:
    case Opcode::ORI:
    case Opcode::XORI:
    case Opcode::SLTI:
    case Opcode::LW:
    case Opcode::JAL:
      return true;
    default:
      return false;
  }
}

bool isBranch(Opcode op) noexcept {
  return op == Opcode::BEQ || op == Opcode::BNE || op == Opcode::BLT;
}

bool isJump(Opcode op) noexcept {
  return op == Opcode::J || op == Opcode::JAL;
}

bool isLoad(Opcode op) noexcept { return op == Opcode::LW; }

bool isStore(Opcode op) noexcept { return op == Opcode::SW; }

std::string disassemble(const Instruction& inst) {
  const auto reg = [](std::uint8_t r) { return "r" + std::to_string(r); };
  const std::string m{mnemonicOf(inst.op)};

  switch (formatOf(inst.op)) {
    case Format::R:
      if (inst.op == Opcode::ADD && inst.rd == 0 && inst.rs == 0 && inst.rt == 0) {
        return "nop";
      }
      return m + " " + reg(inst.rd) + ", " + reg(inst.rs) + ", " + reg(inst.rt);
    case Format::I:
      if (isLoad(inst.op) || isStore(inst.op)) {
        return m + " " + reg(inst.rd) + ", " + std::to_string(inst.imm) + "(" +
               reg(inst.rs) + ")";
      }
      return m + " " + reg(inst.rd) + ", " + reg(inst.rs) + ", " +
             std::to_string(inst.imm);
    case Format::B:
      return m + " " + reg(inst.rs) + ", " + reg(inst.rt) + ", " +
             std::to_string(inst.imm);
    case Format::J:
      if (inst.op == Opcode::RET) return "ret";
      return m + " " + std::to_string(inst.target);
  }
  return m;
}

}  // namespace plab::isa
