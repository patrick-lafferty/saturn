section .text
global changeProcess
changeProcess:
    ;c signature:
    ;void changeProcess(Task* current, Task* next)

    ;save the important registers to the current
    ;task's stack

    push eax
    push ecx
    push edx
    push ebx
    push ebp
    push esi
    push edi
    pushfd 

    ;store the stack pointer to the current
    ;task's context.esp
    mov eax, [esp + 36] 
    mov DWORD [eax], esp 

    ;change the current stack pointer
    ;to the next task's context.esp
    mov eax, DWORD [esp + 40] 
    mov esp, DWORD [eax]

    ;restore the important registers from the
    ;next task's stack

    popfd
    pop edi
    pop esi
    pop ebp
    pop ebx
    pop edx
    pop ecx

    ;no
    ;mov eax, 0x23
    ;mov ds, ax
    ;mov es, ax
    ;mov fs, ax
    ;mov gs, ax


    pop eax

    ret

extern taskA;
global launchProcess
launchProcess:
    mov eax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push 0x23
    mov eax, esp
    push eax
    pushfd
    push 0x1B
    ;push dword [taskA]
    push taskA

    iret
    
global fillTSS
fillTSS:
    mov ebp, [esp + 4]
    mov [ebp + 4], esp
    mov [ebp + 8], DWORD 0x10
    ret