#ifndef PLAB_SIM_INTERPRETER_HPP
#define PLAB_SIM_INTERPRETER_HPP

#include <cstdint>
#include <stdexcept>
#include <string>

#include "plab/asm/assembler.hpp"
#include "plab/sim/memory.hpp"
#include "plab/sim/register_file.hpp"

namespace plab::sim {

/// Thrown on a runtime execution fault (PC out of range, step-limit exceeded).
class ExecError : public std::runtime_error {
 public:
  explicit ExecError(const std::string& what) : std::runtime_error(what) {}
};

/// Sequential reference interpreter: executes one instruction to completion per
/// step with no pipeline. Produces the ground-truth architectural state
/// (registers + data memory) that the pipeline simulator is diff-tested against.
class Interpreter {
 public:
  explicit Interpreter(const asmr::Program& program, std::uint32_t dataBytes = 4096)
      : program_(program), mem_(dataBytes) {}

  /// Runs from PC 0 until a RET executes (or `maxSteps` is hit, which throws).
  /// Returns the number of instructions retired.
  std::uint64_t run(std::uint64_t maxSteps = 1'000'000);

  const RegisterFile& registers() const noexcept { return regs_; }
  const DataMemory& memory() const noexcept { return mem_; }
  RegisterFile& registers() noexcept { return regs_; }
  DataMemory& memory() noexcept { return mem_; }

  std::uint32_t pc() const noexcept { return pc_; }
  std::uint64_t instructionsRetired() const noexcept { return retired_; }

 private:
  const asmr::Program& program_;
  RegisterFile regs_;
  DataMemory mem_;
  std::uint32_t pc_{0};
  std::uint64_t retired_{0};
};

}  // namespace plab::sim

#endif  // PLAB_SIM_INTERPRETER_HPP
