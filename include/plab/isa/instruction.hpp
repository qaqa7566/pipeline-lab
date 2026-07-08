#ifndef PLAB_ISA_INSTRUCTION_HPP
#define PLAB_ISA_INSTRUCTION_HPP

#include <cstdint>
#include <string>
#include <string_view>

namespace plab::isa {

/// Operation codes. Values are the 6-bit opcode field emitted by the encoder;
/// they must stay within [0, 63] and are grouped by instruction format.
enum class Opcode : std::uint8_t {
  // R-type (register-register ALU)
  ADD = 0,
  SUB = 1,
  AND = 2,
  OR = 3,
  XOR = 4,
  SLL = 5,
  SRL = 6,
  SLT = 7,
  // I-type (immediate ALU + memory)
  ADDI = 8,
  ANDI = 9,
  ORI = 10,
  XORI = 11,
  SLTI = 12,
  LW = 13,
  SW = 14,
  // B-type (PC-relative branches)
  BEQ = 15,
  BNE = 16,
  BLT = 17,
  // J-type (jumps / halt)
  J = 18,
  JAL = 19,
  RET = 20,  // V1: terminates execution (return from top-level / halt)
};

/// Instruction encoding formats.
enum class Format : std::uint8_t { R, I, B, J };

/// Decoded instruction. Only the fields relevant to the opcode's format are
/// meaningful; the rest stay zero.
///
///  R: op rd rs rt
///  I: op rd rs imm            (imm is 18-bit sign-extended)
///  B: op rs rt imm            (imm is a 18-bit sign-extended byte offset)
///  J: op target               (target is a 26-bit absolute byte address)
struct Instruction {
  Opcode op{Opcode::ADD};
  std::uint8_t rd{0};
  std::uint8_t rs{0};
  std::uint8_t rt{0};
  std::int32_t imm{0};       // I-type immediate / B-type branch offset
  std::uint32_t target{0};   // J-type jump target (byte address)
};

/// Encoding format for an opcode.
Format formatOf(Opcode op) noexcept;

/// Lowercase mnemonic (e.g. "add"). Returns "???" for unknown values.
std::string_view mnemonicOf(Opcode op) noexcept;

/// True if executing `op` writes a general-purpose register (RegWrite signal).
bool writesRegister(Opcode op) noexcept;

bool isBranch(Opcode op) noexcept;  ///< beq/bne/blt
bool isJump(Opcode op) noexcept;    ///< j/jal (unconditional transfer)
bool isLoad(Opcode op) noexcept;    ///< lw
bool isStore(Opcode op) noexcept;   ///< sw

/// Renders an instruction back to assembly text (e.g. "add r1, r2, r3").
/// Recognizes the `nop` idiom (add r0, r0, r0).
std::string disassemble(const Instruction& inst);

}  // namespace plab::isa

#endif  // PLAB_ISA_INSTRUCTION_HPP
