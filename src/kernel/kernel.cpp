#include <stdio.h>
#include <gdt/gdt.h>
#include <idt/idt.h>
#include <cpu/pic.h>
#include <cpu/acpi.h>
#include <cpu/sse.h>
#include <cpu/tss.h>
#include <string.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <scheduler.h>
#include <system_calls.h>
#include <test/libc/runner.h>
#include <test/libc++/new.h>
#include <ipc.h>
#include <services.h>
#include <initialize_libc.h>
#include <heap.h>
#include <services/memory/memory.h>
#include <services/ps2/ps2.h>
#include <services/terminal/vga.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <services/processFileSystem/processFileSystem.h>
#include <services/fakeFileSystem/fakeFileSystem.h>
#include <services/hardwareFileSystem/system.h>
#include <services/startup/startup.h>
#include <services/discovery/loader.h>

using namespace Kernel;
using namespace Memory;

extern uint32_t __kernel_memory_start;
extern uint32_t __kernel_memory_end;

/*
We get a pointer to this struct passed in from boot.s to kernel_main.
The address is to an instance of the struct created in the pre-kernel,
in pre_kernel.cpp
*/
struct MemManagerAddresses {
    uint32_t physicalManager;
    uint32_t virtualManager;
};

extern "C" int kernel_main(MemManagerAddresses* addresses) {
    initializeLibC();
    /*
    The prekernel creates its own PMM and VMM in lower addresses.
    We want the higher-half kernel to have its own proper versions
    so we can unmap the old addresses, so copy them over here.
    */
    PhysicalMemoryManager physicalMemManager = 
        *reinterpret_cast<PhysicalMemoryManager*>(
            addresses->physicalManager);

    VirtualMemoryManager virtualMemManager = 
        *reinterpret_cast<VirtualMemoryManager*>(
            addresses->virtualManager);

    virtualMemManager.setPhysicalManager(&physicalMemManager);

    auto kernelStartAddress = reinterpret_cast<uint32_t>(&__kernel_memory_start);
    auto kernelEndAddress = reinterpret_cast<uint32_t>(&__kernel_memory_end);
    const uint32_t virtualOffset = 0xD000'0000;

    //we don't need the identity map anymore
    virtualMemManager.unmap(kernelStartAddress, 1 + (kernelEndAddress - virtualOffset - kernelStartAddress) / 0x1000);

    Memory::currentPMM = &physicalMemManager;
    Memory::currentVMM = &virtualMemManager;

    GDT::setup();
    IDT::setup();
    initializeSSE();
    PIC::disable();

    PS2::initializeController();
    PS2::identifyDevices();
    VGA::disableCursor();

    asm volatile("sti");

    auto pageFlags = static_cast<int>(PageTableFlags::AllowWrite);

    auto afterKernel = (kernelEndAddress & ~0xfff) + 0x1000;

    virtualMemManager.HACK_setNextAddress(0xCFFF'F000);
    auto tssAddress = virtualMemManager.allocatePages(1, pageFlags);
    virtualMemManager.HACK_setNextAddress(afterKernel);

    HACK_TSS_ADDRESS = tssAddress;
    GDT::addTSSEntry(tssAddress, 0x1000 * 1);
    auto tss = CPU::setupTSS(tssAddress);

    if (!CPU::parseACPITables()) {

        kprintf("[Kernel] Parsing ACPI Tables failed, halting\n");
        asm volatile("hlt");
    }

    //also don't need APIC tables anymore
    //NOTE: if we actually do, copy them before this line to a new address space
    virtualMemManager.unmap(0x7fe0000, (0x8fe0000 - 0x7fe0000) / 0x1000);

    LibC_Implementation::createHeap(PageSize * PageSize);

    Kernel::Scheduler scheduler{tss};

    kprintf("Saturn OS v 0.1.0\n------------------\n\n");

    ServiceRegistry registry;
    ServiceRegistryInstance = &registry;

    /*runNewTests();*/
    //runAllLibCTests();

    scheduler.scheduleTask(scheduler.createUserTask(reinterpret_cast<uint32_t>(VFS::service)));
    scheduler.scheduleTask(scheduler.createUserTask(reinterpret_cast<uint32_t>(PFS::service)));
    scheduler.scheduleTask(scheduler.createUserTask(reinterpret_cast<uint32_t>(FakeFileSystem::service)));
    scheduler.scheduleTask(scheduler.createUserTask(reinterpret_cast<uint32_t>(HardwareFileSystem::service)));
    scheduler.scheduleTask(scheduler.createKernelTask(reinterpret_cast<uint32_t>(HardwareFileSystem::detectHardware)));
    scheduler.scheduleTask(scheduler.createKernelTask(reinterpret_cast<uint32_t>(Discovery::discoverDevices)));
    scheduler.scheduleTask(scheduler.createUserTask(reinterpret_cast<uint32_t>(Startup::service)));

    scheduler.enterIdle();

    return 0;
}