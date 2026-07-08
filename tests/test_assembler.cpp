#include "plab/asm/assembler.hpp"

#include <gtest/gtest.h>

#include "plab/isa/encoding.hpp"
#include "plab/isa/instruction.hpp"

using namespace plab::asmr;
using plab::isa::Instruction;
using plab::isa::Opcode;

namespace {

// Decode the assembled word at index `i` (round-trips through the encoder).
Instruction decoded(const Program& p, std::size_t i) {
  return plab::isa::decode(p.words.at(i));
}

}  // namespace

TEST(Assembler, EmitsOneWordPerInstruction) {
  const Program p = assemble("add r1, r2, r3\nsub r4, r5, r6\nret");
  EXPECT_EQ(p.words.size(), 3u);
  EXPECT_EQ(p.instructions.size(), 3u);
  EXPECT_EQ(p.sourceLines.size(), 3u);
}

TEST(Assembler, RType) {
  const Program p = assemble("slt r7, r8, r9");
  const Instruction i = decoded(p, 0);
  EXPECT_EQ(i.op, Opcode::SLT);
  EXPECT_EQ(i.rd, 7);
  EXPECT_EQ(i.rs, 8);
  EXPECT_EQ(i.rt, 9);
}

TEST(Assembler, ITypeImmediate) {
  const Program p = assemble("addi r1, r2, -42");
  const Instruction i = decoded(p, 0);
  EXPECT_EQ(i.op, Opcode::ADDI);
  EXPECT_EQ(i.rd, 1);
  EXPECT_EQ(i.rs, 2);
  EXPECT_EQ(i.imm, -42);
}

TEST(Assembler, LoadStoreEncoding) {
  const Program p = assemble("lw r3, 12(r4)\nsw r5, -4(r6)");
  const Instruction lw = decoded(p, 0);
  EXPECT_EQ(lw.op, Opcode::LW);
  EXPECT_EQ(lw.rd, 3);   // destination
  EXPECT_EQ(lw.rs, 4);   // base
  EXPECT_EQ(lw.imm, 12);
  const Instruction sw = decoded(p, 1);
  EXPECT_EQ(sw.op, Opcode::SW);
  EXPECT_EQ(sw.rd, 5);   // store-value source (rd slot)
  EXPECT_EQ(sw.rs, 6);   // base
  EXPECT_EQ(sw.imm, -4);
}

TEST(Assembler, ResolvesLabelsInSymbolTable) {
  const Program p = assemble("start: add r1, r0, r0\nloop: sub r1, r1, r0\n");
  EXPECT_EQ(p.symbols.at("start"), 0u);
  EXPECT_EQ(p.symbols.at("loop"), 4u);
}

TEST(Assembler, BackwardBranchOffsetIsPcRelative) {
  // loop is at address 0; the beq is the 3rd instruction (address 8).
  // offset = target(0) - branchAddr(8) = -8.
  const Program p = assemble(
      "loop: addi r1, r1, -1\n"
      "      addi r2, r2, 1\n"
      "      bne r1, r0, loop\n");
  const Instruction b = decoded(p, 2);
  EXPECT_EQ(b.op, Opcode::BNE);
  EXPECT_EQ(b.rs, 1);
  EXPECT_EQ(b.rt, 0);
  EXPECT_EQ(b.imm, -8);
}

TEST(Assembler, ForwardJumpTargetIsAbsolute) {
  const Program p = assemble(
      "      j end\n"
      "      add r1, r1, r1\n"
      "end:  ret\n");
  const Instruction j = decoded(p, 0);
  EXPECT_EQ(j.op, Opcode::J);
  EXPECT_EQ(j.target, 8u);  // 'end' is the 3rd instruction
}

TEST(Assembler, PseudoInstructionsExpand) {
  const Program p = assemble("nop\nmov r1, r2\nli r3, 100");
  const Instruction nop = decoded(p, 0);
  EXPECT_EQ(nop.op, Opcode::ADD);
  EXPECT_EQ(nop.rd, 0);
  EXPECT_EQ(nop.rs, 0);
  EXPECT_EQ(nop.rt, 0);

  const Instruction mov = decoded(p, 1);
  EXPECT_EQ(mov.op, Opcode::ADD);
  EXPECT_EQ(mov.rd, 1);
  EXPECT_EQ(mov.rs, 2);
  EXPECT_EQ(mov.rt, 0);

  const Instruction li = decoded(p, 2);
  EXPECT_EQ(li.op, Opcode::ADDI);
  EXPECT_EQ(li.rd, 3);
  EXPECT_EQ(li.rs, 0);
  EXPECT_EQ(li.imm, 100);
}

TEST(Assembler, HaltAliasesRet) {
  const Program p = assemble("halt");
  EXPECT_EQ(decoded(p, 0).op, Opcode::RET);
}

// ---- Error cases -----------------------------------------------------------

TEST(Assembler, RejectsUnknownMnemonic) {
  EXPECT_THROW(assemble("frobnicate r1, r2"), AssembleError);
}

TEST(Assembler, RejectsUndefinedLabel) {
  EXPECT_THROW(assemble("j nowhere"), AssembleError);
}

TEST(Assembler, RejectsDuplicateLabel) {
  EXPECT_THROW(assemble("dup: add r1, r0, r0\ndup: ret"), AssembleError);
}

TEST(Assembler, RejectsWrongOperandCount) {
  EXPECT_THROW(assemble("add r1, r2"), AssembleError);
}

TEST(Assembler, RejectsWrongOperandKind) {
  EXPECT_THROW(assemble("add r1, r2, 5"), AssembleError);  // rt must be a register
}

TEST(Assembler, RejectsOutOfRangeImmediate) {
  EXPECT_THROW(assemble("li r1, 200000"), AssembleError);
}
