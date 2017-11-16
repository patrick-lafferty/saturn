#include <stdio.h>
#include <gdt/gdt.h>
#include <idt/idt.h>
#include <cpu/pic.h>
#include <cpu/cpuid.h>
#include <cpu/acpi.h>
#include <cpu/sse.h>
#include <cpu/apic.h>
#include <cpu/tss.h>
#include <string.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <scheduler.h>
#include <system_calls.h>

using namespace Kernel;
using namespace Memory;

extern uint32_t __kernel_memory_start;
extern uint32_t __kernel_memory_end;

bool parseACPITables() {
    auto rsdp = CPU::findRSDP();

    if (!verifyRSDPChecksum(rsdp)) {
        printf("\n[ACPI] RSDP Checksum invalid\n");
        return false;
    }

    auto rootSystemHeader = CPU::getRootSystemHeader(rsdp.rsdtAddress);

    if (CPU::verifySystemHeaderChecksum(rootSystemHeader)) {
        auto apicHeader = CPU::getAPICHeader(rootSystemHeader, (rootSystemHeader->length - sizeof(CPU::SystemDescriptionTableHeader)) / 4);

        if (CPU::verifySystemHeaderChecksum(apicHeader)) {
            auto apicStartingAddress = reinterpret_cast<uintptr_t>(apicHeader);
            apicStartingAddress += sizeof(CPU::SystemDescriptionTableHeader);
            
            APIC::initialize();
            APIC::loadAPICStructures(apicStartingAddress, apicHeader->length - sizeof(CPU::SystemDescriptionTableHeader));
        }
        else {
            printf("\n[ACPI] APIC Header Checksum invalid\n");
            return false;
        }
    }
    else {
        printf("\n[ACPI] Root Checksum invalid\n");
        return false;
    }

    return true;
}

__attribute__((section(".setup")))
PhysicalMemoryManager physicalMemManager;// {info};
__attribute__((section(".setup")))
VirtualMemoryManager virtualMemManager;// {physicalMemManager};

const uint32_t virtualOffset = 0xD000'0000;

extern "C" void 
__attribute__((section(".setup")))
setupKernel(MultibootInformation* info) {

    physicalMemManager.initialize(info);
    virtualMemManager.initialize(&physicalMemManager);

    auto pageFlags = 
        static_cast<int>(PageTableFlags::Present)
        | static_cast<int>(PageTableFlags::AllowWrite)
        | static_cast<int>(PageTableFlags::AllowUserModeAccess);

    virtualMemManager.map_unpaged(0xB8000 + virtualOffset, 0xB8000, 1, pageFlags);
    virtualMemManager.map_unpaged(virtualOffset, 0, 0x100000 / 0x1000, pageFlags);
    auto kernelStartAddress = reinterpret_cast<uint32_t>(&__kernel_memory_start);
    auto kernelEndAddress = reinterpret_cast<uint32_t>(&__kernel_memory_end);
    virtualMemManager.map_unpaged(kernelStartAddress + virtualOffset, kernelStartAddress, 1 + (kernelEndAddress - virtualOffset - kernelStartAddress) / 0x1000, pageFlags);
    virtualMemManager.map_unpaged(kernelStartAddress, kernelStartAddress , 1 + (kernelEndAddress - virtualOffset - kernelStartAddress) / 0x1000, pageFlags);
    virtualMemManager.map_unpaged(0x7fe0000, 0x7fe0000, (0x8fe0000 - 0x7fe0000) / 0x1000, pageFlags);
    virtualMemManager.map_unpaged(0x7fd000, 0x7fd000, (0x8fe000 - 0x7fd000) / 0x1000, pageFlags);
    virtualMemManager.map_unpaged(0xfec00000, 0xfec00000, (0xfef00000 - 0xfec00000) / 0x1000, pageFlags | 0b10000);

    virtualMemManager.activate();

    //virtualMemManager.unmap(kernelStartAddress, 1 + (kernelEndAddress - kernelStartAddress) / 0x1000);
}

extern "C" int kernel_main() {

    Memory::currentPMM = &physicalMemManager;

    GDT::setup();
    IDT::setup();
    initializeSSE();
    PIC::disable();

     auto pageFlags = 
        static_cast<int>(PageTableFlags::Present)
        | static_cast<int>(PageTableFlags::AllowWrite)
        | static_cast<int>(PageTableFlags::AllowUserModeAccess);

    
    auto kernelStartAddress = reinterpret_cast<uint32_t>(&__kernel_memory_start);
    auto kernelEndAddress = reinterpret_cast<uint32_t>(&__kernel_memory_end);
    auto afterKernel = virtualOffset + kernelStartAddress + (20 + (kernelEndAddress - kernelStartAddress) / 0x1000) * 0x1000;
    virtualMemManager.HACK_setNextAddress(afterKernel);

    auto tssAddress = virtualMemManager.allocatePages(3, pageFlags);
    HACK_TSS_ADDRESS = tssAddress;
    asm volatile("sti");
    //uint32_t* t = static_cast<uint32_t*>(reinterpret_cast<void*>(tssAddress));
    //*t = 0;
    GDT::addTSSEntry(tssAddress, 0x1000 * 3);
    CPU::setupTSS(tssAddress);

    if (!parseACPITables()) {
        printf("[Kernel] Parsing ACPI Tables failed, halting\n");
        asm volatile("hlt");
    }

    Kernel::Scheduler scheduler;


    scheduler.enterIdle();

    return 0;
}