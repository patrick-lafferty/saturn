#include <stdio.h>
#include <gdt/gdt.h>
#include <idt/idt.h>
#include <cpu/cpuid.h>
#include <cpu/acpi.h>
#include <string.h>

extern "C" int kernel_main() {

    GDT::setup();
    IDT::setup();

    printf("GDT/IDT Descriptors Installed\n");
    //asm("sti");
    
    //CPUID::printVersionInformation();
    auto rsdp = CPU::findRSDP();
    char sig[9];
    memcpy(sig, rsdp.signature, 8);
    sig[8] = '\0';
    printf("RSDP.signature: %s\n", sig);
    printf("RSDP.checksum: %d\n", rsdp.checksum);
    memset(sig, '\0', 8);
    memcpy(sig, rsdp.oemid, 6);
    printf("RSDP.oemid: %s\n", sig);
    printf("RSDP.revision: %d\n", rsdp.revision);
    printf("RSDP.rsdtAddress: %d\n", rsdp.rsdtAddress);
    printf("RSDP.length: %d\n", rsdp.length);
    printf("RSDP.xsdtAddressLow: %d\n", rsdp.xsdtAddress & 0xffffffff);
    printf("RSDP.xsdtAddressHigh: %d\n", (rsdp.xsdtAddress & 0xffffffff00000000) >> 32);
    printf("RSDP.extendedChecksum: %d\n", rsdp.extendedChecksum);

    if (verifyChecksum(rsdp)) {
        printf("\nChecksum validated\n");
    }
    else {
        printf("\nChecksum invalid\n");
    }

    return 0;
}