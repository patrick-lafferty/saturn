.section .text
.global cpuid_vendor
.type cpuid_vendor @function
.extern cpuid_vendor_impl

cpuid_vendor:
    movl $0, %eax
    cpuid

    push %edx
    push %ecx
    push %ebx
    push %eax

    call cpuid_vendor_impl

    pop %eax
    pop %ebx
    pop %ecx
    pop %edx

    ret
