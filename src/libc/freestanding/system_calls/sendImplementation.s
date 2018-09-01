section .text
global sendImplementation
sendImplementation:
    push rbp
    mov rbp, rsp

    mov rax, 3
    mov rbx, rdi
    mov rcx, rsi
    int 0xFF

    mov rsp, rbp
    pop rbp
    ret