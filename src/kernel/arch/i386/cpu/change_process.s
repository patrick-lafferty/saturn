section .text

global startProcess
startProcess:
    mov ecx, DWORD [esp + 4] 
    mov esp, DWORD [ecx]

    ;restore the important registers from the
    ;next task's stack

    ;TODO: HACK: don't hardcode TSS address, and there might be multiple
    ;ie one per cpu

    ;store the current kernel stack's esp to the TSS
    mov eax, 0xa0000000
    mov ecx, [ecx + 4]
    mov [eax + 4], ecx

    popfd
    pop edi
    pop esi
    pop ebp
    pop ebx
    ;pop edx
    ;pop ecx
    ;pop eax

    ret

;changes from one ring0 process to another
global changeProcess
changeProcess:
    ;c signature:
    ;void changeProcess(Task* current, Task* next)

    ;save the important registers to the current
    ;task's stack
    mov eax, [esp + 4]
    mov ecx, [esp + 8]

    ;push eax
    ;push ecx
    ;push edx
    push ebx
    push ebp
    push esi
    push edi
    pushfd 

    ;store the stack pointer to the current
    ;task's context.esp
    ;mov eax, [esp + 36] 
    ;mov DWORD [eax], esp 
    mov [eax], esp

    ;change the current stack pointer
    ;to the next task's context.esp
    ;mov eax, DWORD [esp + 40] 
    ;mov esp, DWORD [eax]
    mov esp, [ecx]

    ;TODO: HACK: don't hardcode TSS address, and there might be multiple
    ;ie one per cpu

    ;restore the important registers from the
    ;next task's stack

    popfd
    pop edi
    pop esi
    pop ebp
    pop ebx
    ;pop edx
    ;pop ecx
    ;pop eax
    ;store the current kernel stack's esp to the TSS
    mov eax, 0xa0000000
    mov ecx, [ecx + 4]
    mov [eax + 4], ecx

    ret

extern taskA

;launches a usermode process
global launchProcess
launchProcess:

    ;address of user function is stored in eax
    ;mov ebx, eax
    pop ecx
    pop ebx
    ;mov ecx, 0xa0007000
    ;mov ebx, taskA

    ;set the selectors to usermode's gdt entries
    mov eax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push 0x23 ;usermode data segment
    ;user stack is stored in ecx
    push ecx ;esp
    pushfd ;eflags
    push 0x1B ;usermode code segment
    push ebx ;eip

    iret

global fillTSS
fillTSS:
    mov ebp, [esp + 4]
    mov [ebp + 4], esp
    mov [ebp + 8], DWORD 0x10
    mov [ebp + 12], DWORD 0x68
    ret

global loadTSS
loadTSS:
    mov ax, 0x28
    ltr ax
    ret