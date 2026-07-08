#include "plab/viz/trace.hpp"

#include <gtest/gtest.h>

#include <string>

#include "plab/asm/assembler.hpp"
#include "plab/sim/pipeline.hpp"

using namespace plab;

namespace {

bool contains(const std::string& haystack, const std::string& needle) {
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

TEST(Viz, ReportContainsKeyMetrics) {
  asmr::Program prog = asmr::assemble("li r1, 1\nadd r2, r1, r1\nret");
  sim::Pipeline pipe(prog);
  pipe.run();
  const std::string report = viz::renderPerformanceReport(pipe.stats());
  EXPECT_TRUE(contains(report, "Performance Report"));
  EXPECT_TRUE(contains(report, "CPI"));
  EXPECT_TRUE(contains(report, "IPC"));
  EXPECT_TRUE(contains(report, "Forwards"));
}

TEST(Viz, DiagramShowsStagesAndInstructionText) {
  asmr::Program prog = asmr::assemble("add r1, r2, r3\nret");
  sim::Pipeline pipe(prog);
  pipe.run();
  const std::string diagram = viz::renderPipelineDiagram(pipe);
  EXPECT_TRUE(contains(diagram, "add r1, r2, r3"));
  EXPECT_TRUE(contains(diagram, "IF"));
  EXPECT_TRUE(contains(diagram, "WB"));
  EXPECT_TRUE(contains(diagram, "c1"));
}

TEST(Viz, LoadUseStallShowsRepeatedIdColumn) {
  // The dependent add sits in ID for two cycles -> "ID  ID" appears in its row.
  asmr::Program prog = asmr::assemble(
      "li r1, 4\nsw r1, 0(r0)\nlw r2, 0(r0)\nadd r3, r2, r2\nret");
  sim::Pipeline pipe(prog);
  pipe.run();
  const std::string diagram = viz::renderPipelineDiagram(pipe);
  EXPECT_TRUE(contains(diagram, "ID  ID"));
}
