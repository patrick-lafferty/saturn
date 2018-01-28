section .text
global sendImplementation
sendImplementation:
    push ebp
    mov ebp, esp

    mov eax, 3
    mov ebx, [ebp + 8]
    mov ecx, [ebp + 12]

    int 0xFF

    mov esp, ebp
    pop ebp
    ret