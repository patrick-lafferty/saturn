#include <stdio.h>
#include <gdt/gdt.h>
#include <idt/idt.h>
#include <cpu/cpuid.h>

extern "C" int kernel_main() {

    GDT::setup();
    IDT::setup();

    printf("GDT/IDT Descriptors Installed\n");
    //asm("sti");
    
    CPUID::printVersionInformation();

    return 0;
}