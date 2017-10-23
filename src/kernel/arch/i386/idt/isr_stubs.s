.section .text

.macro InterruptServiceRoutine_Errorless index
    .global isr\index
    .type isr\index, @function

    isr\index:
        cli
        push $0
        push $\index
        jmp InterruptServiceRoutine_Common
.endm

.macro InterruptServiceRoutine index
    .global isr\index
    .type isr\index, @function

    isr\index:
        cli
        push $\index
        jmp InterruptServiceRoutine_Common
.endm

InterruptServiceRoutine_Errorless 0 //divide error
InterruptServiceRoutine_Errorless 1 //debug exception
InterruptServiceRoutine_Errorless 2 //non-maskable exxternal interrupt
InterruptServiceRoutine_Errorless 3 //breakpoint
InterruptServiceRoutine_Errorless 4 //overflow
InterruptServiceRoutine_Errorless 5 //bound range exceeded
InterruptServiceRoutine_Errorless 6 //invalid/undefined opcode
InterruptServiceRoutine_Errorless 7 //device not available (math coprocessor)
InterruptServiceRoutine 8 //double fault
InterruptServiceRoutine_Errorless 9 //coprocessor segment overrun
InterruptServiceRoutine 10 //invalid TSS
InterruptServiceRoutine 11 //segment not present
InterruptServiceRoutine 12 //stack-segment fault
InterruptServiceRoutine 13 //general protection
InterruptServiceRoutine 14 //page fault
InterruptServiceRoutine_Errorless 15 //reserved
InterruptServiceRoutine_Errorless 16 //x87 fpu floating-point error
InterruptServiceRoutine 17 //alignment check
InterruptServiceRoutine_Errorless 18 //machine check
InterruptServiceRoutine_Errorless 19 //simd floating-point excpetion
InterruptServiceRoutine_Errorless 20 //virtualization exception

.extern interruptHandler
.type interruptHandler, @function

InterruptServiceRoutine_Common:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    push %esp

    call interruptHandler

    add $4, %esp

    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp

    iret

