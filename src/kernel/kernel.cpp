#include <stdio.h>
#include <gdt/gdt.h>
#include <idt/idt.h>
#include <cpu/pic.h>
#include <cpu/cpuid.h>
#include <cpu/acpi.h>
#include <cpu/sse.h>
#include <cpu/tss.h>
#include <string.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <scheduler.h>
#include <system_calls.h>

#include <test/libc/stdlib.h>
#include <test/libc++/new.h>
#include <ipc.h>
#include <services.h>
#include <services/terminal/terminal.h>

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

/*void taskA() {

    IPC::Message message;
    message.length = 0;

    while (true) {
        print(1, 0);
        sleep(1000);
        send(2, &message);
        message.length++;
    }
}

void taskB() {
    IPC::Message message {};

    while (true) {
        receive(&message);
        print(2, message.length);
        message = IPC::Message {};
        sleep(1000);
    }
}*/

void vgaService() {
    RegisterService registerRequest {};
    registerRequest.type = ServiceType::VGA;

    IPC::MaximumMessageBuffer buffer;
    Terminal* terminal {nullptr};

    send(ServiceRegistryMailbox, &registerRequest);
    receive(&buffer);

    if (buffer.messageId == RegisterServiceDenied::MessageId) {
        //can't print to screen, how to notify?
        print(99, 0);   
    }
    else if (buffer.messageId == VGAServiceMeta::MessageId) {
        auto vgaMeta = IPC::extractMessage<VGAServiceMeta>(buffer);
        terminal = new Terminal(reinterpret_cast<uint16_t*>(vgaMeta.vgaAddress));
        
        char msg[] = "VGA Service online\n";
        char* p = msg;
        while(*p) {
            terminal->writeCharacter(*p++);
        }
    }

    while (true) {
        sleep(1000);
        terminal->writeCharacter('p');
    }
}

extern "C" int kernel_main(MemManagerAddresses* addresses) {

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

    asm volatile("sti");

    auto pageFlags = 
        static_cast<int>(PageTableFlags::AllowWrite)
        | static_cast<int>(PageTableFlags::AllowUserModeAccess);

    auto afterKernel = (kernelEndAddress & ~0xfff) + 0x1000;
    virtualMemManager.HACK_setNextAddress(afterKernel);

    auto tssAddress = virtualMemManager.allocatePages(3, pageFlags);
    HACK_TSS_ADDRESS = tssAddress;
    GDT::addTSSEntry(tssAddress, 0x1000 * 3);
    CPU::setupTSS(tssAddress);

    if (!CPU::parseACPITables()) {

        printf("[Kernel] Parsing ACPI Tables failed, halting\n");
        asm volatile("hlt");
    }

    //also don't need APIC tables anymore
    //NOTE: if we actually do, copy them before this line to a new address space
    virtualMemManager.unmap(0x7fe0000, (0x8fe0000 - 0x7fe0000) / 0x1000);

    Kernel::Scheduler scheduler;

    printf("Saturn OS v 0.1.0\n------------------\n\n");

    ServiceRegistry registry;
    ServiceRegistryInstance = &registry;

    /*runMallocTests();
    runNewTests();*/

    scheduler.scheduleTask(scheduler.createUserTask(reinterpret_cast<uint32_t>(vgaService)));
    //scheduler.scheduleTask(scheduler.createUserTask(reinterpret_cast<uint32_t>(taskA)));
    //scheduler.scheduleTask(scheduler.createKernelTask(reinterpret_cast<uint32_t>(taskB)));
    scheduler.enterIdle();

    return 0;
}