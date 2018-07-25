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

bits 32 

;Multiboot standard, see https://www.gnu.org/software/grub/manual/multiboot/multiboot.html#Header-magic-fields
MAGIC_NUMBER equ 0xE85250D6 

section .multiboot

align 8
header_start:

dd MAGIC_NUMBER ;multiboot 2 magic number
dd 0 ; architecture (0 = i386 in protected mode)
dd header_end - header_start
dd -(MAGIC_NUMBER + (header_end - header_start))

dw 0
dw 0
dd 8

header_end:

section .bss
align 16
stack_bottom:
resb 16384
stack_top:

section .text

global boot
extern startup

;main entrypoint for Saturn
;sets up initial stack and calls startup

boot:
    
    cli
    mov esp, stack_top

    push eax ; magic value to verify we booted with multiboot
    push ebx ; address of multiboot info structure
    call startup

    hlt
