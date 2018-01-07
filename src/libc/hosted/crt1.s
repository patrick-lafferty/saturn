section .text

global _start
extern main

_start:
    mov ebp, 0
    push ebp
    mov ebp, esp

    ;TODO: call constructors

    call main

    pop ebp

    ret