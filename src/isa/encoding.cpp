#include "plab/isa/encoding.hpp"

#include <string>

#include "plab/isa/registers.hpp"

namespace plab::isa {

namespace {

constexpr std::uint32_t kOpcodeShift = 26;
constexpr std::uint32_t kRdShift = 22;   // R/I: rd
constexpr std::uint32_t kRsShift = 18;   // R/I: rs ; B: rt (see layout)
constexpr std::uint32_t kRtShift = 14;   // R: rt

constexpr std::uint32_t kRegMask = 0xF;          // 4 bits
constexpr std::uint32_t kImmMask = 0x3FFFF;      // 18 bits
constexpr std::uint32_t kTargetMask = 0x3FFFFFF; // 26 bits

// Sign-extend the low `bits` of `v` to a 32-bit signed value.
std::int32_t signExtend(std::uint32_t v, int bits) {
  const std::uint32_t m = std::uint32_t{1} << (bits - 1);
  return static_cast<std::int32_t>((v ^ m) - m);
}

void requireReg(std::uint8_t r, const char* field) {
  if (!isValidRegister(r)) {
    throw EncodingError(std::string("register index out of range for field ") +
                        field + ": r" + std::to_string(r));
  }
}

std::uint32_t regField(std::uint8_t r) { return static_cast<std::uint32_t>(r) & kRegMask; }

}  // namespace

std::uint32_t encode(const Instruction& inst) {
  const auto op = static_cast<std::uint32_t>(inst.op);
  std::uint32_t word = op << kOpcodeShift;

  switch (formatOf(inst.op)) {
    case Format::R:
      requireReg(inst.rd, "rd");
      requireReg(inst.rs, "rs");
      requireReg(inst.rt, "rt");
      word |= regField(inst.rd) << kRdShift;
      word |= regField(inst.rs) << kRsShift;
      word |= regField(inst.rt) << kRtShift;
      break;

    case Format::I:
      requireReg(inst.rd, "rd");
      requireReg(inst.rs, "rs");
      if (inst.imm < kImmMin || inst.imm > kImmMax) {
        throw EncodingError("immediate out of range: " + std::to_string(inst.imm));
      }
      word |= regField(inst.rd) << kRdShift;
      word |= regField(inst.rs) << kRsShift;
      word |= static_cast<std::uint32_t>(inst.imm) & kImmMask;
      break;

    case Format::B:
      // B-type reuses the rd/rs slots for its two source registers.
      requireReg(inst.rs, "rs");
      requireReg(inst.rt, "rt");
      if (inst.imm < kImmMin || inst.imm > kImmMax) {
        throw EncodingError("branch offset out of range: " + std::to_string(inst.imm));
      }
      word |= regField(inst.rs) << kRdShift;
      word |= regField(inst.rt) << kRsShift;
      word |= static_cast<std::uint32_t>(inst.imm) & kImmMask;
      break;

    case Format::J:
      if (inst.target > kTargetMask) {
        throw EncodingError("jump target out of range: " + std::to_string(inst.target));
      }
      word |= inst.target & kTargetMask;
      break;
  }
  return word;
}

Instruction decode(std::uint32_t word) {
  const std::uint32_t opv = (word >> kOpcodeShift) & 0x3F;
  if (opv > static_cast<std::uint32_t>(Opcode::RET)) {
    throw DecodingError("unknown opcode: " + std::to_string(opv));
  }

  Instruction inst;
  inst.op = static_cast<Opcode>(opv);

  switch (formatOf(inst.op)) {
    case Format::R:
      inst.rd = static_cast<std::uint8_t>((word >> kRdShift) & kRegMask);
      inst.rs = static_cast<std::uint8_t>((word >> kRsShift) & kRegMask);
      inst.rt = static_cast<std::uint8_t>((word >> kRtShift) & kRegMask);
      break;
    case Format::I:
      inst.rd = static_cast<std::uint8_t>((word >> kRdShift) & kRegMask);
      inst.rs = static_cast<std::uint8_t>((word >> kRsShift) & kRegMask);
      inst.imm = signExtend(word & kImmMask, kImmBits);
      break;
    case Format::B:
      inst.rs = static_cast<std::uint8_t>((word >> kRdShift) & kRegMask);
      inst.rt = static_cast<std::uint8_t>((word >> kRsShift) & kRegMask);
      inst.imm = signExtend(word & kImmMask, kImmBits);
      break;
    case Format::J:
      inst.target = word & kTargetMask;
      break;
  }
  return inst;
}

}  // namespace plab::isa
