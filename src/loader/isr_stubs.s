%if 0

Copyright (c) 2018, Patrick Lafferty
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

extern interruptHandler

global InterruptServiceRoutine_Common
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

global loadIDT
extern idtPointer
loadIDT:
    lidt [idtPointer]
    ret

global gdt32_flush
extern gp32

gdt32_flush:
    lgdt [gp32]

    call reloadSegments
    ret

reloadSegments:
    jmp 0x08:flush 

flush:
    mov eax, 0x10
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    mov ss, eax

    ret

global loadTopLevelPage
loadTopLevelPage:

    push ebp
    mov ebp, esp

    mov eax, [ebp + 8]
    mov cr3, eax
    mov eax, cr4
    bts eax, 5 ; enable PAE 
    mov cr4, eax

    mov esp, ebp
    pop ebp

    ret

global finalEnter
extern gp64
extern mapKernel

;finalEnter performs the final steps to setting up long mode
;ie modifying EFER, enabling paging and loading the 64-bit GDT
;this function never returns
finalEnter:

    ;set the LM bit in EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    bts eax, 8
    wrmsr

    ;enable paging 
    mov eax, CR0
    bts eax, 31
    mov cr0, eax

    lgdt [gp64]

    jmp 0x08:entryPoint64

bits 64
global entryPoint64
entryPoint64:

    mov rax, [esp + 4] 
    mov rdi, [esp + 12]
    mov rsi, [esp + 20]
    
    call rax

    hlt