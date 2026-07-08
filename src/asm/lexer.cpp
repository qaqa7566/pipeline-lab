#include "plab/asm/lexer.hpp"

#include <cctype>
#include <string>

namespace plab::asmr {

namespace {

bool isIdentStart(char c) {
  return std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '.';
}
bool isIdentChar(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.';
}

// True if `text` is a register name rN with N a non-negative decimal integer.
bool isRegisterName(const std::string& text, std::int64_t& index) {
  if (text.size() < 2 || text[0] != 'r') return false;
  std::int64_t v = 0;
  for (std::size_t i = 1; i < text.size(); ++i) {
    if (!std::isdigit(static_cast<unsigned char>(text[i]))) return false;
    v = v * 10 + (text[i] - '0');
  }
  index = v;
  return true;
}

}  // namespace

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;
  int line = 1;
  int col = 1;
  const std::size_t n = src_.size();
  std::size_t i = 0;

  auto push = [&](TokenKind kind, std::string text, std::int64_t value, int c) {
    tokens.push_back(Token{kind, std::move(text), value, line, c});
  };

  while (i < n) {
    const char c = src_[i];

    if (c == '\n') {
      push(TokenKind::Newline, "\\n", 0, col);
      ++i;
      ++line;
      col = 1;
      continue;
    }
    if (c == ' ' || c == '\t' || c == '\r') {
      ++i;
      ++col;
      continue;
    }
    if (c == ';') {  // comment to end of line
      while (i < n && src_[i] != '\n') {
        ++i;
        ++col;
      }
      continue;
    }

    const int startCol = col;
    switch (c) {
      case ',': push(TokenKind::Comma, ",", 0, startCol); ++i; ++col; continue;
      case ':': push(TokenKind::Colon, ":", 0, startCol); ++i; ++col; continue;
      case '(': push(TokenKind::LParen, "(", 0, startCol); ++i; ++col; continue;
      case ')': push(TokenKind::RParen, ")", 0, startCol); ++i; ++col; continue;
      default: break;
    }

    // Signed number: a sign only starts a number if a digit follows.
    const bool signedNum =
        (c == '-' || c == '+') && i + 1 < n &&
        std::isdigit(static_cast<unsigned char>(src_[i + 1]));
    if (std::isdigit(static_cast<unsigned char>(c)) || signedNum) {
      const std::size_t begin = i;
      if (c == '-' || c == '+') {
        ++i;
        ++col;
      }
      int base = 10;
      if (i + 1 < n && src_[i] == '0' && (src_[i + 1] == 'x' || src_[i + 1] == 'X')) {
        base = 16;
        i += 2;
        col += 2;
      }
      while (i < n && std::isalnum(static_cast<unsigned char>(src_[i]))) {
        ++i;
        ++col;
      }
      const std::string text{src_.substr(begin, i - begin)};
      try {
        std::size_t consumed = 0;
        const long long v = std::stoll(text, &consumed, base == 16 ? 16 : 10);
        if (consumed != text.size()) throw std::invalid_argument(text);
        push(TokenKind::Number, text, static_cast<std::int64_t>(v), startCol);
      } catch (const std::exception&) {
        throw LexError("invalid number literal '" + text + "'", line);
      }
      continue;
    }

    if (isIdentStart(c)) {
      const std::size_t begin = i;
      while (i < n && isIdentChar(src_[i])) {
        ++i;
        ++col;
      }
      std::string text{src_.substr(begin, i - begin)};
      std::int64_t regIndex = 0;
      if (isRegisterName(text, regIndex)) {
        push(TokenKind::Register, std::move(text), regIndex, startCol);
      } else {
        push(TokenKind::Identifier, std::move(text), 0, startCol);
      }
      continue;
    }

    throw LexError(std::string("unexpected character '") + c + "'", line);
  }

  tokens.push_back(Token{TokenKind::End, "", 0, line, col});
  return tokens;
}

}  // namespace plab::asmr
