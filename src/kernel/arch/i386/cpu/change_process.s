section .text

global startProcess
extern HACK_TSS_ADDRESS

startProcess:

    mov ecx, [esp + 4]
    mov esp, [ecx] 

    mov eax, [HACK_TSS_ADDRESS]
    mov ecx, [ecx + 4]
    mov [eax + 4], ecx

    ret    


;changes from one ring0 process to another
global changeProcess
extern activateVMM

changeProcess:

    mov eax, ebp
    push ebp
    mov ebp, esp
    add esp, 4

    mov eax, [ebp + 8]
    mov [eax], esp

    mov eax, [ebp + 12]
    push eax
    call activateVMM
    pop eax

    mov eax, [ebp + 12]
    mov esp, [eax]

    mov eax, [HACK_TSS_ADDRESS]
    mov ecx, [ebp + 12]
    mov ecx, [ecx + 4]
    mov [eax + 4], ecx

    ret

;launches a usermode process
global launchProcess
launchProcess:

    ;when launchProcess is called, the stack contains
    ; user vmm
    ; user esp
    ; user eip
    pop eax

    pop ecx
    pop ebx

    ;set the selectors to usermode's gdt entries
    mov eax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax


    push 0x23 ; usermode data segment
    push ecx ; user esp
    pushfd ; eflags
    push 0x1B ; usermode code segment
    push ebx ; user eip

    iret

endTask:
    mov eax, 1
    int 0xFF
    ret

global usermodeStub
;calls the user function in ebx, then when it returns
;calls endTask to cleanup the task
usermodeStub:

    add esp, 20
    pop ebx

    call ebx

    call endTask
    ret

global elfUsermodeStub
extern loadElf
;loads a compiled program from an elf file
elfUsermodeStub:

    push ebp
    mov ebp, esp

    add esp, 20
    pop ebx

    call loadElf

    call eax

    call endTask
    ret

global fillTSS
fillTSS:
    mov eax, [esp + 4]
    mov [eax + 4], esp
    mov [eax + 8], DWORD 0x10
    ret

global loadTSS
loadTSS:
    mov ax, 0x28
    ltr ax
    ret

global setCR3
setCR3:
    mov eax, [esp + 4]
    mov cr3, eax
    ret

global idleLoop
idleLoop:
    hlt
    jmp idleLoop