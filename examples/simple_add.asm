; Smallest possible program: load two immediates and add them.
; r3 = 5 + 7 = 12. No hazards, no branches — the baseline trace.

        li   r1, 5
        li   r2, 7
        add  r3, r1, r2
        ret
