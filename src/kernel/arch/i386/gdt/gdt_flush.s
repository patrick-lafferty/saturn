section .text
global gdt_flush
extern gp

gdt_flush:
    lgdt [gp]
    mov eax, 0x10
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    mov ss, eax

    jmp 0x08:flush 

flush:
    ret
