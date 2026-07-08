#include "plab/sim/alu.hpp"

namespace plab::sim {

using isa::Opcode;

namespace {

std::int32_t asSigned(std::uint32_t v) { return static_cast<std::int32_t>(v); }
std::uint32_t asUnsigned(std::int32_t v) { return static_cast<std::uint32_t>(v); }

}  // namespace

std::int32_t aluResult(Opcode op, std::int32_t a, std::int32_t b) noexcept {
  const std::uint32_t ua = asUnsigned(a);
  const std::uint32_t ub = asUnsigned(b);
  const unsigned shamt = static_cast<unsigned>(ub) & 31u;

  switch (op) {
    case Opcode::ADD:
    case Opcode::ADDI:
      return asSigned(ua + ub);
    case Opcode::SUB:
      return asSigned(ua - ub);
    case Opcode::AND:
    case Opcode::ANDI:
      return asSigned(ua & ub);
    case Opcode::OR:
    case Opcode::ORI:
      return asSigned(ua | ub);
    case Opcode::XOR:
    case Opcode::XORI:
      return asSigned(ua ^ ub);
    case Opcode::SLL:
      return asSigned(ua << shamt);
    case Opcode::SRL:
      return asSigned(ua >> shamt);  // logical shift (operand is unsigned)
    case Opcode::SLT:
    case Opcode::SLTI:
      return a < b ? 1 : 0;  // signed comparison
    default:
      return 0;
  }
}

bool branchTaken(Opcode op, std::int32_t a, std::int32_t b) noexcept {
  switch (op) {
    case Opcode::BEQ: return a == b;
    case Opcode::BNE: return a != b;
    case Opcode::BLT: return a < b;
    default: return false;
  }
}

}  // namespace plab::sim
