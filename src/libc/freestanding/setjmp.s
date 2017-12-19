global setjmp
setjmp:
    mov ecx, [ebp + 8]
    mov [ecx], ebx
    mov [ecx + 4], esi
    mov [ecx + 8], edi
    mov [ecx + 12], ebp
    mov edx, [ecx + 16]
    lea edx, [ebp + 8]
    mov [ecx + 20], eax
    xor eax, eax

global longjmp
longjmp:
    mov ecx, [ebp + 8]
    inc eax
    mov ebx, [ecx]
    mov esi, [ecx + 4]
    mov edi, [ecx + 8]
    mov ebp, [ecx  +12]
    mov esp, [ecx + 16]
    jmp [ecx + 20]
