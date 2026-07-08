; Store three values into memory, then load and accumulate them in a loop.
; The `lw` immediately feeding `add` triggers a load-use stall each iteration,
; making the single unavoidable data stall visible in the trace and stats.

        li   r1, 10
        sw   r1, 0(r0)
        li   r1, 20
        sw   r1, 4(r0)
        li   r1, 30
        sw   r1, 8(r0)

        li   r5, 0          ; running sum
        li   r6, 0          ; byte index
        li   r7, 12         ; end index (3 words)
loop:   lw   r2, 0(r6)      ; load array[index]
        add  r5, r5, r2     ; sum += array[index]   (load-use hazard)
        addi r6, r6, 4      ; index += 4 bytes
        blt  r6, r7, loop   ; continue while index < end
        ret                 ; r5 = 60
