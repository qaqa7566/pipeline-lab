#ifndef PLAB_ISA_ENCODING_HPP
#define PLAB_ISA_ENCODING_HPP

#include <cstdint>
#include <stdexcept>
#include <string>

#include "plab/isa/instruction.hpp"

namespace plab::isa {

/// Thrown when an Instruction cannot be represented in 32 bits (register index
/// out of range, immediate/offset/target too large for its field).
class EncodingError : public std::runtime_error {
 public:
  explicit EncodingError(const std::string& what) : std::runtime_error(what) {}
};

/// Thrown when a 32-bit word does not decode to a known instruction.
class DecodingError : public std::runtime_error {
 public:
  explicit DecodingError(const std::string& what) : std::runtime_error(what) {}
};

// ---- Field widths (bits) ---------------------------------------------------
inline constexpr int kOpcodeBits = 6;
inline constexpr int kRegBits = 4;      // 16 registers
inline constexpr int kImmBits = 18;     // I-type immediate / B-type offset
inline constexpr int kTargetBits = 26;  // J-type target

// Inclusive value ranges for the signed 18-bit immediate/offset field.
inline constexpr std::int32_t kImmMin = -(1 << (kImmBits - 1));       // -131072
inline constexpr std::int32_t kImmMax = (1 << (kImmBits - 1)) - 1;    //  131071

/// Encode a decoded instruction into its 32-bit machine word.
/// Throws EncodingError if any field is out of range.
std::uint32_t encode(const Instruction& inst);

/// Decode a 32-bit machine word into an Instruction.
/// Throws DecodingError if the opcode is unknown.
Instruction decode(std::uint32_t word);

}  // namespace plab::isa

#endif  // PLAB_ISA_ENCODING_HPP
