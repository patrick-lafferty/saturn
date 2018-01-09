%if 0

Copyright (c) 2017, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

%endif
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
InterruptServiceRoutine_Errorless 2 ;non-maskable external interrupt
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
InterruptServiceRoutine_Errorless 19 ;simd floating-point exception
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
InterruptServiceRoutine_Errorless 49; APIC IRQ 1
InterruptServiceRoutine_Errorless 50; APIC IRQ 2
InterruptServiceRoutine_Errorless 51; APIC IRQ 3

global isr52
extern taskSwitch
isr52:

    push ebx
    push eax
    push esi
    push edi
    pushfd

    call taskSwitch

    popfd
    pop edi
    pop esi
    pop eax
    pop ebx

    iret

InterruptServiceRoutine_Errorless 53; APIC IRQ 5
InterruptServiceRoutine_Errorless 54; APIC IRQ 6
InterruptServiceRoutine_Errorless 55; APIC IRQ 7
InterruptServiceRoutine_Errorless 56; APIC IRQ 8
InterruptServiceRoutine_Errorless 57; APIC IRQ 9
InterruptServiceRoutine_Errorless 58; APIC IRQ 10
InterruptServiceRoutine_Errorless 59; APIC IRQ 11
InterruptServiceRoutine_Errorless 207; APIC spurious interrupt

;Usermode system calls
InterruptServiceRoutine_Errorless 255

extern interruptHandler

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

