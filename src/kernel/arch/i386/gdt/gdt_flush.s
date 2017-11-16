section .text
global gdt_flush
extern gp

gdt_flush:
    lgdt [gp]

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
