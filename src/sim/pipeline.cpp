#include "plab/sim/pipeline.hpp"

#include "plab/isa/encoding.hpp"
#include "plab/isa/registers.hpp"
#include "plab/sim/alu.hpp"

namespace plab::sim {

using isa::Instruction;
using isa::Opcode;

namespace {

std::uint32_t addOffset(std::uint32_t base, std::int32_t offset) {
  return static_cast<std::uint32_t>(static_cast<std::int64_t>(base) + offset);
}

// A source port reads a forwardable register only if it names a real,
// non-zero register (r0 is always zero and never a hazard target).
bool readsReg(std::uint8_t r) { return r != kNoReg && r != isa::kZeroReg; }

}  // namespace

Control makeControl(const Instruction& inst) {
  Control c;
  const Opcode op = inst.op;
  switch (isa::formatOf(op)) {
    case isa::Format::R:
      c.regWrite = true;
      c.destReg = inst.rd;
      c.aReg = inst.rs;
      c.bReg = inst.rt;
      break;
    case isa::Format::I:
      c.aluUsesImm = true;
      c.aReg = inst.rs;
      if (isa::isLoad(op)) {
        c.memRead = true;
        c.regWrite = true;
        c.destReg = inst.rd;
      } else if (isa::isStore(op)) {
        c.memWrite = true;
        c.bReg = inst.rd;  // store-value source lives in the rd slot
      } else {
        c.regWrite = true;
        c.destReg = inst.rd;
      }
      break;
    case isa::Format::B:
      c.isBranch = true;
      c.aReg = inst.rs;
      c.bReg = inst.rt;
      break;
    case isa::Format::J:
      if (op == Opcode::RET) {
        c.isRet = true;
      } else {
        c.isJump = true;
        if (op == Opcode::JAL) {
          c.isJal = true;
          c.regWrite = true;
          c.destReg = isa::kReturnAddrReg;
        }
      }
      break;
  }
  return c;
}

std::int32_t Pipeline::forwardValue(std::uint8_t reg, std::int32_t fallback) {
  if (!forwarding_ || !readsReg(reg)) return fallback;

  // EX/MEM has priority (most recent value). Loads forward from MEM/WB only,
  // since their data isn't ready until after the MEM stage.
  if (!exmem_.bubble && exmem_.ctrl.regWrite && exmem_.ctrl.destReg != 0 &&
      !exmem_.ctrl.memRead && exmem_.ctrl.destReg == reg) {
    ++stats_.forwards;
    return exmem_.aluResult;
  }
  if (!memwb_.bubble && memwb_.ctrl.regWrite && memwb_.ctrl.destReg != 0 &&
      memwb_.ctrl.destReg == reg) {
    ++stats_.forwards;
    return memwb_.writeData;
  }
  return fallback;
}

bool Pipeline::detectStall() const {
  if (ifid_.bubble) return false;
  const Control cc = makeControl(ifid_.inst);  // consumer's source registers

  const auto conflicts = [&](const Latch& producer) {
    if (producer.bubble || !producer.ctrl.regWrite || producer.ctrl.destReg == 0) {
      return false;
    }
    const std::uint8_t d = producer.ctrl.destReg;
    return (readsReg(cc.aReg) && cc.aReg == d) ||
           (readsReg(cc.bReg) && cc.bReg == d);
  };

  if (forwarding_) {
    // With forwarding, only a load feeding the very next instruction stalls.
    return !idex_.bubble && idex_.ctrl.memRead && conflicts(idex_);
  }
  // Without forwarding, stall while any in-flight producer (EX or MEM) has yet
  // to write back a register this instruction needs.
  return conflicts(idex_) || conflicts(exmem_);
}

void Pipeline::writeBack() {
  if (!memwb_.bubble && memwb_.ctrl.regWrite) {
    regs_.write(memwb_.ctrl.destReg, memwb_.writeData);
  }
}

void Pipeline::memoryStage(Latch& next) {
  if (exmem_.bubble) {
    next = Latch{};
    return;
  }
  next = exmem_;
  const std::uint32_t addr = static_cast<std::uint32_t>(exmem_.aluResult);
  if (exmem_.ctrl.memRead) {
    next.writeData = mem_.loadWord(addr);
  } else {
    next.writeData = exmem_.aluResult;  // ALU result or jal link value
  }
  if (exmem_.ctrl.memWrite) {
    mem_.storeWord(addr, exmem_.storeData);
  }
}

void Pipeline::execute(Latch& next, bool& flushYounger, bool& countFlush,
                       bool& stopFetch, std::uint32_t& newPc) {
  if (idex_.bubble) {
    next = Latch{};
    return;
  }
  ++stats_.retired;  // a real instruction commits when it clears EX

  const Control& c = idex_.ctrl;
  const Instruction& inst = idex_.inst;
  const std::int32_t aVal = forwardValue(c.aReg, idex_.aVal);
  const std::int32_t bVal = forwardValue(c.bReg, idex_.bVal);

  next = idex_;  // real instructions flow through MEM/WB (doing nothing if idle)
  next.bubble = false;

  if (c.isRet) {
    flushYounger = true;
    stopFetch = true;
    return;
  }
  if (c.isJump) {
    flushYounger = true;
    countFlush = true;
    newPc = inst.target;
    if (c.isJal) next.aluResult = static_cast<std::int32_t>(idex_.pc + 4);
    return;
  }
  if (c.isBranch) {
    if (branchTaken(inst.op, aVal, bVal)) {
      flushYounger = true;
      countFlush = true;
      newPc = addOffset(idex_.pc, inst.imm);
    }
    return;
  }
  if (c.memRead || c.memWrite) {
    next.aluResult =
        static_cast<std::int32_t>(static_cast<std::int64_t>(aVal) + inst.imm);
    next.storeData = bVal;
    return;
  }
  next.aluResult = aluResult(inst.op, aVal, c.aluUsesImm ? inst.imm : bVal);
}

void Pipeline::decode(Latch& next) {
  if (ifid_.bubble) {
    next = Latch{};
    return;
  }
  next = Latch{};
  next.bubble = false;
  next.id = ifid_.id;
  next.inst = ifid_.inst;
  next.pc = ifid_.pc;
  next.ctrl = makeControl(ifid_.inst);
  next.aVal = next.ctrl.aReg == kNoReg ? 0 : regs_.read(next.ctrl.aReg);
  next.bVal = next.ctrl.bReg == kNoReg ? 0 : regs_.read(next.ctrl.bReg);
}

void Pipeline::fetchInto(Latch& next) {
  const std::uint32_t codeBytes =
      static_cast<std::uint32_t>(program_.words.size()) * asmr::kInstructionBytes;
  if (fetchStopped_ || pc_ >= codeBytes) {
    next = Latch{};
    return;
  }
  const std::size_t index = pc_ / asmr::kInstructionBytes;
  const Instruction inst = isa::decode(program_.words[index]);
  next = Latch{};
  next.bubble = false;
  next.id = nextId_;
  next.inst = inst;
  next.pc = pc_;
  traced_.push_back(TracedInstr{pc_, program_.sourceLines[index], isa::disassemble(inst)});
  ++nextId_;
}

void Pipeline::recordStages(bool fetching, int fetchId) {
  if (fetching) trace_.push_back(TraceEvent{fetchId, cycle_, Stage::IF});
  if (!ifid_.bubble) trace_.push_back(TraceEvent{ifid_.id, cycle_, Stage::ID});
  if (!idex_.bubble) trace_.push_back(TraceEvent{idex_.id, cycle_, Stage::EX});
  if (!exmem_.bubble) trace_.push_back(TraceEvent{exmem_.id, cycle_, Stage::MEM});
  if (!memwb_.bubble) trace_.push_back(TraceEvent{memwb_.id, cycle_, Stage::WB});
}

const PipelineStats& Pipeline::run(std::uint64_t maxCycles) {
  const std::uint32_t codeBytes =
      static_cast<std::uint32_t>(program_.words.size()) * asmr::kInstructionBytes;

  while (true) {
    // Drained: nothing left to fetch and every stage empty.
    if (fetchStopped_ && ifid_.bubble && idex_.bubble && exmem_.bubble &&
        memwb_.bubble) {
      break;
    }
    if (cycle_ >= maxCycles) {
      throw ExecError("pipeline exceeded cycle limit (" + std::to_string(maxCycles) +
                      "); possible infinite loop");
    }
    ++cycle_;
    stats_.cycles = cycle_;

    const bool stall = detectStall();
    const bool willFetch = !stall && !fetchStopped_ && pc_ < codeBytes;
    const int fetchId = willFetch ? nextId_ : -1;
    recordStages(willFetch, fetchId);

    Latch nIfid, nIdex, nExmem, nMemwb;

    writeBack();
    memoryStage(nMemwb);

    bool flushYounger = false, countFlush = false, stopFetch = false;
    std::uint32_t newPc = 0;
    execute(nExmem, flushYounger, countFlush, stopFetch, newPc);

    decode(nIdex);
    if (!stall) fetchInto(nIfid);

    if (stall) {
      ++stats_.stallCycles;
      nIdex = Latch{};   // bubble into EX
      nIfid = ifid_;     // hold the instruction in ID; pc stays frozen
    } else {
      if (willFetch) pc_ += asmr::kInstructionBytes;
      if (flushYounger) {
        nIdex = Latch{};
        nIfid = Latch{};
        if (stopFetch) {
          fetchStopped_ = true;
        } else {
          pc_ = newPc;
        }
        if (countFlush) {
          ++stats_.flushes;
          stats_.flushBubbles += 2;
        }
      }
    }

    ifid_ = nIfid;
    idex_ = nIdex;
    exmem_ = nExmem;
    memwb_ = nMemwb;
  }

  return stats_;
}

}  // namespace plab::sim
