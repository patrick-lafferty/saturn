section .text
global changeProcess
changeProcess:

    push eax
    push ecx
    push edx
    push ebx
    ;push esp
    push ebp
    push esi
    push edi
    pushfd 

    ;mov ebp, [esp + 16]
    ;mov ebp, DWORD [esp + 32]

    mov eax, [esp + 36] ;[ebp + 4]
    mov DWORD [eax], esp ;ebp

    mov eax, DWORD [esp + 40] ;[ebp + 8]
    mov esp, DWORD [eax]

    popfd
    pop edi
    pop esi
    pop ebp
    ;pop esp
    pop ebx
    pop edx
    pop ecx
    pop eax

    ret


;works
changeProcess2:
    ;push all the registers on the current process's stack

    ;switch to the next process's stack
    mov eax, [esp + 4]
    mov esp, [eax]

    ;pop all the registers from the new process's stack
    ret