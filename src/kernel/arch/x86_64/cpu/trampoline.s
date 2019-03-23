%if 0

Copyright (c) 2019, Patrick Lafferty
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
bits 16

global trampoline_entry
global trampoline_size

trampoline_entry:
    cli

    ;offsets:
    ; 0xfe4: 32-bit GDT Pointer address
    ; 0xfe8: pml4 address
    ; 0xff0: 64-bit GDT Pointer address
    ; 0xff8: stack address

    ;the bootstrap processor is waiting for this processor
    ;to acknowledge the IPI/SIPI sequence by incrementing
    ;a flag at cs:0xfd8
    mov dword [cs:0xfd8], 1
    ;once the bsp notices we acknowledged, it acknowledges
    ;the acknowledgement by setting the flag to 1337
    cmp dword [cs:0xfd8], 1337
    ;seeing that means we can continue
    jne $-10

    ;load the GDT
    mov eax, dword [cs:0xfe4]
    o32 lgdt [eax]

    ;setup paging and protected mode
    mov eax, cr0
    or eax, 1 ;0x80000001
    mov cr0, eax

    mov eax, [cs:0xfe8]
    mov ebp, [cs:0xff0]
    mov esp, [cs:0xff8]

    ;set cs by jumping and then finish setting the rest
    ;Note: the 0x4000 offset gets patched in Trampoline::create
    ;to be the actual address of the trampoline
    jmp 0x08:(0x4000 + flush_32 - trampoline_entry)

bits 32

flush_32:

    mov bx, 0x10
    mov ds, bx
    mov ss, bx
    mov es, bx

    ;load the pml4
    mov cr3, eax
    mov eax, cr4
    bts eax, 5 ; enable Page
    mov cr4, eax

    ;set the LM bit in EFER MSR
    mov ecx, 0xC0000080 
    rdmsr
    bts eax, 8
    wrmsr

    ;enable paging
    mov eax, cr0
    bts eax, 31
    mov cr0, eax

    ;load the 64-bit temporary gdt
    mov eax, ebp
    lgdt [eax]

    jmp 0x08:(0x4000 + entryPoint64 - trampoline_entry)

bits 64
global entryPoint64
entryPoint64:
    
    ;we only had 32-bits to store a 64-bit stack address,
    ;but the top 32-bits are constant (fixed at PDP 511 ie last)
    ;so just add that here
    mov rax, 0xffffff8000000000
    add rsp, rax

    ;tell the bootstrap processor that we're done with
    ;the trampoline, so it can reuse/free it
    pop rax ; the status flag address
    mov dword [rax], 9001
    pop rax
    pop rdi
    pop rsi

    and rsp, -16
    mov rbp, rsp
    call rax

    cli
    hlt

trampoline_end:

section .data
trampoline_size: dd trampoline_end - trampoline_entry