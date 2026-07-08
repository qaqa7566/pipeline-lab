#include "plab/asm/assembler.hpp"

#include <optional>

#include "plab/asm/lexer.hpp"
#include "plab/isa/encoding.hpp"
#include "plab/isa/registers.hpp"

namespace plab::asmr {

using isa::Instruction;
using isa::Opcode;

namespace {

// Maps a real mnemonic to its opcode. Pseudo-instructions (nop/mov/li) are
// handled separately and are not in this table.
std::optional<Opcode> lookupOpcode(const std::string& m) {
  static const std::unordered_map<std::string, Opcode> table = {
      {"add", Opcode::ADD},   {"sub", Opcode::SUB},   {"and", Opcode::AND},
      {"or", Opcode::OR},     {"xor", Opcode::XOR},   {"sll", Opcode::SLL},
      {"srl", Opcode::SRL},   {"slt", Opcode::SLT},   {"addi", Opcode::ADDI},
      {"andi", Opcode::ANDI}, {"ori", Opcode::ORI},   {"xori", Opcode::XORI},
      {"slti", Opcode::SLTI}, {"lw", Opcode::LW},     {"sw", Opcode::SW},
      {"beq", Opcode::BEQ},   {"bne", Opcode::BNE},   {"blt", Opcode::BLT},
      {"j", Opcode::J},       {"jal", Opcode::JAL},   {"ret", Opcode::RET},
      {"halt", Opcode::RET},
  };
  auto it = table.find(m);
  return it == table.end() ? std::nullopt : std::optional<Opcode>(it->second);
}

// Per-statement translator: turns one parsed instruction statement into an
// encoded ISA instruction, resolving labels against `symbols`. `address` is the
// byte address of this instruction (needed for PC-relative branch offsets).
class Translator {
 public:
  Translator(const std::unordered_map<std::string, std::uint32_t>& symbols)
      : symbols_(symbols) {}

  Instruction translate(const Statement& s, std::uint32_t address) {
    line_ = s.line;
    const std::string& m = s.name;

    if (m == "nop") return makeNop();
    if (m == "mov") return makeMov(s);
    if (m == "li") return makeLi(s);

    const auto op = lookupOpcode(m);
    if (!op) fail("unknown instruction '" + m + "'");

    switch (isa::formatOf(*op)) {
      case isa::Format::R: return makeR(*op, s);
      case isa::Format::I:
        return isa::isLoad(*op) || isa::isStore(*op) ? makeMem(*op, s)
                                                     : makeIAlu(*op, s);
      case isa::Format::B: return makeBranch(*op, s, address);
      case isa::Format::J: return makeJump(*op, s);
    }
    fail("internal: unhandled format");
  }

 private:
  [[noreturn]] void fail(const std::string& msg) { throw AssembleError(msg, line_); }

  void expectArgc(const Statement& s, std::size_t n) {
    if (s.operands.size() != n) {
      fail(s.name + " expects " + std::to_string(n) + " operand(s), got " +
           std::to_string(s.operands.size()));
    }
  }

  std::uint8_t reg(const Operand& op) {
    if (op.kind != Operand::Kind::Register) fail("expected a register operand");
    return op.reg;
  }

  std::int32_t imm(const Operand& op) {
    if (op.kind != Operand::Kind::Immediate) fail("expected an immediate operand");
    if (op.imm < isa::kImmMin || op.imm > isa::kImmMax) {
      fail("immediate out of range: " + std::to_string(op.imm));
    }
    return static_cast<std::int32_t>(op.imm);
  }

  std::uint32_t resolveLabel(const std::string& name) {
    auto it = symbols_.find(name);
    if (it == symbols_.end()) fail("undefined label '" + name + "'");
    return it->second;
  }

  Instruction makeNop() {
    return Instruction{Opcode::ADD, 0, 0, 0, 0, 0};  // add r0, r0, r0
  }

  Instruction makeMov(const Statement& s) {
    expectArgc(s, 2);  // mov rd, rs  ->  add rd, rs, r0
    return Instruction{Opcode::ADD, reg(s.operands[0]), reg(s.operands[1]), 0, 0, 0};
  }

  Instruction makeLi(const Statement& s) {
    expectArgc(s, 2);  // li rd, imm  ->  addi rd, r0, imm
    Instruction inst{Opcode::ADDI, reg(s.operands[0]), 0, 0, 0, 0};
    inst.imm = imm(s.operands[1]);
    return inst;
  }

