section .text
global initializeSSE
initializeSSE:
    mov eax, CR4
    or eax, 1 << 9 ;set OSFXR bit
    or eax, 1 << 10 ;set OSXMMEXCPT bit
    mov CR4, eax
    mov eax, CR0
    mov eax, 0x80000003
    mov CR0, eax
    ret

