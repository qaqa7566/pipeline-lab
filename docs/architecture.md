# Architecture

Pipeline Lab is organized as four small libraries with a strict dependency order,
plus a CLI and a test suite. Each library builds in isolation with warnings treated
as errors.

```
        plab_isa   ← instruction formats, 32-bit encode/decode, disassembly
           ▲
     ┌─────┴─────┐
 plab_asm     plab_sim   ← register file, data memory, ALU, interpreter, pipeline
                 ▲
             plab_viz     ← ASCII trace diagram + performance report
                 ▲
             plab_cli     ← the `plab` command-line driver (main.cpp)
```

- **`plab_isa`** (`include/plab/isa`, `src/isa`)
  `Opcode`/`Instruction`, format classification, `encode`/`decode` for the 32-bit
  machine words, and `disassemble`. No dependencies.
- **`plab_asm`** (`include/plab/asm`, `src/asm`)
  `Lexer` → `Parser` → two-pass `assemble`. Produces a `Program`: the machine-code
  word image, the decoded instructions, a per-word source-line map, and the symbol
  table.
- **`plab_sim`** (`include/plab/sim`, `src/sim`)
  Shared state (`RegisterFile`, `DataMemory`) and shared semantics (`alu.hpp`), the
  sequential `Interpreter`, and the 5-stage `Pipeline` (control signals, hazard
  detection, forwarding, flushing, stats, and a per-cycle trace).
- **`plab_viz`** (`include/plab/viz`, `src/viz`)
  Renders the pipeline's trace into the cycle-by-cycle diagram and the performance
  report.

## Data flow

```
source text
  → Lexer.tokenize()        (plab_asm)
  → Parser.parse()          → statements with symbolic operands
  → assemble()              → Program { words, instructions, sourceLines, symbols }
  → Interpreter.run()       (plab_sim)  → reference architectural state
  → Pipeline.run()          (plab_sim)  → same state + PipelineStats + trace
  → renderPipelineDiagram() (plab_viz)  → ASCII output
```

## The interpreter as a correctness oracle

The single most important design choice: the **sequential interpreter is the source
of truth**. It executes one instruction to completion per step, so its final
registers and data memory define what a program "means." The pipeline is a *timing*
model layered on the *same* ALU/branch semantics (both call into
`plab/sim/alu.hpp`), and it is validated by asserting that its final architectural
state equals the interpreter's for every example program — with forwarding both on
and off (`tests/test_pipeline_correctness.cpp`). Bugs in stalling, forwarding, or
flushing surface immediately as a state mismatch.

## Testing strategy

One GoogleTest binary per component, discovered by CTest:

| Suite | Focus |
|-------|-------|
| `test_encoding` | encode/decode round-trips, field ranges, sign extension |
| `test_lexer_parser` | tokenization, operand shapes, error cases |
| `test_assembler` | opcode selection, labels, pseudo-instructions, errors |
| `test_interpreter` | per-op semantics, memory, control flow, faults |
| `test_pipeline_correctness` | pipeline state == interpreter state (both modes) |
| `test_hazards` | stall / forward / flush counts and CPI/IPC vs hand calc |
| `test_viz` | diagram and report contents |

## Extension points for V2 (C frontend)

- A new `plab_cc` library (`lexer`/`parser`/`ast`/`lower`) would emit `asmr::Program`
  or assembly text, reusing the entire backend unchanged.
- The ISA already reserves `jal`/`r15` (link) and `r14` (stack pointer); V2 adds a
  `jr`-style return and a calling convention so the interpreter and pipeline gain
  function calls with no changes to the hazard/forwarding machinery.
