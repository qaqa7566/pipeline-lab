#ifndef PLAB_ASM_LEXER_HPP
#define PLAB_ASM_LEXER_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace plab::asmr {

/// Thrown on an invalid character or malformed token. Carries the 1-based
/// source line so the assembler can report a useful location.
class LexError : public std::runtime_error {
 public:
  LexError(const std::string& what, int line)
      : std::runtime_error(what), line_(line) {}
  int line() const noexcept { return line_; }

 private:
  int line_;
};

enum class TokenKind {
  Identifier,  ///< mnemonic or label name (e.g. "add", "loop")
  Register,    ///< rN; `value` holds N
  Number,      ///< decimal or 0x-hex literal; `value` holds the parsed integer
  Comma,
  Colon,
  LParen,
  RParen,
  Newline,     ///< statement separator
  End,         ///< end of input
};

struct Token {
  TokenKind kind{TokenKind::End};
  std::string text;        ///< source spelling (identifiers/registers)
  std::int64_t value{0};   ///< numeric value for Number / register index for Register
  int line{1};
  int column{1};
};

/// Converts assembly source into a flat token stream terminated by an End
/// token. Comments (`;` to end of line) are stripped; each physical line ends
/// with a Newline token.
class Lexer {
 public:
  explicit Lexer(std::string_view source) : src_(source) {}

  std::vector<Token> tokenize();

 private:
  std::string_view src_;
};

}  // namespace plab::asmr

#endif  // PLAB_ASM_LEXER_HPP
