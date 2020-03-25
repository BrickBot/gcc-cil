.global ___mulhi3
___mulhi3:
    mov.w    r0,r2
    mulxu.b  r1h,r2
    mov.b    r0h,r2h
    mulxu.b  r1l,r0
    add.b    r2l,r0h
    mulxu.b  r2h,r1
    add.b    r1l,r0h
    rts

.global ___udivhi3
___udivhi3:
    push r5
    push r6
    mov.w r0,r6
    mov.w r1,r5
    jsr @@78
    mov.w r6,r0
    pop r6
    pop r5
    rts


.globl ___divhi3
___divhi3:
    push r5
    push r6
    mov.w r0,r6
    mov.w r1,r5
    jsr @@82
    mov.w r6,r0
    pop r6
    pop r5
    rts

.global ___divsi3
___divsi3:
    push r4
    push r5
    push r6
    mov.w r1,r6
    mov.w r0,r5
    mov.w r3,r4
    mov.w r2,r3
    jsr @@88
    mov.w r6,r1
    mov.w r5,r0
    pop r6
    pop r5
    pop r4
    rts

.globl __ZN5CliHw8Hardware3Cpu5SleepEv
__ZN5CliHw8Hardware3Cpu5SleepEv:
    bclr #7, @0xC4:8
    sleep
    rts
    
.globl __ZN5CliHw8Hardware3Cpu15InvokeSchedulerEv
__ZN5CliHw8Hardware3Cpu15InvokeSchedulerEv:
    ; save context, r6 has been saved by ROM
    push r0
    push r1
    push r2
    push r3
    push r4
    push r5
    
    ; pass current stack pointer
    push r7
    
    ; pass location for new stack pointer
    push r7
    
    ; call scheduler method
    mov.w @__ZN5CliHw8Hardware3Cpu9SchedulerE:16, r2
    jsr @r2

    ; pop new stack pointer
    pop r7
    
    ; adjust stack
    adds #2, r7

    ; restore context, r6 will be restored by ROM
    pop r5
    pop r4
    pop r3
    pop r2
    pop r1
    pop r0
    
    rts

.globl __ZN5CliHw8Hardware3Cpu9InitStackEPvt
__ZN5CliHw8Hardware3Cpu9InitStackEPvt:

    ; move stack pointer in r1
    mov.w @(6,r7), r1
    
    ; prepare stack
    ; push pc
    mov.w @(4,r7), r2
    mov.w r2, @r1 
    
    ; push ccr
    sub.w r2, r2
    mov.w r2, @-r1
    
    ; push r6
    mov.w r2, @-r1
    
    ; push timer interrupt return address
    mov.w #0x04d4, r2
    mov.w r2, @-r1
    
    ; reserve space for registers r0 - r5
    add.b #0xF4, r1l
    addx #0xFF, r1h
    
    ; push stack pointer
    mov.w r1, @-r1

    ; return stack pointer
    mov.w @(2,r7), r2
    mov.w r1, @r2
    rts
    
      

.ascii "Do you byte, when I knock?\0"
