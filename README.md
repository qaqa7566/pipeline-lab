# Pipeline Lab

[![CI](https://github.com/qaqa7566/pipeline-lab/actions/workflows/ci.yml/badge.svg)](https://github.com/qaqa7566/pipeline-lab/actions/workflows/ci.yml)

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

## Demo

`examples/array_sum.asm` walks an array and accumulates the sum, so it has both a
load-use dependency and a loop branch — a good case for seeing forwarding earn its
keep. Run it once with forwarding (the default) and once without:

```sh
# With forwarding (default)
./build/src/plab examples/array_sum.asm --pipeline --stats

# Without forwarding
./build/src/plab examples/array_sum.asm --pipeline --stats --no-forward
```

The two runs execute the same instructions and land on the same final state; only
the pipeline behavior differs:

| Run           | Cycles | Stall cycles | Forwards |
|---------------|:------:|:------------:|:--------:|
| forwarding    |   33   |      3       |    10    |
| no forwarding |   49   |     19       |     0    |

Turning forwarding off trades all 10 forwards for stalls, and the RAW hazards that
were hidden behind those forwards now cost 16 extra stall cycles (3 → 19). Add
`--trace` (or run with no flags for the full default output) to also see the
cycle-by-cycle diagram.

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

### CLI flags

With no flags, `plab` runs `--pipeline --trace --stats --regs`. Passing any flag
switches to an explicit set, so list exactly what you want.

| Flag           | Effect                                                        |
|----------------|--------------------------------------------------------------|
| `--pipeline`   | run the 5-stage pipeline simulator                           |
| `--interp`     | run the sequential reference interpreter                     |
| `--trace`      | print the cycle-by-cycle pipeline diagram                    |
| `--stats`      | print the performance report (cycles, CPI/IPC, stalls, …)    |
| `--regs`       | print the final register file                                |
| `--mem N`      | print the first `N` words of data memory                     |
| `--no-forward` | disable operand forwarding (pipeline only; more stalls)      |
| `-h`, `--help` | show usage                                                   |

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

## Known limitations / non-goals

This is a teaching-scale model of one specific microarchitecture, not a
general-purpose simulator. On purpose, V1 does not model:

- **Memory timing** — loads and stores complete in a single cycle; there is no
  memory latency.
- **Caches** — no I-cache or D-cache, and therefore no miss penalties.
- **Dynamic branch prediction** — the pipeline is fixed predict-not-taken and
  resolves branches in EX; there is no BTB or history-based predictor.
- **Anything beyond a scalar, in-order 5-stage pipeline** — no superscalar issue,
  no out-of-order execution, no register renaming.
- **`ret`** acts as a halt in V1 (there is no call stack yet).
- A **C-like frontend** is future work, not part of this repo — see the roadmap.

## Roadmap

- **V1 (this repo):** ISA + assembler + interpreter + pipeline + visualization.
- **V2:** a tiny C-like frontend (lexer/parser/AST → lowering) that emits this
  assembly, introducing function calls, a calling convention, and stack frames
  (`jal`/`jr`).

## License

MIT — see [`LICENSE`](LICENSE).
