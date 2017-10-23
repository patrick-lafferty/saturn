#include <stdio.h>
#include <gdt/gdt.h>
#include <idt/idt.h>


extern "C" int kernel_main() {

    GDT::setup();
    IDT::setup();

    printf("GDT/IDT Descriptors Installed\n");

    return 0;
}