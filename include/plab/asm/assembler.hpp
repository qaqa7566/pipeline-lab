#ifndef PLAB_ASM_ASSEMBLER_HPP
#define PLAB_ASM_ASSEMBLER_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "plab/asm/parser.hpp"
#include "plab/isa/instruction.hpp"

namespace plab::asmr {

/// Thrown on a semantic assembly error (unknown mnemonic, wrong operand shape,
/// undefined/duplicate label, out-of-range field). Carries the 1-based line.
class AssembleError : public std::runtime_error {
 public:
  AssembleError(const std::string& what, int line)
      : std::runtime_error(what), line_(line) {}
  int line() const noexcept { return line_; }

 private:
  int line_;
};

/// Bytes occupied by one instruction (fixed-width ISA).
inline constexpr std::uint32_t kInstructionBytes = 4;

/// The assembled program: a machine-code image plus metadata for the
/// interpreter, pipeline, and visualizer.
struct Program {
  std::vector<std::uint32_t> words;           ///< 32-bit machine words
  std::vector<isa::Instruction> instructions; ///< decoded, parallel to `words`
  std::vector<int> sourceLines;               ///< source line per word (parallel)
  std::unordered_map<std::string, std::uint32_t> symbols;  ///< label -> byte addr

  std::size_t size() const noexcept { return words.size(); }
};

/// Assembles source text into a Program. Runs lex -> parse -> two-pass assemble
/// (pass 1 assigns addresses and builds the symbol table; pass 2 expands
/// pseudo-instructions, resolves labels, and encodes each word).
Program assemble(std::string_view source);

/// Assembles an already-parsed statement list (used by tests).
Program assemble(const std::vector<Statement>& statements);

}  // namespace plab::asmr

#endif  // PLAB_ASM_ASSEMBLER_HPP
