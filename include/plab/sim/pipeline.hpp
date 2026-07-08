#ifndef PLAB_SIM_PIPELINE_HPP
#define PLAB_SIM_PIPELINE_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "plab/asm/assembler.hpp"
#include "plab/isa/instruction.hpp"
#include "plab/sim/interpreter.hpp"  // ExecError
#include "plab/sim/memory.hpp"
#include "plab/sim/register_file.hpp"

namespace plab::sim {

/// The five pipeline stages, in order.
enum class Stage { IF, ID, EX, MEM, WB };

/// Sentinel meaning "this operand port reads no register".
inline constexpr std::uint8_t kNoReg = 0xFF;

/// Control signals derived from an instruction at decode time.
struct Control {
  bool regWrite = false;    ///< writes destReg in WB
  std::uint8_t destReg = 0; ///< register written (rd, or r15 for jal)
  bool memRead = false;     ///< lw
  bool memWrite = false;    ///< sw
  bool aluUsesImm = false;  ///< ALU second operand is the immediate
  bool isBranch = false;    ///< beq/bne/blt
  bool isJump = false;      ///< j/jal
  bool isJal = false;       ///< jal (links r15)
  bool isRet = false;       ///< ret/halt
  std::uint8_t aReg = kNoReg;  ///< source register feeding port A
  std::uint8_t bReg = kNoReg;  ///< source register feeding port B
};

/// Derives control signals (and source/destination registers) for `inst`.
Control makeControl(const isa::Instruction& inst);

/// A pipeline latch: the state carried between two stages. `bubble` marks a
/// pipeline gap (injected by reset, a stall, or a flush) that does nothing.
struct Latch {
  bool bubble = true;
  int id = -1;  ///< fetch sequence number, for tracing (-1 for bubbles)
  isa::Instruction inst{};
  std::uint32_t pc = 0;
  Control ctrl{};
  std::int32_t aVal = 0;       ///< port A operand value (captured at ID)
  std::int32_t bVal = 0;       ///< port B operand value (captured at ID)
  std::int32_t aluResult = 0;  ///< EX output / effective address / link value
  std::int32_t storeData = 0;  ///< value to be stored by sw (post-forwarding)
  std::int32_t writeData = 0;  ///< value written back at WB
};

/// One entry in the execution trace: instruction `id` occupied `stage` during
/// cycle `cycle` (1-based).
struct TraceEvent {
  int id = -1;
  std::uint64_t cycle = 0;
  Stage stage = Stage::IF;
};

/// Static description of a fetched instruction, indexed by trace id.
struct TracedInstr {
  std::uint32_t pc = 0;
  int sourceLine = 0;
  std::string text;  ///< disassembly (e.g. "add r1, r2, r3")
};

/// Cycle-level performance counters.
struct PipelineStats {
  std::uint64_t cycles = 0;       ///< total cycles executed
  std::uint64_t retired = 0;      ///< real instructions completed
  std::uint64_t stallCycles = 0;  ///< bubbles injected by load-use / RAW stalls
  std::uint64_t flushes = 0;      ///< taken branches + jumps (mispredicts)
  std::uint64_t flushBubbles = 0; ///< penalty cycles from flushes (2 per flush)
  std::uint64_t forwards = 0;     ///< operand values delivered by forwarding

  double cpi() const { return retired ? static_cast<double>(cycles) / static_cast<double>(retired) : 0.0; }
  double ipc() const { return cycles ? static_cast<double>(retired) / static_cast<double>(cycles) : 0.0; }
};

/// In-order, 5-stage pipeline simulator (IF/ID/EX/MEM/WB) with data-hazard
/// detection, forwarding, and branch flushing. Branches and jumps resolve in
/// EX (predict-not-taken, 2-cycle flush penalty). The register file writes in
/// the first half of a cycle and reads in the second, so WB->ID hazards need no
/// forwarding path.
class Pipeline {
 public:
  explicit Pipeline(const asmr::Program& program, std::uint32_t dataBytes = 4096,
                    bool enableForwarding = true)
      : program_(program), mem_(dataBytes), forwarding_(enableForwarding) {}

  /// Runs until the pipeline drains after a ret (or a fetch runs off the end).
  /// Throws ExecError if `maxCycles` is exceeded. Returns the final stats.
  const PipelineStats& run(std::uint64_t maxCycles = 1'000'000);

  const RegisterFile& registers() const noexcept { return regs_; }
  const DataMemory& memory() const noexcept { return mem_; }
  const PipelineStats& stats() const noexcept { return stats_; }
  const std::vector<TraceEvent>& trace() const noexcept { return trace_; }
  const std::vector<TracedInstr>& tracedInstrs() const noexcept { return traced_; }
  bool forwardingEnabled() const noexcept { return forwarding_; }

 private:
  // Per-cycle stage functions operate on current latches and fill `next`.
  void writeBack();
  void memoryStage(Latch& next);
  // EX may request a control transfer; fills the transfer outputs by reference.
  void execute(Latch& next, bool& flushYounger, bool& countFlush,
               bool& stopFetch, std::uint32_t& newPc);
  void decode(Latch& next);
  void fetchInto(Latch& next);
  bool detectStall() const;
  std::int32_t forwardValue(std::uint8_t reg, std::int32_t fallback);
  void recordStages(bool fetching, int fetchId);

  const asmr::Program& program_;
  RegisterFile regs_;
  DataMemory mem_;
  bool forwarding_;

  // Pipeline latches (current state).
  Latch ifid_, idex_, exmem_, memwb_;
  std::uint32_t pc_ = 0;
  bool fetchStopped_ = false;
  int nextId_ = 0;
  std::uint64_t cycle_ = 0;

  PipelineStats stats_;
  std::vector<TraceEvent> trace_;
  std::vector<TracedInstr> traced_;
};

}  // namespace plab::sim

#endif  // PLAB_SIM_PIPELINE_HPP