  Instruction makeR(Opcode op, const Statement& s) {
    expectArgc(s, 3);  // op rd, rs, rt
    return Instruction{op, reg(s.operands[0]), reg(s.operands[1]),
                       reg(s.operands[2]), 0, 0};
  }

  Instruction makeIAlu(Opcode op, const Statement& s) {
    expectArgc(s, 3);  // op rd, rs, imm
    Instruction inst{op, reg(s.operands[0]), reg(s.operands[1]), 0, 0, 0};
    inst.imm = imm(s.operands[2]);
    return inst;
  }

  Instruction makeMem(Opcode op, const Statement& s) {
    expectArgc(s, 2);  // lw rd, disp(base) ; sw rval, disp(base)
    const std::uint8_t valueOrDest = reg(s.operands[0]);
    const Operand& mem = s.operands[1];
    if (mem.kind != Operand::Kind::Memory) fail("expected disp(base) memory operand");
    if (mem.imm < isa::kImmMin || mem.imm > isa::kImmMax) {
      fail("displacement out of range: " + std::to_string(mem.imm));
    }
    // For both lw and sw the rd slot holds the load destination / store source;
    // rs is the base register.
    Instruction inst{op, valueOrDest, mem.reg, 0, 0, 0};
    inst.imm = static_cast<std::int32_t>(mem.imm);
    return inst;
  }

  Instruction makeBranch(Opcode op, const Statement& s, std::uint32_t address) {
    expectArgc(s, 3);  // beq rs, rt, target
    Instruction inst{op, 0, reg(s.operands[0]), reg(s.operands[1]), 0, 0};
    const Operand& tgt = s.operands[2];
    std::int64_t offset = 0;
    if (tgt.kind == Operand::Kind::Label) {
      offset = static_cast<std::int64_t>(resolveLabel(tgt.label)) -
               static_cast<std::int64_t>(address);
    } else if (tgt.kind == Operand::Kind::Immediate) {
      offset = tgt.imm;  // explicit byte offset
    } else {
      fail("branch target must be a label or immediate offset");
    }
    if (offset < isa::kImmMin || offset > isa::kImmMax) {
      fail("branch target too far: offset " + std::to_string(offset));
    }
    inst.imm = static_cast<std::int32_t>(offset);
    return inst;
  }

  Instruction makeJump(Opcode op, const Statement& s) {
    Instruction inst{op, 0, 0, 0, 0, 0};
    if (op == Opcode::RET) {
      expectArgc(s, 0);
      return inst;
    }
    expectArgc(s, 1);  // j/jal target
    const Operand& tgt = s.operands[0];
    if (tgt.kind == Operand::Kind::Label) {
      inst.target = resolveLabel(tgt.label);
    } else if (tgt.kind == Operand::Kind::Immediate) {
      if (tgt.imm < 0) fail("jump target must be non-negative");
      inst.target = static_cast<std::uint32_t>(tgt.imm);
    } else {
      fail("jump target must be a label or address");
    }
    return inst;
  }

  const std::unordered_map<std::string, std::uint32_t>& symbols_;
  int line_{0};
};

}  // namespace

Program assemble(const std::vector<Statement>& statements) {
  Program prog;

  // Pass 1: assign addresses and build the symbol table.
  std::uint32_t address = 0;
  for (const Statement& s : statements) {
    if (s.kind == Statement::Kind::Label) {
      if (prog.symbols.count(s.name)) {
        throw AssembleError("duplicate label '" + s.name + "'", s.line);
      }
      prog.symbols.emplace(s.name, address);
    } else {
      address += kInstructionBytes;
    }
  }

  // Pass 2: translate + encode.
  Translator translator(prog.symbols);
  address = 0;
  for (const Statement& s : statements) {
    if (s.kind == Statement::Kind::Label) continue;
    const Instruction inst = translator.translate(s, address);
    std::uint32_t word = 0;
    try {
      word = isa::encode(inst);
    } catch (const isa::EncodingError& e) {
      throw AssembleError(std::string("encoding error: ") + e.what(), s.line);
    }
    prog.words.push_back(word);
    prog.instructions.push_back(inst);
    prog.sourceLines.push_back(s.line);
    address += kInstructionBytes;
  }

  return prog;
}

Program assemble(std::string_view source) {
  Lexer lexer(source);
  Parser parser(lexer.tokenize());
  return assemble(parser.parse());
}

}  // namespace plab::asmr
