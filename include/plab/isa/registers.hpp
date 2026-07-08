#ifndef PLAB_ISA_REGISTERS_HPP
#define PLAB_ISA_REGISTERS_HPP

#include <cstdint>

namespace plab::isa {

/// Number of architectural general-purpose registers (r0..r15).
inline constexpr std::uint8_t kNumRegisters = 16;

/// r0 is hardwired to zero: reads return 0, writes are discarded.
inline constexpr std::uint8_t kZeroReg = 0;

/// V2 ABI conventions (documented now, unused by the V1 single-function model).
/// r15 holds the return address for jal; r14 is the stack pointer.
inline constexpr std::uint8_t kReturnAddrReg = 15;
inline constexpr std::uint8_t kStackPtrReg = 14;

/// True if `r` names a valid architectural register.
constexpr bool isValidRegister(std::uint8_t r) noexcept {
  return r < kNumRegisters;
}

}  // namespace plab::isa

#endif  // PLAB_ISA_REGISTERS_HPP
