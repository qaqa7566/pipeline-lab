# Pipeline Lab ISA

A small, custom RISC-style instruction set. Fixed-width 32-bit instructions, 16
registers, byte-addressed word-aligned memory. Designed to be minimal while still
exercising every hazard class a classic 5-stage pipeline must handle.

## Registers

- `r0`–`r15`, each 32-bit.
- **`r0` is hardwired to zero:** reads always return 0; writes are discarded. This
  gives clean idioms (`nop`, `mov`, `li`).
- V2 ABI (documented now, unused in V1): `r15` = return address (`jal` link),
  `r14` = stack pointer.

## Memory model

- Separate instruction and data memories (**Harvard**), which removes structural
  hazards between `IF` and `MEM`.
- Data memory is **byte-addressed** but accessed a **word (4 bytes) at a time**;
  `lw`/`sw` addresses must be 4-byte aligned or the access faults.
- Single-cycle "magic" memory (no caches or latency modelling in V1).
- The program counter is a byte address and advances by 4 per instruction.

## Instruction formats

Every instruction is 32 bits. The opcode occupies the top 6 bits; register fields
are 4 bits each.

```
 31    26 25   22 21   18 17   14 13                     0
+--------+-------+-------+-------+------------------------+
| opcode |  rd   |  rs   |  rt   |        unused          |  R-type
+--------+-------+-------+-------+------------------------+
| opcode |  rd   |  rs   |     imm (18-bit signed)        |  I-type
+--------+-------+-------+--------------------------------+
| opcode |  rs   |  rt   |     imm (18-bit signed)        |  B-type
+--------+-------+-------+--------------------------------+
| opcode |            target (26-bit)                     |  J-type
+--------+------------------------------------------------+
```

- The 18-bit immediate/offset is sign-extended; range **−131072 … 131071**.
- B-type reuses the rd/rs slots for its two source registers; the immediate is a
  **byte offset relative to the branch instruction's own address**
  (`target = branch_pc + imm`).
- J-type `target` is a 26-bit **absolute byte address**.

## Instructions

Opcode numbers are the values written into the 6-bit field.

### R-type — `op rd, rs, rt`
| op | # | Semantics |
|----|---|-----------|
| `add` | 0 | `rd = rs + rt` |
| `sub` | 1 | `rd = rs - rt` |
| `and` | 2 | `rd = rs & rt` |
| `or`  | 3 | `rd = rs \| rt` |
| `xor` | 4 | `rd = rs ^ rt` |
| `sll` | 5 | `rd = rs << (rt & 31)` |
| `srl` | 6 | `rd = rs >> (rt & 31)` (logical) |
| `slt` | 7 | `rd = (rs < rt) ? 1 : 0` (signed) |

### I-type — `op rd, rs, imm`
| op | # | Semantics |
|----|---|-----------|
| `addi` | 8 | `rd = rs + imm` |
| `andi` | 9 | `rd = rs & imm` |
| `ori`  | 10 | `rd = rs \| imm` |
| `xori` | 11 | `rd = rs ^ imm` |
| `slti` | 12 | `rd = (rs < imm) ? 1 : 0` |

### Memory (I-type) — `op rd, imm(rs)`
| op | # | Semantics |
|----|---|-----------|
| `lw` | 13 | `rd = mem[rs + imm]` |
| `sw` | 14 | `mem[rs + imm] = rd` (the rd slot holds the store-value source) |

### Branch (B-type) — `op rs, rt, label`
| op | # | Taken when |
|----|---|-----------|
| `beq` | 15 | `rs == rt` |
| `bne` | 16 | `rs != rt` |
| `blt` | 17 | `rs < rt` (signed) |

The assembler computes the byte offset from the branch to the label; you may also
write an explicit numeric offset.

### Jumps (J-type)
| op | # | Semantics |
|----|---|-----------|
| `j target`   | 18 | `pc = target` |
| `jal target` | 19 | `r15 = pc + 4; pc = target` |
| `ret`/`halt` | 20 | ends execution (V1 has no callee returns yet) |

### Pseudo-instructions (assembler-expanded, one word each)
| Written | Expands to |
|---------|-----------|
| `nop`        | `add r0, r0, r0` |
| `mov rd, rs` | `add rd, rs, r0` |
| `li rd, imm` | `addi rd, r0, imm` |

## Assembly syntax

- One statement per line; `;` begins a comment to end of line.
- Labels are `name:`; multiple labels may precede an instruction.
- Registers are written `rN`; immediates are decimal or `0x` hex, optionally signed.
- Memory operands are `disp(base)`, e.g. `lw r1, 8(r2)`; `(r2)` means displacement 0.

```asm
        li   r1, 0          ; sum
        li   r2, 5          ; counter
loop:   add  r1, r1, r2
        addi r2, r2, -1
        bne  r2, r0, loop
        ret
```

## Encoding example

`add r1, r2, r3` → opcode 0, rd 1, rs 2, rt 3:

```
opcode=000000 rd=0001 rs=0010 rt=0011 unused=0…0
  (1<<22) | (2<<18) | (3<<14) = 0x0048C000
```

(The exact bit layout is verified by the encode/decode round-trip tests in
`tests/test_encoding.cpp`.)

The encoder rejects out-of-range registers and immediates; the decoder rejects
unknown opcodes. See `tests/test_encoding.cpp`.
