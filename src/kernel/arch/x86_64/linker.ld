ENTRY(bootstrap32)
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

SECTIONS
{
    . = 1M;

    .bootstrap : 
    {
        *(.multiboot)
        src/kernel/arch/x86_64/bootstrap.o (.text)
    }

    .bootstrap_stack :  
    {
        src/kernel/arch/x86_64/bootstrap.o (.bootstrap_stack)
    }

}