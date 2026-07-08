#ifndef PLAB_SIM_ALU_HPP
#define PLAB_SIM_ALU_HPP

#include <cstdint>

#include "plab/isa/instruction.hpp"

namespace plab::sim {

/// Evaluates the ALU result for an arithmetic/logic opcode given two operands.
/// For R-type ops both operands are registers; for I-type ops `b` is the
/// sign-extended immediate. Shared by the interpreter and the pipeline EX stage
/// so the two models cannot drift apart.
///
/// Arithmetic uses wrap-around (unsigned) semantics to avoid signed-overflow UB.
std::int32_t aluResult(isa::Opcode op, std::int32_t a, std::int32_t b) noexcept;

/// Evaluates a branch condition (beq/bne/blt) on two register values.
bool branchTaken(isa::Opcode op, std::int32_t a, std::int32_t b) noexcept;

}  // namespace plab::sim

#endif  // PLAB_SIM_ALU_HPP
