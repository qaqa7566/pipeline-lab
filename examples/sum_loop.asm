; Sum the integers 1..5 into r1 (result = 15).
; Demonstrates a backward branch and the resulting flush penalty each iteration.

        li   r1, 0          ; sum
        li   r2, 5          ; counter
loop:   add  r1, r1, r2     ; sum += counter
        addi r2, r2, -1     ; counter--
        bne  r2, r0, loop   ; repeat while counter != 0
        ret
