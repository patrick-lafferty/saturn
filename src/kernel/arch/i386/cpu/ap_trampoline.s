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
bits 16

global trampoline_start
global trampoline_size

trampoline_start:
    cli

    ;offsets:
    ; 0xff0: GDT Pointer address
    ; 0xff4: Page Directory physical address
    ; 0xff8: Stack to use once paging is enabled
    ; 0xffc: Kernel func to call once in protected mode

    ;the bootstrap processor is waiting for this processor
    ;to acknowledge the IPI/SIPI sequence by incrementing
    ;a flag at cs:0xfec
    mov dword [cs:0xfec], 1
    ;once the bsp notices we acknowledged, it acknowledges
    ;the acknowledgement by setting the flag to 1337
    cmp dword [cs:0xfec], 1337
    ;seeing that means we can continue
    jne $-10

    ;load the GDT
    mov eax, dword [cs:0xff0]
    o32 lgdt [eax]

    ;setup paging and protected mode
    mov eax, [cs:0xff4]
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax

    ;switch to kernel stack
    mov esp, [cs:0xff8]

    ;set cs by jumping and then finish setting the rest
    jmp far dword 0x08:finish

trampoline_end:

bits 32

;sets all of the selectors and then jumps to the kernel
;function stored in edx
finish:
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax

    ;tell the bootstrap processor that we're done with
    ;the trampoline, so it can reuse/free it
    pop eax ; the status flag address
    mov dword [eax], 9001
    pop eax
    call eax
    

    ret

section .data
trampoline_size: db trampoline_end - trampoline_start
