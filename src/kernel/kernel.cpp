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
#include <scheduler.h>

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
            
            APIC::initialize();
            APIC::loadAPICStructures(apicStartingAddress, apicHeader->length - sizeof(CPU::SystemDescriptionTableHeader));

        }
    }
    else {
        printf("\n[ACPI] Root Checksum invalid\n");
    }
}

struct TSS {
    uint32_t previousTaskLink;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldtSegmentSelector;
    uint16_t reserved;
    uint16_t ioMapBaseAddress;
};

volatile int x;
extern "C" void taskA() {
    printf("[TaskA] Hello, world\n");
    for(int i = 0; i < 1000000; i++) 
        x++;
    asm volatile("cli");
    taskA();
}

void taskB() {
    printf("[TaskB] This is\n");
    asm volatile("hlt");
    taskB();
}

void taskC() {
    printf("[TaskC] a test of\n");
    asm volatile("hlt");
    taskC();
}

void taskD() {
    printf("[TaskD] the emergency broadcast system\n");
    asm volatile("hlt");
    taskD();
}

extern "C" void launchProcess();
extern "C" void fillTSS(TSS* tss);
extern "C" void loadTSS();

extern "C" int kernel_main(MultibootInformation* info) {

    GDT::setup();
    IDT::setup();
    initializeSSE();
    PIC::disable();

    PhysicalMemoryManager physicalMemManager {info};
    VirtualMemoryManager virtualMemManager {physicalMemManager};

    auto pageFlags = 
        static_cast<int>(PageTableFlags::Present)
        | static_cast<int>(PageTableFlags::AllowWrite)
        | static_cast<int>(PageTableFlags::AllowUserModeAccess)
        ;

    virtualMemManager.map_unpaged(0xB8000, 0xB8000, 1, pageFlags);
    virtualMemManager.map_unpaged(0, 0, 0x100000 / 0x1000, pageFlags);
    auto kernelStartAddress = reinterpret_cast<uint32_t>(&__kernel_memory_start);
    auto kernelEndAddress = reinterpret_cast<uint32_t>(&__kernel_memory_end);
    virtualMemManager.map_unpaged(kernelStartAddress, kernelStartAddress, 1 + (kernelEndAddress - kernelStartAddress) / 0x1000, pageFlags);
    virtualMemManager.map_unpaged(0x7fe0000, 0x7fe0000, (0x8fe0000 - 0x7fe0000) / 0x1000, pageFlags);
    virtualMemManager.map_unpaged(0xfec00000, 0xfec00000, (0xfef00000 - 0xfec00000) / 0x1000, pageFlags | 0b10000);

    virtualMemManager.activate();

    virtualMemManager.HACK_setNextAddress(0xa0000000);
    auto tssAddress = virtualMemManager.allocatePages(3, pageFlags);
    GDT::addTSSEntry(tssAddress, 0x1000 * 3);

    TSS* tss = static_cast<TSS*>(reinterpret_cast<void*>(tssAddress));
    fillTSS(tss);
    loadTSS();

    //printf("Paging Enabled\n");

    acpi_stuff();

    Kernel::Scheduler scheduler;
    //scheduler.scheduleTask(scheduler.createTestTask(reinterpret_cast<uint32_t>(launchProcess)));
    //scheduler.scheduleTask(scheduler.createTestTask(reinterpret_cast<uint32_t>(taskA)));
    scheduler.scheduleTask(scheduler.createTestTask(reinterpret_cast<uint32_t>(taskB)));
    //scheduler.scheduleTask(scheduler.createTestTask(reinterpret_cast<uint32_t>(taskC)));
    //scheduler.scheduleTask(scheduler.createTestTask(reinterpret_cast<uint32_t>(taskD)));

    asm("sti");

    scheduler.enterIdle();

    return 0;
}