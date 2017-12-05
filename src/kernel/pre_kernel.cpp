#define TARGET_PREKERNEL true
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>

using namespace Kernel;
using namespace MemoryPrekernel;

extern uint32_t __kernel_memory_start;
extern uint32_t __kernel_memory_end;

__attribute__((section(".setup")))
PhysicalMemoryManager _physicalMemManager;
__attribute__((section(".setup")))
VirtualMemoryManager _virtualMemManager;

struct MemManagerAddresses {
    uint32_t physicalManager;
    uint32_t virtualManager;
    uint32_t multiboot;
};

extern "C" 
__attribute__((section(".setup")))
MemManagerAddresses MemoryManagerAddresses;
__attribute__((section(".setup")))
MemManagerAddresses MemoryManagerAddresses;

const uint32_t virtualOffset = 0xD000'0000;

extern "C" void setupKernel(MultibootInformation* info) {
    auto acpiTableLocation = _physicalMemManager.initialize(info);
    _virtualMemManager.initialize(&_physicalMemManager);

    MemoryManagerAddresses.physicalManager = reinterpret_cast<uint32_t>(&_physicalMemManager);
    MemoryManagerAddresses.virtualManager = reinterpret_cast<uint32_t>(&_virtualMemManager);
    MemoryManagerAddresses.multiboot = reinterpret_cast<uint32_t>(info);

    auto pageFlags = 
        static_cast<int>(PageTableFlags::Present)
        | static_cast<int>(PageTableFlags::AllowWrite);

    //VGA video memory
    _virtualMemManager.map_unpaged(0xB8000 + virtualOffset, 0xB8000, 1, pageFlags);

    //the first megabyte of memory, for bios stuff
    _virtualMemManager.map_unpaged(virtualOffset, 0, 0x100000 / 0x1000, pageFlags);

    auto kernelStartAddress = reinterpret_cast<uint32_t>(&__kernel_memory_start);
    auto kernelEndAddress = reinterpret_cast<uint32_t>(&__kernel_memory_end);

    //TODO: until we get an elf loader, kernel code needs to have usermode access
    pageFlags |= static_cast<int>(PageTableFlags::AllowUserModeAccess); 

    //the higher-half kernel address
    _virtualMemManager.map_unpaged(kernelStartAddress + virtualOffset, kernelStartAddress, 1 + (kernelEndAddress - virtualOffset - kernelStartAddress) / 0x1000, pageFlags);
    
    /*
    temporarily identity map the kernel until we enter the higher half, so EIP
    is always valid
    */
    _virtualMemManager.map_unpaged(kernelStartAddress, kernelStartAddress , 1 + (kernelEndAddress - virtualOffset - kernelStartAddress) / 0x1000, pageFlags);

    //TODO: until we get an elf loader, kernel code needs to have usermode access
    pageFlags &= ~static_cast<int>(PageTableFlags::AllowUserModeAccess); 

    ACPITableLocation location {static_cast<uint32_t>(acpiTableLocation >> 32), 
        static_cast<uint32_t>(acpiTableLocation & 0xFFFFFFFF)};

    //ACPI tables
    if (location.startAddress == 0 || location.pages == 0) {
        //QEMU doesn't record the ACPI memory range as type 3, so just hardcode it
        _virtualMemManager.map_unpaged(0x7fe0000, 0x7fe0000, (0x8fe0000 - 0x7fe0000) / 0x1000, pageFlags);
    }
    else {
        //we're on a sane system, map the proper addresses
        _virtualMemManager.map_unpaged(location.startAddress, location.startAddress, location.pages, pageFlags);
    }

    //APIC registers
    _virtualMemManager.map_unpaged(0xfec00000, 0xfec00000, (0xfef00000 - 0xfec00000) / 0x1000, pageFlags | 0b10000);

    _virtualMemManager.activate();
}