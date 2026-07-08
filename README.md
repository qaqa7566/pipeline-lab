# Pipeline Lab

A tiny compiler toolchain and a cycle-accurate **5-stage CPU pipeline simulator**,
built around a custom RISC-style instruction set. It takes assembly source through
a real 32-bit machine-code encoding, runs it on both a sequential reference
interpreter and an in-order pipeline that models **data-hazard detection,
forwarding, and branch flushing**, and prints a cycle-by-cycle diagram with a
performance report.

```
assembly source ─▶ lexer ─▶ parser ─▶ two-pass assembler ─▶ 32-bit machine code
                                                                    │
                          ┌─────────────────────────────────────────┤
                          ▼                                          ▼
              sequential interpreter                     5-stage pipeline simulator
              (correctness oracle)                       IF │ ID │ EX │ MEM │ WB
                          │                                          │
                          └──────────── diff-tested ─────────────────┘
                                                                     ▼
                                              ASCII trace + performance report
```

A future version (V2) adds a tiny C-like frontend that lowers to this assembly.

## Highlights

- **Custom 32-bit ISA** with three register formats and a real encode/decode layer
  (not just decoded structs) — see [`docs/isa.md`](docs/isa.md).
- **In-order 5-stage pipeline** (IF/ID/EX/MEM/WB) with:
  - EX/MEM and MEM/WB **operand forwarding**,
  - **load-use hazard** detection with a 1-cycle stall,
  - **branch/jump resolution in EX** (predict-not-taken, 2-cycle flush),
  - a register file that writes in the first half of a cycle and reads in the second.
  See [`docs/pipeline.md`](docs/pipeline.md).
- **Correctness by construction:** the pipeline's final register + memory state is
  diff-tested against the sequential interpreter for every example program, with
  forwarding both on and off.
- **Cycle-by-cycle ASCII visualization** and a CPI/IPC/stall/flush/forward report.

## Build & test

Requires a C++20 compiler and CMake ≥ 3.20 (GoogleTest is fetched automatically).

```sh
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Run

```sh
# Full demo: pipeline diagram + performance report + final registers
./build/src/plab examples/sum_loop.asm

# Compare against the sequential reference interpreter
./build/src/plab examples/array_sum.asm --interp --regs

# Show the extra stalls you get with forwarding disabled
./build/src/plab examples/fib.asm --pipeline --stats --no-forward
```

### Example output (`examples/sum_loop.asm`)

```
addr  instruction             c1  c2  c3  c4  c5  c6  c7  c8  c9 c10 ...
0x0000  addi r1, r0, 0        IF  ID  EX MEM  WB   .   .   .   .   . ...
0x0004  addi r2, r0, 5         .  IF  ID  EX MEM  WB   .   .   .   . ...
0x0008  add r1, r1, r2         .   .  IF  ID  EX MEM  WB   .   .   . ...
0x000c  addi r2, r2, -1        .   .   .  IF  ID  EX MEM  WB   .   . ...
0x0010  bne r2, r0, -8         .   .   .   .  IF  ID  EX MEM  WB   . ...
0x0014  ret                    .   .   .   .   .  IF  ID   .   .   . ...   <- squashed by taken branch
0x0008  add r1, r1, r2         .   .   .   .   .   .   .  IF  ID  EX ...   <- loop re-fetched at branch target

=== Performance Report ===
Cycles           : 30
Instructions     : 18
CPI              : 1.667
IPC              : 0.600
Stall cycles     : 0  (load-use / RAW)
Flushes          : 4  (8 penalty cycles)
Forwards         : 8
```

Reading the diagram: a **repeated `ID`** cell is a stall bubble; a row that reaches
only `IF`/`ID` and then disappears was **squashed by a flush** (the two instructions
behind a taken branch). Run `examples/array_sum.asm` to see the load-use stall.

## Instruction set at a glance

Registers `r0`–`r15` (`r0` hardwired to zero). Byte-addressed, word-aligned memory.

| Class      | Instructions                              |
|------------|-------------------------------------------|
| ALU (reg)  | `add sub and or xor sll srl slt`          |
| ALU (imm)  | `addi andi ori xori slti`                 |
| Memory     | `lw rd, off(rs)` · `sw rd, off(rs)`       |
| Branch     | `beq bne blt` (PC-relative)               |
| Jump       | `j` · `jal` · `ret` / `halt`              |
| Pseudo     | `nop` · `mov rd, rs` · `li rd, imm`       |

Full semantics, encoding tables, and the assembly grammar are in
[`docs/isa.md`](docs/isa.md).

## Project layout

```
include/plab/{isa,asm,sim,viz}   public headers
src/{isa,asm,sim,viz}            implementations + the `plab` CLI (main.cpp)
tests/                           GoogleTest suites (one per component)
examples/                        sample .asm programs
docs/                            ISA, pipeline, and architecture references
```

See [`docs/architecture.md`](docs/architecture.md) for the module map and how the
interpreter is used as a correctness oracle.

## Roadmap

- **V1 (this repo):** ISA + assembler + interpreter + pipeline + visualization.
- **V2:** a tiny C-like frontend (lexer/parser/AST → lowering) that emits this
  assembly, introducing function calls, a calling convention, and stack frames
  (`jal`/`jr`).

## License

MIT — see [`LICENSE`](LICENSE).
