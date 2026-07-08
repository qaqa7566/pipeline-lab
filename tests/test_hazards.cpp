// Targeted checks on stall, forwarding, branch-flush, and stat counters with
// hand-computed expectations on small programs.
#include "plab/sim/pipeline.hpp"

#include <gtest/gtest.h>

#include "plab/asm/assembler.hpp"

using namespace plab;

namespace {

sim::PipelineStats runFwd(std::string_view src, bool forwarding = true) {
  asmr::Program prog = asmr::assemble(src);
  sim::Pipeline pipe(prog, 4096, forwarding);
  return pipe.run();
}

}  // namespace

TEST(Hazards, BackToBackAluNeedsNoStallWithForwarding) {
  const auto s = runFwd(
      "li r1, 1\n"
      "add r2, r1, r1\n"   // depends on r1 (distance 1)
      "add r3, r2, r1\n"   // depends on r2 (distance 1)
      "ret");
  EXPECT_EQ(s.stallCycles, 0u);
  EXPECT_GT(s.forwards, 0u);
}

TEST(Hazards, BackToBackAluStallsWithoutForwarding) {
  const auto s = runFwd(
      "li r1, 1\n"
      "add r2, r1, r1\n"
      "add r3, r2, r1\n"
      "ret",
      /*forwarding=*/false);
  EXPECT_GT(s.stallCycles, 0u);
  EXPECT_EQ(s.forwards, 0u);
}

TEST(Hazards, LoadUseStallsExactlyOneCycle) {
  const auto s = runFwd(
      "li r1, 5\n"
      "sw r1, 0(r0)\n"
      "lw r2, 0(r0)\n"
      "add r3, r2, r2\n"   // uses loaded value immediately -> 1 stall
      "ret");
  EXPECT_EQ(s.stallCycles, 1u);
}

TEST(Hazards, LoadThenIndependentOpDoesNotStall) {
  const auto s = runFwd(
      "li r1, 5\n"
      "sw r1, 0(r0)\n"
      "lw r2, 0(r0)\n"
      "add r4, r1, r1\n"   // independent of the load -> no stall
      "ret");
  EXPECT_EQ(s.stallCycles, 0u);
}

TEST(Hazards, ForwardingBeatsNoForwardingOnStalls) {
  const std::string_view src =
      "li r1, 1\n"
      "add r2, r1, r1\n"
      "add r3, r2, r2\n"
      "add r4, r3, r3\n"
      "ret";
  EXPECT_LT(runFwd(src, true).stallCycles, runFwd(src, false).stallCycles);
}

TEST(Branches, TakenBranchCostsTwoFlushBubbles) {
  const auto s = runFwd(
      "      li r1, 0\n"
      "      beq r1, r0, tgt\n"  // taken
      "      li r2, 999\n"       // squashed
      "      li r3, 888\n"       // squashed
      "tgt:  ret");
  EXPECT_EQ(s.flushes, 1u);
  EXPECT_EQ(s.flushBubbles, 2u);
}

TEST(Branches, NotTakenBranchHasNoFlush) {
  const auto s = runFwd(
      "      li r1, 1\n"
      "      beq r1, r0, tgt\n"  // not taken
      "      li r2, 5\n"
      "tgt:  ret");
  EXPECT_EQ(s.flushes, 0u);
  EXPECT_EQ(s.flushBubbles, 0u);
}

TEST(Branches, UnconditionalJumpFlushes) {
  const auto s = runFwd(
      "      j tgt\n"
      "      li r1, 1\n"
      "tgt:  ret");
  EXPECT_EQ(s.flushes, 1u);
}

TEST(Stats, IdealPipelineHasFiveCyclesForOneInstructionPlusDrain) {
  // A single-instruction program: IF ID EX MEM WB then ret drains.
  const auto s = runFwd("ret");
  // ret: IF(c1) ID(c2) EX(c3) MEM(c4) WB(c5). retired = 1.
  EXPECT_EQ(s.retired, 1u);
  EXPECT_EQ(s.cycles, 5u);
}

TEST(Stats, CpiApproachesOneForLongHazardFreeRun) {
  // Independent instructions: no stalls, no flushes -> CPI -> 1 as N grows.
  std::string src;
  for (int i = 0; i < 50; ++i) src += "addi r1, r0, 1\n";
  src += "ret";
  const auto s = runFwd(src);
  EXPECT_EQ(s.stallCycles, 0u);
  EXPECT_EQ(s.flushes, 0u);
  EXPECT_LT(s.cpi(), 1.2);  // 51 instrs + 4-cycle fill => ~1.08
  EXPECT_GT(s.ipc(), 0.8);
}

TEST(Stats, RetiredCountsEveryRealInstruction) {
  const auto s = runFwd("li r1, 1\nli r2, 2\nadd r3, r1, r2\nret");
  EXPECT_EQ(s.retired, 4u);
}
