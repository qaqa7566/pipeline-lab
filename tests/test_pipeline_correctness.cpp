// The central correctness invariant: the pipeline's final architectural state
// (registers + data memory) and retired-instruction count must match the
// sequential interpreter for every program, both with and without forwarding.
#include "plab/sim/pipeline.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "plab/asm/assembler.hpp"
#include "plab/sim/interpreter.hpp"

using namespace plab;

namespace {

const std::vector<std::string>& programs() {
  static const std::vector<std::string> ps = {
      // Dependent ALU chain (exercises forwarding).
      "li r1, 3\n"
      "add r2, r1, r1\n"
      "add r3, r2, r1\n"
      "sub r4, r3, r2\n"
      "ret",

      // Load-use hazard.
      "li r1, 77\n"
      "sw r1, 0(r0)\n"
      "lw r2, 0(r0)\n"
      "add r3, r2, r2\n"
      "ret",

      // Counting loop (backward branch).
      "      li r1, 0\n"
      "      li r2, 6\n"
      "loop: add r1, r1, r2\n"
      "      addi r2, r2, -1\n"
      "      bne r2, r0, loop\n"
      "      ret",

      // Not-taken then taken branch mix.
      "      li r1, 5\n"
      "      li r2, 5\n"
      "      beq r1, r2, eq\n"
      "      li r3, 111\n"
      "eq:   li r4, 222\n"
      "      ret",

      // Unconditional jump + jal link.
      "      j over\n"
      "      li r1, 999\n"
      "over: jal fn\n"
      "fn:   li r2, 5\n"
      "      ret",

      // max(r1, r2) -> r3, using slt + branch (control + data hazards).
      "       li r1, 7\n"
      "       li r2, 12\n"
      "       slt r4, r1, r2\n"      // r1 < r2 ? 1 : 0
      "       beq r4, r0, r1big\n"  // if not, r1 is larger
      "       mov r3, r2\n"
      "       j done\n"
      "r1big: mov r3, r1\n"
      "done:  ret",

      // Array sum: store 4 values, then load+accumulate in a loop.
      "      li r1, 10\n"
      "      sw r1, 0(r0)\n"
      "      li r1, 20\n"
      "      sw r1, 4(r0)\n"
      "      li r1, 30\n"
      "      sw r1, 8(r0)\n"
      "      li r5, 0\n"       // sum
      "      li r6, 0\n"       // index (bytes)
      "      li r7, 12\n"      // end
      "loop: lw r2, 0(r6)\n"
      "      add r5, r5, r2\n"
      "      addi r6, r6, 4\n"
      "      blt r6, r7, loop\n"
      "      ret",
  };
  return ps;
}

void expectMatchesOracle(const std::string& src, bool forwarding) {
  asmr::Program prog = asmr::assemble(src);

  sim::Interpreter interp(prog);
  const std::uint64_t oracleRetired = interp.run();

  sim::Pipeline pipe(prog, 4096, forwarding);
  const sim::PipelineStats stats = pipe.run();

  EXPECT_TRUE(pipe.registers() == interp.registers())
      << "register mismatch (forwarding=" << forwarding << ")\n"
      << src;
  EXPECT_TRUE(pipe.memory() == interp.memory())
      << "memory mismatch (forwarding=" << forwarding << ")\n"
      << src;
  EXPECT_EQ(stats.retired, oracleRetired)
      << "retired-count mismatch (forwarding=" << forwarding << ")\n"
      << src;
}

}  // namespace

TEST(PipelineCorrectness, MatchesInterpreterWithForwarding) {
  for (const auto& src : programs()) expectMatchesOracle(src, /*forwarding=*/true);
}

TEST(PipelineCorrectness, MatchesInterpreterWithoutForwarding) {
  for (const auto& src : programs()) expectMatchesOracle(src, /*forwarding=*/false);
}

TEST(PipelineCorrectness, ForwardingDoesNotChangeResults) {
  // Same program, both modes, must yield identical architectural state.
  const std::string src =
      "li r1, 4\nadd r2, r1, r1\nlw r3, 0(r0)\nadd r4, r2, r1\nret";
  asmr::Program prog = asmr::assemble(src);

  sim::Pipeline withFwd(prog, 4096, true);
  sim::Pipeline noFwd(prog, 4096, false);
  withFwd.run();
  noFwd.run();

  EXPECT_TRUE(withFwd.registers() == noFwd.registers());
  EXPECT_TRUE(withFwd.memory() == noFwd.memory());
}
