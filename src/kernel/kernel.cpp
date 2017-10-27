#include <stdio.h>
#include <gdt/gdt.h>
#include <idt/idt.h>
#include <cpu/cpuid.h>
#include <cpu/acpi.h>
#include <cpu/sse.h>
#include <string.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>

using namespace Kernel;
using namespace Memory;

extern uint32_t __kernel_memory_start;
extern uint32_t __kernel_memory_end;

void acpi_stuff() {
    auto rsdp = CPU::findRSDP();
    /*char sig[9];
    memcpy(sig, rsdp.signature, 8);
    sig[8] = '\0';
    printf("RSDP.signature: %s\n", sig);
    printf("RSDP.checksum: %d\n", rsdp.checksum);
    memset(sig, '\0', 8);
    memcpy(sig, rsdp.oemid, 6);
    printf("RSDP.oemid: %s\n", sig);
    printf("RSDP.revision: %d\n", rsdp.revision);
    printf("RSDP.rsdtAddress: %x\n", rsdp.rsdtAddress);*/
    /*printf("RSDP.length: %d\n", rsdp.length);
    printf("RSDP.xsdtAddressLow: %d\n", rsdp.xsdtAddress & 0xffffffff);
    printf("RSDP.xsdtAddressHigh: %d\n", (rsdp.xsdtAddress & 0xffffffff00000000) >> 32);
    printf("RSDP.extendedChecksum: %d\n", rsdp.extendedChecksum);*/

    if (verifyRSDPChecksum(rsdp)) {
        printf("\nRSDP Checksum validated\n");
    }
    else {
        printf("\nChecksum invalid\n");
    }

    //auto systemDescriptionTable = CPU::findSDT(rsdp.rsdtAddress);
    auto ptr = static_cast<CPU::SystemDescriptionTableHeader*>(reinterpret_cast<void*>(rsdp.rsdtAddress));

    //char sig[9];
    /*memcpy(sig, systemDescriptionTable.signature, 4);
    sig[4] = '\0';
    printf("SDT.signature: %s\n", sig);
    printf("SDT.length: %d\n", systemDescriptionTable.length);
    printf("SDT.revision: %d\n", systemDescriptionTable.revision);
    printf("SDT.checksum: %d\n", systemDescriptionTable.checksum);
    memset(sig, '\0', 9);
    memcpy(sig, systemDescriptionTable.oemid, 6);
    printf("SDT.oemid: %s\n", sig);
    memset(sig, '\0', 9);
    memcpy(sig, systemDescriptionTable.oemTableId, 8);
    printf("SDT.oemTableId: %s\n", sig);
    memset(sig, '\0', 9);*/
    //memcpy(sig, &systemDescriptionTable.creatorId, 4);
    //printf("SDT.creatorId: %s\n", sig);
    //printf("SDT.creatorRevision: %d\n", systemDescriptionTable.creatorRevision);

    if (verifySDTChecksum(ptr)) {//systemDescriptionTable)) {
        printf("\nSDT Checksum validated\n");
    }
    else {
        printf("\nChecksum invalid\n");
    }
}

extern "C" int kernel_main(MultibootInformation* info) {

    GDT::setup();
    IDT::setup();
    initializeSSE();

    PhysicalMemoryManager physicalMemManager {info};
    VirtualMemoryManager virtualMemManager {physicalMemManager};

    auto pageFlags = 
        static_cast<int>(PageTableFlags::Present)
        | static_cast<int>(PageTableFlags::AllowWrite);

    virtualMemManager.map_unpaged(0xB8000, 0xB8000, 1, pageFlags);
    virtualMemManager.map_unpaged(0, 0, 0x100000 / 0x1000, pageFlags);
    auto kernelStartAddress = reinterpret_cast<uint32_t>(&__kernel_memory_start);
    auto kernelEndAddress = reinterpret_cast<uint32_t>(&__kernel_memory_end);
    virtualMemManager.map_unpaged(kernelStartAddress, kernelStartAddress, 1 + (kernelEndAddress - kernelStartAddress) / 0x1000, pageFlags);

    //virtualMemManager.activate();

    printf("Paging Enabled\n");

    acpi_stuff();

    return 0;
}