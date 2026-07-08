; Compute the 10th Fibonacci number (0,1,1,2,...) into r3.
; Chained dependent adds exercise EX/MEM and MEM/WB forwarding with no stalls.

        li   r1, 0          ; fib(0)
        li   r2, 1          ; fib(1)
        li   r4, 9          ; iterations remaining
loop:   add  r3, r1, r2     ; next = a + b
        mov  r1, r2         ; a = b
        mov  r2, r3         ; b = next
        addi r4, r4, -1
        bne  r4, r0, loop
        ret                 ; r3 = 55
