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
;Multiboot standard, see https://www.gnu.org/software/grub/manual/multiboot/multiboot.html#Header-magic-fields
ALIGNED_MODULES_PAGE equ 1 << 0
MEMORY_INFORMATION equ 1 << 1
MAGIC_NUMBER equ 0x1BADB002
FLAGS equ ALIGNED_MODULES_PAGE | MEMORY_INFORMATION
CHECKSUM equ -(MAGIC_NUMBER + FLAGS)

section .multiboot
align 4
dd MAGIC_NUMBER
dd FLAGS
dd CHECKSUM

;/*setup the stack*/
section .bsss
align 16
stack_bottom_:
resb 16384
stack_top_:

section .bss
align 16
stack_bottom:
resb 16384
stack_top:

section .text

extern _init
extern setupKernel
extern kernel_main

global _start

section .setup
extern MemoryManagerAddresses

_start:
    mov esp, stack_top_ ;

    ;call _init
    
    push ebx
    call setupKernel
    pop ebx

    mov eax, MemoryManagerAddresses

    lea ecx, [higherHalf]
    jmp ecx


section .text

higherHalf:

    mov esp, stack_top

    call _init

    push eax
    call kernel_main

    cli

.keep_halting:  
    hlt
    jmp .keep_halting

