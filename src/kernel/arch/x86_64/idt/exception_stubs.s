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

bits 64

section .text

%macro Stub_Errorless 1
    global exception%1

    exception%1:
        push 0
        push %1
        jmp Stub_Common
%endmacro

%macro Stub 1
    global exception%1

    exception%1:
        push %1
        jmp Stub_Common
%endmacro

extern exceptionHandler

Stub_Common:

    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    mov rdi, rsp
    mov rbx, rsp
    and rsp, ~0xF

    call exceptionHandler

    mov rsp, rbx

    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    add rsp, 16

    iretq

Stub_Errorless 0 ;divide error
Stub_Errorless 1 ;debug exception
Stub_Errorless 2 ;non-maskable external interrupt
Stub_Errorless 3 ;breakpoint
Stub_Errorless 4 ;overflow
Stub_Errorless 5 ;bound range exceeded
Stub_Errorless 6 ;invalid/undefined opcode
Stub_Errorless 7 ;device not available (math coprocessor)
Stub 8 ;double fault
Stub_Errorless 9 ;coprocessor segment overrun
Stub 10 ;invalid TSS
Stub 11 ;segment not present
Stub 12 ;stack-segment fault
Stub 13 ;general protection
Stub 14 ;page fault
Stub_Errorless 15 ;reserved
Stub_Errorless 16 ;x87 fpu floating-point error
Stub 17 ;alignment check
Stub_Errorless 18 ;machine check
Stub_Errorless 19 ;simd floating-point exception
Stub_Errorless 20 ;virtualization exception


global loadIDT
extern idtPointer
loadIDT:
    mov rax, qword idtPointer
    lidt [rax]
    ret
