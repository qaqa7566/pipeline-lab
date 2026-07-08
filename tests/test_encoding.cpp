#include "plab/isa/encoding.hpp"

#include <gtest/gtest.h>

#include <cstdint>

#include "plab/isa/instruction.hpp"

using namespace plab::isa;

namespace {

// Compare the fields relevant to an instruction's format.
void expectSameInstruction(const Instruction& a, const Instruction& b) {
  EXPECT_EQ(a.op, b.op);
  switch (formatOf(a.op)) {
    case Format::R:
      EXPECT_EQ(a.rd, b.rd);
      EXPECT_EQ(a.rs, b.rs);
      EXPECT_EQ(a.rt, b.rt);
      break;
    case Format::I:
      EXPECT_EQ(a.rd, b.rd);
      EXPECT_EQ(a.rs, b.rs);
      EXPECT_EQ(a.imm, b.imm);
      break;
    case Format::B:
      EXPECT_EQ(a.rs, b.rs);
      EXPECT_EQ(a.rt, b.rt);
      EXPECT_EQ(a.imm, b.imm);
      break;
    case Format::J:
      EXPECT_EQ(a.target, b.target);
      break;
  }
}

}  // namespace

TEST(Encoding, RoundTripRType) {
  Instruction in{.op = Opcode::SUB, .rd = 3, .rs = 10, .rt = 15};
  const Instruction out = decode(encode(in));
  expectSameInstruction(in, out);
}

TEST(Encoding, RoundTripIType) {
  Instruction in{.op = Opcode::ADDI, .rd = 1, .rs = 2, .imm = 1234};
  expectSameInstruction(in, decode(encode(in)));
}

TEST(Encoding, RoundTripBType) {
  Instruction in{.op = Opcode::BEQ, .rs = 4, .rt = 5, .imm = -8};
  expectSameInstruction(in, decode(encode(in)));
}

TEST(Encoding, RoundTripJType) {
  Instruction in{.op = Opcode::JAL};
  in.target = 0x1000;
  expectSameInstruction(in, decode(encode(in)));
}

TEST(Encoding, OpcodeOccupiesTopSixBits) {
  Instruction in{.op = Opcode::RET};
  const std::uint32_t word = encode(in);
  EXPECT_EQ(word >> 26, static_cast<std::uint32_t>(Opcode::RET));
}

TEST(Encoding, NegativeImmediateSignExtends) {
  Instruction in{.op = Opcode::ADDI, .rd = 1, .rs = 0, .imm = -1};
  const Instruction out = decode(encode(in));
  EXPECT_EQ(out.imm, -1);
}

TEST(Encoding, ImmediateFieldBoundaries) {
  for (std::int32_t imm : {kImmMin, kImmMax, 0, -1, 1}) {
    Instruction in{.op = Opcode::ADDI, .rd = 2, .rs = 3, .imm = imm};
    EXPECT_EQ(decode(encode(in)).imm, imm) << "imm=" << imm;
  }
}

TEST(Encoding, RejectsOutOfRangeRegister) {
  Instruction in{.op = Opcode::ADD, .rd = 16, .rs = 0, .rt = 0};
  EXPECT_THROW(encode(in), EncodingError);
}

TEST(Encoding, RejectsOutOfRangeImmediate) {
  Instruction hi{.op = Opcode::ADDI, .rd = 1, .rs = 0, .imm = kImmMax + 1};
  Instruction lo{.op = Opcode::ADDI, .rd = 1, .rs = 0, .imm = kImmMin - 1};
  EXPECT_THROW(encode(hi), EncodingError);
  EXPECT_THROW(encode(lo), EncodingError);
}

TEST(Encoding, RejectsUnknownOpcode) {
  // Opcode 63 is well past RET (20) and unused.
  const std::uint32_t bad = std::uint32_t{63} << 26;
  EXPECT_THROW(decode(bad), DecodingError);
}

TEST(Encoding, MnemonicsAreStable) {
  EXPECT_EQ(mnemonicOf(Opcode::ADD), "add");
  EXPECT_EQ(mnemonicOf(Opcode::LW), "lw");
  EXPECT_EQ(mnemonicOf(Opcode::RET), "ret");
}
