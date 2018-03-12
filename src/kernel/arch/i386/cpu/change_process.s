%if 0

Copyright (c) 2017, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

%endif
section .text

global startProcess
extern HACK_TSS_ADDRESS

startProcess:

    mov ecx, [esp + 4]
    mov esp, [ecx + 4] 

    mov eax, [HACK_TSS_ADDRESS]
    mov ecx, [ecx + 4]
    mov [eax + 4], ecx

    ret    


;changes from one ring0 process to another
global changeProcess
extern activateVMM

changeProcess:

    push ebp
    mov ebp, esp
    add esp, 4

    mov eax, [ebp + 8]
    mov [eax + 4], esp ; currentTask->context.kernelESP = esp

    ;saving fpu/sse registers
    ;mov eax, [eax + 8]
    ;fxsave [eax]

    mov eax, [ebp + 12]
    push eax
    call activateVMM
    pop eax

    mov eax, [ebp + 12]
    mov esp, [eax + 4] ; esp = nextTask->context.kernelESP

    ;restoring fpu/sse registers
    ;mov eax, [eax + 8]
    ;fxrstor [eax]

    mov eax, [HACK_TSS_ADDRESS]
    mov ecx, [ebp + 12] ; nextTask->context                
    mov ecx, [ecx + 4] ; nextTask->context.kernelESP
    mov [eax + 4], ecx ; tss->esp0 = nextTask->context.kernelESP

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
    push 0x202 ; eflags
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
    invlpg [0xfffff000]
    ret

global idleLoop
idleLoop:
    hlt
    jmp idleLoop