section .text
global initializeSSE
initializeSSE:
    mov eax, CR4
    or eax, 1 << 9 ;set OSFXR bit
    or eax, 1 << 10 ;set OSXMMEXCPT bit
    mov CR4, eax
    mov eax, CR0
    ;and eax, -(1 << 2) ;clear EM bit
    ;or eax, 1 << 1 ;set MP bit
    mov eax, 0x80000001
    mov CR0, eax
    ret

