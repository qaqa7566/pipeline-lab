// plab: command-line driver for the Pipeline Lab toolchain.
//
//   plab <file.asm> [options]
//
// Assembles a program and runs it on the sequential interpreter and/or the
// 5-stage pipeline simulator, printing a cycle-by-cycle diagram, a performance
// report, and final architectural state.
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "plab/asm/assembler.hpp"
#include "plab/asm/lexer.hpp"
#include "plab/asm/parser.hpp"
#include "plab/sim/interpreter.hpp"
#include "plab/sim/pipeline.hpp"
#include "plab/viz/trace.hpp"

namespace {

struct Options {
  std::string path;
  bool runInterp = false;
  bool runPipeline = false;
  bool showTrace = false;
  bool showStats = false;
  bool showRegs = false;
  bool forwarding = true;
  int memWords = 0;
};

// Parses a --mem word count: a fully-numeric, non-negative value that fits in
// an int. Returns false (rather than throwing) on garbage or overflow so the
// caller can report a clean error instead of aborting.
bool parseMemCount(const std::string& s, int& out) {
  const char* begin = s.data();
  const char* end = begin + s.size();
  int value = 0;
  const auto [ptr, ec] = std::from_chars(begin, end, value);
  if (ec != std::errc() || ptr != end || value < 0) return false;
  out = value;
  return true;
}

void printUsage(const char* prog) {
  std::cout <<
      "Usage: " << prog << " <file.asm> [options]\n"
      "\n"
      "Modes (default: --pipeline --trace --stats --regs):\n"
      "  --pipeline      run the 5-stage pipeline simulator\n"
      "  --interp        run the sequential reference interpreter\n"
      "\n"
      "Output:\n"
      "  --trace         print the cycle-by-cycle pipeline diagram\n"
      "  --stats         print the performance report\n"
      "  --regs          print the final register file\n"
      "  --mem N         print the first N words of data memory\n"
      "\n"
      "Pipeline options:\n"
      "  --no-forward    disable operand forwarding (more stalls)\n"
      "  -h, --help      show this help\n";
}

void printRegisters(const plab::sim::RegisterFile& regs) {
  const auto& r = regs.raw();
  std::cout << "=== Registers ===\n";
  for (std::size_t i = 0; i < r.size(); ++i) {
    std::cout << "r" << i << (i < 10 ? " " : "") << " = " << r[i];
    std::cout << ((i % 4 == 3) ? '\n' : '\t');
  }
  if (r.size() % 4 != 0) std::cout << '\n';
}

void printMemory(const plab::sim::DataMemory& mem, int words) {
  std::cout << "=== Data memory (first " << words << " words) ===\n";
  for (int i = 0; i < words; ++i) {
    const std::uint32_t addr = static_cast<std::uint32_t>(i) * 4;
    if (addr + 4 > mem.sizeBytes()) break;
    std::cout << "[" << addr << "] = " << mem.loadWord(addr) << '\n';
  }
}

}  // namespace

int main(int argc, char** argv) {
  Options opts;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "-h" || a == "--help") {
      printUsage(argv[0]);
      return 0;
    } else if (a == "--pipeline") {
      opts.runPipeline = true;
    } else if (a == "--interp") {
      opts.runInterp = true;
    } else if (a == "--trace") {
      opts.showTrace = true;
    } else if (a == "--stats") {
      opts.showStats = true;
    } else if (a == "--regs") {
      opts.showRegs = true;
    } else if (a == "--no-forward") {
      opts.forwarding = false;
    } else if (a == "--mem") {
      if (i + 1 >= argc) {
        std::cerr << "error: --mem requires a count\n";
        return 2;
      }
      const std::string val = argv[++i];
      if (!parseMemCount(val, opts.memWords)) {
        std::cerr << "error: invalid --mem count '" << val << "'\n";
        return 2;
      }
    } else if (!a.empty() && a[0] == '-') {
      std::cerr << "error: unknown option '" << a << "'\n";
      return 2;
    } else {
      opts.path = a;
    }
  }

  if (opts.path.empty()) {
    printUsage(argv[0]);
    return 2;
  }

  // Default mode: full pipeline demo.
  if (!opts.runInterp && !opts.runPipeline) {
    opts.runPipeline = true;
    opts.showTrace = opts.showStats = opts.showRegs = true;
  }

  std::ifstream in(opts.path);
  if (!in) {
    std::cerr << "error: cannot open '" << opts.path << "'\n";
    return 2;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  const std::string source = ss.str();

  plab::asmr::Program program;
  try {
    program = plab::asmr::assemble(source);
  } catch (const plab::asmr::LexError& e) {
    std::cerr << opts.path << ":" << e.line() << ": lex error: " << e.what() << '\n';
    return 1;
  } catch (const plab::asmr::ParseError& e) {
    std::cerr << opts.path << ":" << e.line() << ": parse error: " << e.what() << '\n';
    return 1;
  } catch (const plab::asmr::AssembleError& e) {
    std::cerr << opts.path << ":" << e.line() << ": error: " << e.what() << '\n';
    return 1;
  }

  try {
    if (opts.runInterp) {
      plab::sim::Interpreter interp(program);
      interp.run();
      std::cout << "=== Interpreter (sequential reference) ===\n";
      std::cout << "Instructions retired: " << interp.instructionsRetired() << "\n\n";
      if (opts.showRegs) printRegisters(interp.registers());
      if (opts.memWords > 0) printMemory(interp.memory(), opts.memWords);
    }

    if (opts.runPipeline) {
      plab::sim::Pipeline pipe(program, 4096, opts.forwarding);
      pipe.run();
      if (opts.showTrace) {
        std::cout << "=== Pipeline diagram"
                  << (opts.forwarding ? "" : " (no forwarding)") << " ===\n";
        std::cout << plab::viz::renderPipelineDiagram(pipe) << '\n';
      }
      if (opts.showStats) std::cout << plab::viz::renderPerformanceReport(pipe.stats()) << '\n';
      if (opts.showRegs) printRegisters(pipe.registers());
      if (opts.memWords > 0) printMemory(pipe.memory(), opts.memWords);
    }
  } catch (const std::exception& e) {
    std::cerr << "runtime error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}
