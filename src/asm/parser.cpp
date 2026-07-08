#include "plab/asm/parser.hpp"

#include <algorithm>
#include <cctype>

#include "plab/isa/registers.hpp"

namespace plab::asmr {

namespace {

std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

}  // namespace

const Token& Parser::expect(TokenKind k, const char* what) {
  if (!check(k)) {
    throw ParseError(std::string("expected ") + what + " but found '" +
                         peek().text + "'",
                     peek().line);
  }
  return advance();
}

Operand Parser::parseOperand() {
  const Token& tok = peek();
  Operand op;
  op.line = tok.line;

  switch (tok.kind) {
    case TokenKind::Register: {
      advance();
      if (tok.value < 0 || tok.value >= isa::kNumRegisters) {
        throw ParseError("register out of range: r" + std::to_string(tok.value),
                         tok.line);
      }
      op.kind = Operand::Kind::Register;
      op.reg = static_cast<std::uint8_t>(tok.value);
      return op;
    }
    case TokenKind::Identifier: {
      advance();
      op.kind = Operand::Kind::Label;
      op.label = tok.text;
      return op;
    }
    case TokenKind::LParen: {  // (base) with implicit zero displacement
      advance();
      const Token& base = expect(TokenKind::Register, "register");
      expect(TokenKind::RParen, "')'");
      if (base.value < 0 || base.value >= isa::kNumRegisters) {
        throw ParseError("register out of range: r" + std::to_string(base.value),
                         base.line);
      }
      op.kind = Operand::Kind::Memory;
      op.imm = 0;
      op.reg = static_cast<std::uint8_t>(base.value);
      return op;
    }
    case TokenKind::Number: {
      advance();
      if (check(TokenKind::LParen)) {  // disp(base)
        advance();
        const Token& base = expect(TokenKind::Register, "register");
        expect(TokenKind::RParen, "')'");
        if (base.value < 0 || base.value >= isa::kNumRegisters) {
          throw ParseError("register out of range: r" + std::to_string(base.value),
                           base.line);
        }
        op.kind = Operand::Kind::Memory;
        op.imm = tok.value;
        op.reg = static_cast<std::uint8_t>(base.value);
        return op;
      }
      op.kind = Operand::Kind::Immediate;
      op.imm = tok.value;
      return op;
    }
    default:
      throw ParseError("expected operand but found '" + tok.text + "'", tok.line);
  }
}

std::vector<Statement> Parser::parse() {
  std::vector<Statement> stmts;

  while (!check(TokenKind::End)) {
    if (check(TokenKind::Newline)) {
      advance();
      continue;
    }

    const Token& head = expect(TokenKind::Identifier, "label or mnemonic");

    if (check(TokenKind::Colon)) {  // label definition
      advance();
      Statement s;
      s.kind = Statement::Kind::Label;
      s.name = head.text;
      s.line = head.line;
      stmts.push_back(std::move(s));
      continue;
    }

    // Instruction: mnemonic followed by comma-separated operands.
    Statement s;
    s.kind = Statement::Kind::Instruction;
    s.name = toLower(head.text);
    s.line = head.line;

    if (!check(TokenKind::Newline) && !check(TokenKind::End)) {
      s.operands.push_back(parseOperand());
      while (check(TokenKind::Comma)) {
        advance();
        s.operands.push_back(parseOperand());
      }
    }

    if (!check(TokenKind::Newline) && !check(TokenKind::End)) {
      throw ParseError("unexpected token after operands: '" + peek().text + "'",
                       peek().line);
    }
    stmts.push_back(std::move(s));
  }

  return stmts;
}

}  // namespace plab::asmr
