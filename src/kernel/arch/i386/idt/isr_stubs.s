section .text

%macro InterruptServiceRoutine_Errorless 1
    global isr%1

    isr%1:
        cli
        push 0
        push %1
        jmp InterruptServiceRoutine_Common
%endmacro

%macro InterruptServiceRoutine 1
    global isr%1

    isr%1:
        cli
        push %1
        jmp InterruptServiceRoutine_Common
%endmacro

InterruptServiceRoutine_Errorless 0 ;divide error
InterruptServiceRoutine_Errorless 1 ;debug exception
InterruptServiceRoutine_Errorless 2 ;non-maskable exxternal interrupt
InterruptServiceRoutine_Errorless 3 ;breakpoint
InterruptServiceRoutine_Errorless 4 ;overflow
InterruptServiceRoutine_Errorless 5 ;bound range exceeded
InterruptServiceRoutine_Errorless 6 ;invalid/undefined opcode
InterruptServiceRoutine_Errorless 7 ;device not available (math coprocessor)
InterruptServiceRoutine 8 ;double fault
InterruptServiceRoutine_Errorless 9 ;coprocessor segment overrun
InterruptServiceRoutine 10 ;invalid TSS
InterruptServiceRoutine 11 ;segment not present
InterruptServiceRoutine 12 ;stack-segment fault
InterruptServiceRoutine 13 ;general protection
InterruptServiceRoutine 14 ;page fault
InterruptServiceRoutine_Errorless 15 ;reserved
InterruptServiceRoutine_Errorless 16 ;x87 fpu floating-point error
InterruptServiceRoutine 17 ;alignment check
InterruptServiceRoutine_Errorless 18 ;machine check
InterruptServiceRoutine_Errorless 19 ;simd floating-point excpetion
InterruptServiceRoutine_Errorless 20 ;virtualization exception

;remapped IRQs

InterruptServiceRoutine_Errorless 32; IRQ 0
InterruptServiceRoutine_Errorless 33; IRQ 1
InterruptServiceRoutine_Errorless 34; IRQ 2
InterruptServiceRoutine_Errorless 35; IRQ 3
InterruptServiceRoutine_Errorless 36; IRQ 4
InterruptServiceRoutine_Errorless 37; IRQ 5
InterruptServiceRoutine_Errorless 38; IRQ 6
InterruptServiceRoutine_Errorless 39; IRQ 7
InterruptServiceRoutine_Errorless 40; IRQ 8
InterruptServiceRoutine_Errorless 41; IRQ 9
InterruptServiceRoutine_Errorless 42; IRQ 10
InterruptServiceRoutine_Errorless 43; IRQ 11
InterruptServiceRoutine_Errorless 44; IRQ 12
InterruptServiceRoutine_Errorless 45; IRQ 13
InterruptServiceRoutine_Errorless 46; IRQ 14
InterruptServiceRoutine_Errorless 47; IRQ 15

;APIC
InterruptServiceRoutine_Errorless 48; APIC IRQ 0

extern interruptHandler
type interruptHandler, @function

InterruptServiceRoutine_Common:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp

    call interruptHandler

    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8

    iret

