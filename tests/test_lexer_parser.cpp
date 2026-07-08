#include "plab/asm/lexer.hpp"
#include "plab/asm/parser.hpp"

#include <gtest/gtest.h>

#include <vector>

using namespace plab::asmr;

namespace {

std::vector<Token> lex(std::string_view src) { return Lexer(src).tokenize(); }
std::vector<Statement> parse(std::string_view src) {
  return Parser(Lexer(src).tokenize()).parse();
}

}  // namespace

TEST(Lexer, ClassifiesRegistersAndNumbers) {
  const auto toks = lex("add r1, r2, 0x10");
  ASSERT_GE(toks.size(), 6u);
  EXPECT_EQ(toks[0].kind, TokenKind::Identifier);
  EXPECT_EQ(toks[1].kind, TokenKind::Register);
  EXPECT_EQ(toks[1].value, 1);
  EXPECT_EQ(toks[2].kind, TokenKind::Comma);
  EXPECT_EQ(toks[3].kind, TokenKind::Register);
  EXPECT_EQ(toks[5].kind, TokenKind::Number);
  EXPECT_EQ(toks[5].value, 16);
}

TEST(Lexer, HandlesNegativeAndHexNumbers) {
  const auto toks = lex("-5 +7 0xFF");
  EXPECT_EQ(toks[0].value, -5);
  EXPECT_EQ(toks[1].value, 7);
  EXPECT_EQ(toks[2].value, 255);
}

TEST(Lexer, StripsCommentsAndTracksLines) {
  const auto toks = lex("add r1, r0, r0 ; a comment\nsub r2, r1, r0");
  // First line has no comment tokens; a newline separates the two statements.
  bool sawNewline = false;
  for (const auto& t : toks) {
    EXPECT_NE(t.text, ";");
    if (t.kind == TokenKind::Newline) sawNewline = true;
  }
  EXPECT_TRUE(sawNewline);
  EXPECT_EQ(toks.back().kind, TokenKind::End);
}

TEST(Lexer, RejectsBadNumber) {
  EXPECT_THROW(lex("addi r1, r0, 0xZZ"), LexError);
}

TEST(Lexer, RejectsUnexpectedCharacter) {
  EXPECT_THROW(lex("add r1, r0, @"), LexError);
}

TEST(Parser, ParsesLabelThenInstruction) {
  const auto stmts = parse("loop: add r1, r2, r3");
  ASSERT_EQ(stmts.size(), 2u);
  EXPECT_EQ(stmts[0].kind, Statement::Kind::Label);
  EXPECT_EQ(stmts[0].name, "loop");
  EXPECT_EQ(stmts[1].kind, Statement::Kind::Instruction);
  EXPECT_EQ(stmts[1].name, "add");
  ASSERT_EQ(stmts[1].operands.size(), 3u);
  EXPECT_EQ(stmts[1].operands[0].kind, Operand::Kind::Register);
  EXPECT_EQ(stmts[1].operands[0].reg, 1);
}

TEST(Parser, ParsesMemoryOperand) {
  const auto stmts = parse("lw r1, 8(r2)");
  ASSERT_EQ(stmts.size(), 1u);
  const auto& mem = stmts[0].operands[1];
  EXPECT_EQ(mem.kind, Operand::Kind::Memory);
  EXPECT_EQ(mem.imm, 8);
  EXPECT_EQ(mem.reg, 2);
}

TEST(Parser, ParsesImplicitZeroDisplacement) {
  const auto stmts = parse("sw r1, (r2)");
  const auto& mem = stmts[0].operands[1];
  EXPECT_EQ(mem.kind, Operand::Kind::Memory);
  EXPECT_EQ(mem.imm, 0);
  EXPECT_EQ(mem.reg, 2);
}

TEST(Parser, LowercasesMnemonic) {
  const auto stmts = parse("ADD r1, r2, r3");
  EXPECT_EQ(stmts[0].name, "add");
}

TEST(Parser, RejectsRegisterOutOfRange) {
  EXPECT_THROW(parse("add r16, r0, r0"), ParseError);
}

TEST(Parser, RejectsTrailingGarbage) {
  EXPECT_THROW(parse("add r1, r2, r3 r4"), ParseError);
}
