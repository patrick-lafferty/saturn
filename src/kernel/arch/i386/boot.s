;/*Multiboot standard, see https://www.gnu.org/software/grub/manual/multiboot/multiboot.html#Header-magic-fields*/
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

;/*kernel entry*/
section .text

extern _init
extern setupKernel
extern kernel_main

global _start
start equ 0x100000
global start

section .setup
_start:
    mov esp, stack_top_ ; - 0xD0000000

    ;call _init
    
    push ebx
    call setupKernel
    pop ebx

    lea ecx, [higherHalf]
    jmp ecx

section .text

higherHalf:

    mov esp, stack_top

    call _init

    call kernel_main

    cli

.keep_halting:  
    hlt
    jmp .keep_halting

