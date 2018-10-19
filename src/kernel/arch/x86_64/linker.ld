ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)

KERNEL_VMA = 0xFFFFFFFF80000000;

SECTIONS
{
    . = KERNEL_VMA;

    .text ALIGN(4K) :
    {
        *(.text)
    }

    .rodata ALIGN(4K) :
    {
        *(.rodata)
    }

    .data ALIGN(4K):
    {
        *(.data)
        *(COMMON)
    }

    .bss ALIGN(4K) :  
    {
        _bss_start = .;
        *(.bss)
        _bss_end = .;
    }
    
    _bss_length = _bss_end - _bss_start; 

    .ctors :
    {
        _constructors_start = .;
        *(SORT(.ctors*))
        _constructors_end = .;
    }

    .dtors :
    {   
        _destructors_start = .;
        *(SORT(.dtors*))
        _destructors_end = .;
    }

}