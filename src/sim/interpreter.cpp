#include "plab/sim/interpreter.hpp"

#include "plab/isa/encoding.hpp"
#include "plab/isa/instruction.hpp"
#include "plab/isa/registers.hpp"
#include "plab/sim/alu.hpp"

namespace plab::sim {

using isa::Instruction;
using isa::Opcode;

namespace {

// Advance PC by a signed byte offset, wrapping in unsigned space.
std::uint32_t addOffset(std::uint32_t pc, std::int32_t offset) {
  return static_cast<std::uint32_t>(static_cast<std::int64_t>(pc) + offset);
}

}  // namespace

std::uint64_t Interpreter::run(std::uint64_t maxSteps) {
  const std::uint32_t codeBytes =
      static_cast<std::uint32_t>(program_.words.size()) * asmr::kInstructionBytes;

  for (std::uint64_t step = 0; step < maxSteps; ++step) {
    if (pc_ >= codeBytes) {
      throw ExecError("PC out of range: " + std::to_string(pc_));
    }

    const Instruction inst = isa::decode(program_.words[pc_ / asmr::kInstructionBytes]);
    ++retired_;

    switch (isa::formatOf(inst.op)) {
      case isa::Format::R:
        regs_.write(inst.rd, aluResult(inst.op, regs_.read(inst.rs), regs_.read(inst.rt)));
        pc_ += asmr::kInstructionBytes;
        break;

      case isa::Format::I:
        if (inst.op == Opcode::LW) {
          const std::uint32_t addr =
              addOffset(static_cast<std::uint32_t>(regs_.read(inst.rs)), inst.imm);
          regs_.write(inst.rd, mem_.loadWord(addr));
        } else if (inst.op == Opcode::SW) {
          const std::uint32_t addr =
              addOffset(static_cast<std::uint32_t>(regs_.read(inst.rs)), inst.imm);
          mem_.storeWord(addr, regs_.read(inst.rd));  // rd slot holds store value
        } else {
          regs_.write(inst.rd, aluResult(inst.op, regs_.read(inst.rs), inst.imm));
        }
        pc_ += asmr::kInstructionBytes;
        break;

      case isa::Format::B:
        if (branchTaken(inst.op, regs_.read(inst.rs), regs_.read(inst.rt))) {
          pc_ = addOffset(pc_, inst.imm);
        } else {
          pc_ += asmr::kInstructionBytes;
        }
        break;

      case isa::Format::J:
        if (inst.op == Opcode::RET) {
          return retired_;
        }
        if (inst.op == Opcode::JAL) {
          regs_.write(isa::kReturnAddrReg,
                      static_cast<std::int32_t>(pc_ + asmr::kInstructionBytes));
        }
        pc_ = inst.target;
        break;
    }
  }

  throw ExecError("execution exceeded step limit (" + std::to_string(maxSteps) +
                  "); possible infinite loop");
}

}  // namespace plab::sim
