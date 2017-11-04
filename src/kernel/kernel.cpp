#include <stdio.h>
#include <gdt/gdt.h>
#include <idt/idt.h>
#include <cpu/pic.h>
#include <cpu/cpuid.h>
#include <cpu/acpi.h>
#include <cpu/sse.h>
#include <cpu/apic.h>
#include <string.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>

using namespace Kernel;
using namespace Memory;

extern uint32_t __kernel_memory_start;
extern uint32_t __kernel_memory_end;

void acpi_stuff() {
    auto rsdp = CPU::findRSDP();

    if (verifyRSDPChecksum(rsdp)) {
        //printf("\n[ACPI] RSDP Checksum validated\n");
    }
    else {
        //printf("\n[ACPI] RSDP Checksum invalid\n");
    }

    auto rootSystemHeader = CPU::getRootSystemHeader(rsdp.rsdtAddress);

    if (verifySystemHeaderChecksum(rootSystemHeader)) {
        //printf("\n[ACPI] SDT Checksum validated\n");
        auto apicHeader = getAPICHeader(rootSystemHeader, (rootSystemHeader->length - sizeof(CPU::SystemDescriptionTableHeader)) / 4);

        if (verifySystemHeaderChecksum(apicHeader)) {
            auto apicStartingAddress = reinterpret_cast<uintptr_t>(apicHeader);
            apicStartingAddress += sizeof(CPU::SystemDescriptionTableHeader);
            
            APIC::loadAPICStructures(apicStartingAddress, apicHeader->length - sizeof(CPU::SystemDescriptionTableHeader));

        }
    }
    else {
        printf("\n[ACPI] Root Checksum invalid\n");
    }
}

extern "C" int kernel_main(MultibootInformation* info) {

    GDT::setup();
    IDT::setup();
    initializeSSE();
    PIC::disable();

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
    virtualMemManager.map_unpaged(0x7fe0000, 0x7fe0000, (0x8fe0000 - 0x7fe0000) / 0x1000, pageFlags);
    virtualMemManager.map_unpaged(0xfec00000, 0xfec00000, (0xfef00000 - 0xfec00000) / 0x1000, pageFlags | 0b10000);

    virtualMemManager.activate();

    //printf("Paging Enabled\n");
    
    acpi_stuff();

    asm("sti");

    printf("Finished loading, halting now\n");

    while(true) {
        asm("hlt");
    }

    return 0;
}