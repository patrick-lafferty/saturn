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

void acpi_stuff() {
    auto rsdp = CPU::findRSDP();

    if (verifyRSDPChecksum(rsdp)) {
        //printf("\n[ACPI] RSDP Checksum validated\n");
    }
    else {
        //printf("\n[ACPI] RSDP Checksum invalid\n");
    }

    auto rootSystemHeader = CPU::getRootSystemHeader(rsdp.rsdtAddress);

    if (CPU::verifySystemHeaderChecksum(rootSystemHeader)) {
        //printf("\n[ACPI] SDT Checksum validated\n");
        auto apicHeader = CPU::getAPICHeader(rootSystemHeader, (rootSystemHeader->length - sizeof(CPU::SystemDescriptionTableHeader)) / 4);

        if (CPU::verifySystemHeaderChecksum(apicHeader)) {
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

extern "C" void taskA() {
    int a = 0; 
    int a2 = 50;

    while(true) {
        print(1, a);
        sleep(1000);    
        a++;

        print(1, a2);
        sleep(1000);
        a2++;
    }
}

void taskB() {
    int b = 0;
    int b2 = 50;
    
    while(true) {
        //print(2, b);
        sleep(1000); 
        b += 2;
        //print(2, b2);
        sleep(1000); 
        b2 += 2;
    }
}

void taskC() {
    int c = 0;
    int c2 = 50;

    while(true) {
        //print(3, c);
        sleep(3000); 
        c += 3;
        //print(3, c2);
        sleep(3000); 
        c2 += 3;
    }
}

void taskD() {
    int d = 0;
    int d2 = 50;

    while(true) {
        //print(4, d);
        sleep(4000); 
        d += 4;
        //print(4, d2);
        sleep(4000); 
        d2 += 4;
    }
}

void taskE() {
    int e = 0;
    int e2 = 50;

    while(true) {
        //print(5, e);
        sleep(5000); 
        e += 10;
        //print(5, e2);
        sleep(5000); 
        e2 += 10;
    }
}

void taskF() {
    int f = 0;
    int f2 = 50;

    while(true) {
        //print(6, f);
        sleep(2000); 
        f += 10;
        //print(6, f2);
        sleep(2000); 
        f2 += 10;
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
    CPU::setupTSS(tssAddress);

    acpi_stuff();

    Kernel::Scheduler scheduler;
    scheduler.scheduleTask(scheduler.createUserTask(reinterpret_cast<uint32_t>(taskA)));
    scheduler.scheduleTask(scheduler.createUserTask(reinterpret_cast<uint32_t>(taskB)));
    scheduler.scheduleTask(scheduler.createUserTask(reinterpret_cast<uint32_t>(taskC)));
    scheduler.scheduleTask(scheduler.createUserTask(reinterpret_cast<uint32_t>(taskD)));
    scheduler.scheduleTask(scheduler.createKernelTask(reinterpret_cast<uint32_t>(taskE)));
    scheduler.scheduleTask(scheduler.createKernelTask(reinterpret_cast<uint32_t>(taskF)));

    asm volatile("sti");

    scheduler.enterIdle();

    return 0;
}