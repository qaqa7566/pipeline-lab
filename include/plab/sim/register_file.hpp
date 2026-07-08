#ifndef PLAB_SIM_REGISTER_FILE_HPP
#define PLAB_SIM_REGISTER_FILE_HPP

#include <array>
#include <cstdint>

#include "plab/isa/registers.hpp"

namespace plab::sim {

/// The 16 general-purpose registers. r0 is hardwired to zero: reads return 0
/// and writes to it are discarded.
class RegisterFile {
 public:
  std::int32_t read(std::uint8_t index) const { return regs_[index]; }

  void write(std::uint8_t index, std::int32_t value) {
    if (index != isa::kZeroReg) regs_[index] = value;
  }

  const std::array<std::int32_t, isa::kNumRegisters>& raw() const { return regs_; }

  bool operator==(const RegisterFile& other) const { return regs_ == other.regs_; }

 private:
  std::array<std::int32_t, isa::kNumRegisters> regs_{};
};

}  // namespace plab::sim

#endif  // PLAB_SIM_REGISTER_FILE_HPP
