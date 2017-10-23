.section .text
.global gdt_flush
.type gdt_flush, @function
.extern gp

gdt_flush:
    lgdt (gp)
    movl $0x10, %eax
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %fs
    movl %eax, %gs
    movl %eax, %ss

    ljmp $0x8, $flush

flush:
    ret
