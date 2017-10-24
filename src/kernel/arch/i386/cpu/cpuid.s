section .text
global cpuid_vendor
extern cpuid_vendor_impl

cpuid_vendor:
    mov eax, 0
    cpuid

    push edx
    push ecx
    push ebx
    push eax

    call cpuid_vendor_impl

    pop eax
    pop ebx
    pop ecx
    pop edx

    ret
