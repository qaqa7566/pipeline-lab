#include "plab/viz/trace.hpp"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace plab::viz {

namespace {

const char* stageName(sim::Stage s) {
  switch (s) {
    case sim::Stage::IF: return "IF";
    case sim::Stage::ID: return "ID";
    case sim::Stage::EX: return "EX";
    case sim::Stage::MEM: return "MEM";
    case sim::Stage::WB: return "WB";
  }
  return "?";
}

std::string hexPc(std::uint32_t pc) {
  std::ostringstream os;
  os << "0x" << std::hex << std::setw(4) << std::setfill('0') << pc;
  return os.str();
}

}  // namespace

std::string renderPipelineDiagram(const sim::Pipeline& pipeline) {
  const auto& trace = pipeline.trace();
  const auto& instrs = pipeline.tracedInstrs();
  const std::uint64_t cycles = pipeline.stats().cycles;
  const std::size_t n = instrs.size();

  // grid[id][cycle] holds the stage abbreviation (empty = not in the pipeline).
  std::vector<std::vector<std::string>> grid(
      n, std::vector<std::string>(static_cast<std::size_t>(cycles) + 1, ""));
  for (const auto& e : trace) {
    if (e.id < 0 || static_cast<std::size_t>(e.id) >= n) continue;
    grid[static_cast<std::size_t>(e.id)][static_cast<std::size_t>(e.cycle)] =
        stageName(e.stage);
  }

  constexpr int kLabelW = 28;
  constexpr int kCellW = 4;
  std::ostringstream os;

  // Header row.
  os << std::left << std::setw(kLabelW) << "addr  instruction" << std::right;
  for (std::uint64_t c = 1; c <= cycles; ++c) {
    os << std::setw(kCellW) << ("c" + std::to_string(c));
  }
  os << '\n';

  // One row per fetched instruction.
  for (std::size_t id = 0; id < n; ++id) {
    std::string label = hexPc(instrs[id].pc) + "  " + instrs[id].text;
    if (static_cast<int>(label.size()) >= kLabelW) {
      label = label.substr(0, static_cast<std::size_t>(kLabelW) - 1);
    }
    os << std::left << std::setw(kLabelW) << label << std::right;
    for (std::uint64_t c = 1; c <= cycles; ++c) {
      const std::string& cell = grid[id][static_cast<std::size_t>(c)];
      os << std::setw(kCellW) << (cell.empty() ? "." : cell);
    }
    os << '\n';
  }
  return os.str();
}

std::string renderPerformanceReport(const sim::PipelineStats& stats) {
  std::ostringstream os;
  os << std::fixed << std::setprecision(3);
  os << "=== Performance Report ===\n";
  os << "Cycles           : " << stats.cycles << '\n';
  os << "Instructions     : " << stats.retired << '\n';
  os << "CPI              : " << stats.cpi() << '\n';
  os << "IPC              : " << stats.ipc() << '\n';
  os << "Stall cycles     : " << stats.stallCycles << "  (load-use / RAW)\n";
  os << "Flushes          : " << stats.flushes << "  (" << stats.flushBubbles
     << " penalty cycles)\n";
  os << "Forwards         : " << stats.forwards << '\n';
  return os.str();
}

}  // namespace plab::viz
