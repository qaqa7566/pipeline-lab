# Pipeline design

An in-order, single-issue, 5-stage pipeline modelled cycle-by-cycle. The simulator
uses **double-buffered latches**: each cycle it reads the current latches and
computes a fresh set, then commits them atomically — mirroring real edge-triggered
pipeline registers and sidestepping stage-ordering bugs.

## Stages

| Stage | Work |
|-------|------|
| **IF**  | Fetch the instruction at `pc` from instruction memory; `pc += 4`. |
| **ID**  | Decode, read the register file, sign-extend the immediate. |
| **EX**  | ALU op; compute the branch condition and target; **resolve control flow here**. |
| **MEM** | Data-memory load/store. |
| **WB**  | Write the result back to the register file. |

Four latches connect them: `IF/ID`, `ID/EX`, `EX/MEM`, `MEM/WB`. Each carries the
decoded instruction, its PC, control signals, the operand values read at ID, and the
results computed downstream.

## Register file timing

The register file is **written in the first half of a cycle and read in the second
half**. An instruction in WB therefore makes its value visible to an instruction in
ID *in the same cycle*, so a producer three or more instructions ahead needs no
forwarding path at all.

## Data hazards

### Forwarding
Handled by a forwarding unit consulted in EX. For each source operand it checks, in
priority order:

1. **EX/MEM → EX** — the instruction now in MEM produced a value the EX instruction
   needs. Forwarded unless that producer is a **load** (its data isn't ready yet).
2. **MEM/WB → EX** — the instruction now in WB produced the needed value (this is the
   path that also delivers a load's result after the load-use stall).

A forward fires only when the producer writes a register (`RegWrite`), its
destination is not `r0`, and the destination matches the consumer's source.

### Load-use stall
A load's data is available only at the end of MEM, so an instruction that needs it in
the *very next* cycle cannot be satisfied by forwarding alone. The hazard-detection
unit spots this (a load in EX whose destination is a source of the instruction in ID)
and **stalls one cycle**: `pc` and `IF/ID` freeze, and a bubble is injected into
`ID/EX`. After the stall, the load has reached MEM/WB and forwarding delivers the
value. This is the only data stall that remains once forwarding is enabled.

```
lw   r2, 0(r6)    IF  ID  EX  MEM  WB
add  r5, r5, r2       IF  ID  ID   EX   MEM  WB     <- one stall (repeated ID), then MEM/WB forward
```

### Forwarding off
The simulator can run with forwarding disabled (`--no-forward`). It then stalls on
*any* RAW hazard against an instruction still in EX or MEM, resolving only via the
first-half/second-half register file. Back-to-back dependent ALU ops cost two stalls
instead of zero — a direct, measurable demonstration of what forwarding buys.

## Control hazards

Branches and jumps **resolve in EX** under a **predict-not-taken** policy: fetch keeps
going sequentially, and if the branch turns out taken (or it's a jump), the two
instructions already fetched behind it (in `IF/ID` and `ID/EX`) are **flushed** to
bubbles and the PC is redirected. That is a fixed **2-cycle penalty** per taken
control transfer.

```
beq  r1, r0, tgt   IF  ID  EX ...          <- resolves taken in EX
(next)                 IF  ID   (squashed)
(next+1)                   IF   (squashed)
tgt: ...                       IF  ID ...   <- correct path re-fetched
```

`ret`/`halt` similarly squashes younger instructions and stops fetching, then the
pipeline drains.

## Performance counters

The simulator tracks, per run:

- **cycles** and **instructions retired** (an instruction retires when it clears EX),
- **CPI** and **IPC**,
- **stall cycles** (load-use / RAW bubbles),
- **flushes** (taken branches + jumps) and their **penalty cycles** (2 each),
- **forwards** (operand values delivered by the forwarding unit).

A hazard-free run approaches CPI = 1 as the fill cost is amortized. Every counter is
asserted against hand-computed values in `tests/test_hazards.cpp`.

## Why it's trustworthy

The pipeline shares its ALU and branch semantics with the sequential interpreter
(`plab/sim/alu.hpp`), and `tests/test_pipeline_correctness.cpp` asserts that the two
models reach **identical registers and memory** on every example program, with
forwarding both enabled and disabled. Timing is layered on top of semantics that are
independently verified.
