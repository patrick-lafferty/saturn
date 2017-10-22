/*Multiboot standard, see https://www.gnu.org/software/grub/manual/multiboot/multiboot.html#Header-magic-fields*/
.set ALIGNED_MODULES_PAGE, 1 << 0
.set MEMORY_INFORMATION, 1 << 1
.set MAGIC_NUMBER, 0x1BADB002
.set FLAGS, ALIGNED_MODULES_PAGE | MEMORY_INFORMATION
.set CHECKSUM, -(MAGIC_NUMBER + FLAGS)

.section .multiboot
.align 4
.long MAGIC_NUMBER
.long FLAGS
.long CHECKSUM

/*setup the stack*/
.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:

/*kernel entry*/
.section .text
.global _start
.type _start, @function
_start:
    movl $stack_top, %esp

    call _init
    call kernel_main

    cli
1:  hlt
    jmp 1b

.size _start, . - _start
