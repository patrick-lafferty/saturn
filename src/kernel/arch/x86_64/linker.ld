ENTRY(initializeKernel)
OUTPUT_FORMAT(elf64-x86-64)

KERNEL_VMA = 0xFFFFFFFF80000000;

SECTIONS
{
    . = KERNEL_VMA;

    .text : AT(ADDR(.text) - KERNEL_VMA) 
    {
        *(.text)
        *(.rodata)
        . = ALIGN(4K);
    }

    .data ALIGN(4K):
    {
        *(.data)
    }

    .bss ALIGN(4K) :  
    {
        *(COMMON)
        *(.bss)
    }

}