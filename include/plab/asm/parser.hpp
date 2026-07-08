#ifndef PLAB_ASM_PARSER_HPP
#define PLAB_ASM_PARSER_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "plab/asm/lexer.hpp"

namespace plab::asmr {

/// Thrown on a syntactically invalid statement. Carries the 1-based line.
class ParseError : public std::runtime_error {
 public:
  ParseError(const std::string& what, int line)
      : std::runtime_error(what), line_(line) {}
  int line() const noexcept { return line_; }

 private:
  int line_;
};

/// A single instruction operand in its symbolic (pre-assembly) form.
struct Operand {
  enum class Kind {
    Register,    ///< rN
    Immediate,   ///< integer literal
    Label,       ///< symbolic reference resolved by the assembler
    Memory,      ///< disp(base): `imm` is the displacement, `reg` the base
  };
  Kind kind{Kind::Register};
  std::uint8_t reg{0};
  std::int64_t imm{0};
  std::string label;
  int line{0};
};

/// A parsed statement: either a label definition or an instruction.
struct Statement {
  enum class Kind { Label, Instruction };
  Kind kind{Kind::Instruction};
  std::string name;               ///< label name, or mnemonic (lowercased)
  std::vector<Operand> operands;  ///< instruction operands
  int line{0};
};

/// Parses a token stream into an ordered list of statements. Performs
/// syntactic validation and register-range checks, but does not resolve
/// labels or select opcodes (the assembler does that).
class Parser {
 public:
  explicit Parser(std::vector<Token> tokens) : toks_(std::move(tokens)) {}

  std::vector<Statement> parse();

 private:
  const Token& peek() const { return toks_[pos_]; }
  const Token& advance() { return toks_[pos_++]; }
  bool check(TokenKind k) const { return peek().kind == k; }
  const Token& expect(TokenKind k, const char* what);
  Operand parseOperand();

  std::vector<Token> toks_;
  std::size_t pos_{0};
};

}  // namespace plab::asmr

#endif  // PLAB_ASM_PARSER_HPP
