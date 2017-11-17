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
};

extern "C" 
__attribute__((section(".setup")))
MemManagerAddresses MemoryManagerAddresses;
__attribute__((section(".setup")))
MemManagerAddresses MemoryManagerAddresses;

const uint32_t virtualOffset = 0xD000'0000;

extern "C" void setupKernel(MultibootInformation* info) {

    _physicalMemManager.initialize(info);
    _virtualMemManager.initialize(&_physicalMemManager);

    MemoryManagerAddresses.physicalManager = reinterpret_cast<uint32_t>(&_physicalMemManager);
    MemoryManagerAddresses.virtualManager = reinterpret_cast<uint32_t>(&_virtualMemManager);

    auto pageFlags = 
        static_cast<int>(PageTableFlags::Present)
        | static_cast<int>(PageTableFlags::AllowWrite)
        | static_cast<int>(PageTableFlags::AllowUserModeAccess);

    _virtualMemManager.map_unpaged(0xB8000 + virtualOffset, 0xB8000, 1, pageFlags);
    _virtualMemManager.map_unpaged(virtualOffset, 0, 0x100000 / 0x1000, pageFlags);
    auto kernelStartAddress = reinterpret_cast<uint32_t>(&__kernel_memory_start);
    auto kernelEndAddress = reinterpret_cast<uint32_t>(&__kernel_memory_end);
    _virtualMemManager.map_unpaged(kernelStartAddress + virtualOffset, kernelStartAddress, 1 + (kernelEndAddress - virtualOffset - kernelStartAddress) / 0x1000, pageFlags);
    _virtualMemManager.map_unpaged(kernelStartAddress, kernelStartAddress , 1 + (kernelEndAddress - virtualOffset - kernelStartAddress) / 0x1000, pageFlags);
    _virtualMemManager.map_unpaged(0x7fe0000, 0x7fe0000, (0x8fe0000 - 0x7fe0000) / 0x1000, pageFlags);
    _virtualMemManager.map_unpaged(0x7fd000, 0x7fd000, (0x8fe000 - 0x7fd000) / 0x1000, pageFlags);
    _virtualMemManager.map_unpaged(0xfec00000, 0xfec00000, (0xfef00000 - 0xfec00000) / 0x1000, pageFlags | 0b10000);

    _virtualMemManager.activate();
}