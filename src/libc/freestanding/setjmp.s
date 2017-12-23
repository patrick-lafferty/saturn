global setjmp
setjmp:
    mov ecx, [esp + 4]
    mov [ecx], ebx
    mov [ecx + 4], esi
    mov [ecx + 8], edi
    mov [ecx + 12], ebp
    lea edx, [esp + 4] 
    mov [ecx + 16], edx
    mov eax, [esp]
    mov [ecx + 20], eax ;eax
    xor eax, eax
    ret

global longjmp
longjmp:
    mov ecx, [esp + 4] 
    inc eax
    mov ebx, [ecx]
    mov esi, [ecx + 4]
    mov edi, [ecx + 8]
    mov ebp, [ecx  +12]
    mov esp, [ecx + 16]
    jmp [ecx + 20]
    ret
