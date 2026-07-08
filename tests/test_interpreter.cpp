#include "plab/sim/interpreter.hpp"

#include <gtest/gtest.h>

#include "plab/asm/assembler.hpp"

using namespace plab;
using sim::Interpreter;

// Assemble `src`, run to completion, and expose the resulting interpreter as
// `interp` (with the backing Program kept alive for the test body).
#define RUN(src)                            \
  asmr::Program prog = asmr::assemble(src);  \
  Interpreter interp(prog);                 \
  interp.run()

TEST(Interpreter, BasicArithmetic) {
  RUN("li r1, 5\nli r2, 7\nadd r3, r1, r2\nret");
  EXPECT_EQ(interp.registers().read(3), 12);
}

TEST(Interpreter, AllAluOps) {
  RUN("li r1, 12\n"
      "li r2, 5\n"
      "sub r3, r1, r2\n"    // 7
      "and r4, r1, r2\n"    // 12 & 5 = 4
      "or  r5, r1, r2\n"    // 13
      "xor r6, r1, r2\n"    // 9
      "slt r7, r2, r1\n"    // 5 < 12 -> 1
      "ret");
  EXPECT_EQ(interp.registers().read(3), 7);
  EXPECT_EQ(interp.registers().read(4), 4);
  EXPECT_EQ(interp.registers().read(5), 13);
  EXPECT_EQ(interp.registers().read(6), 9);
  EXPECT_EQ(interp.registers().read(7), 1);
}

TEST(Interpreter, ShiftsAreLogical) {
  RUN("li r1, 1\n"
      "li r2, 4\n"
      "sll r3, r1, r2\n"   // 1 << 4 = 16
      "li r4, -1\n"
      "li r5, 28\n"
      "srl r6, r4, r5\n"   // 0xFFFFFFFF >> 28 = 15
      "ret");
  EXPECT_EQ(interp.registers().read(3), 16);
  EXPECT_EQ(interp.registers().read(6), 15);
}

TEST(Interpreter, LoadStoreRoundTrip) {
  RUN("li r1, 42\n"
      "sw r1, 0(r0)\n"
      "li r2, 99\n"
      "sw r2, 4(r0)\n"
      "lw r3, 0(r0)\n"
      "lw r4, 4(r0)\n"
      "ret");
  EXPECT_EQ(interp.registers().read(3), 42);
  EXPECT_EQ(interp.registers().read(4), 99);
  EXPECT_EQ(interp.memory().loadWord(0), 42);
  EXPECT_EQ(interp.memory().loadWord(4), 99);
}

TEST(Interpreter, BranchLoopSumsTo15) {
  // sum = 5 + 4 + 3 + 2 + 1 = 15
  RUN("      li r1, 0\n"
      "      li r2, 5\n"
      "loop: add r1, r1, r2\n"
      "      addi r2, r2, -1\n"
      "      bne r2, r0, loop\n"
      "      ret");
  EXPECT_EQ(interp.registers().read(1), 15);
  EXPECT_EQ(interp.registers().read(2), 0);
}

TEST(Interpreter, ZeroRegisterWritesIgnored) {
  RUN("addi r0, r0, 99\nadd r0, r0, r0\nret");
  EXPECT_EQ(interp.registers().read(0), 0);
}

TEST(Interpreter, UnconditionalJumpSkips) {
  RUN("      j skip\n"
      "      li r1, 999\n"
      "skip: li r2, 7\n"
      "      ret");
  EXPECT_EQ(interp.registers().read(1), 0);
  EXPECT_EQ(interp.registers().read(2), 7);
}

TEST(Interpreter, JalLinksReturnAddress) {
  // jal at address 0 -> r15 = 4, jumps to foo (address 4).
  RUN("     jal foo\n"
      "foo: ret");
  EXPECT_EQ(interp.registers().read(15), 4);
}

TEST(Interpreter, CountsRetiredInstructions) {
  RUN("li r1, 1\nli r2, 2\nadd r3, r1, r2\nret");
  EXPECT_EQ(interp.instructionsRetired(), 4u);  // 3 ops + ret
}

TEST(Interpreter, ThrowsOnInfiniteLoop) {
  asmr::Program prog = asmr::assemble("loop: j loop");
  Interpreter interp(prog);
  EXPECT_THROW(interp.run(1000), sim::ExecError);
}

TEST(Interpreter, ThrowsWhenPcRunsOffEnd) {
  asmr::Program prog = asmr::assemble("add r1, r0, r0");  // no ret
  Interpreter interp(prog);
  EXPECT_THROW(interp.run(), sim::ExecError);
}

TEST(Interpreter, ThrowsOnUnalignedAccess) {
  asmr::Program prog = asmr::assemble("li r1, 5\nlw r2, 2(r1)\nret");
  Interpreter interp(prog);
  EXPECT_THROW(interp.run(), sim::MemoryError);
}
