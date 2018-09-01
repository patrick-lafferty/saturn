/*
Copyright (c) 2018, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "multiboot.h"
#include "print.h"
#include "elf.h"
#include "panic.h"
#include "idt_32.h"
#include "gdt.h"

struct PhysicalMemStats {
    uint64_t firstAddress;
    uint64_t nextFreeAddress;
    uint64_t totalPages;
    uint64_t acpiLocation;
};

void freePhysicalPages(uint64_t pageAddress, uint64_t count, PhysicalMemStats& stats) {

    /*
    We want to set aside the first four physical pages to be used for the initial
    paging structures. If we instead used stats.nextFreeAddress,
    we would need an additional page table to account for where nextFreeAddress
    might be (ie 512mb ram => nextFreeAddress might be 0x1ffdf000, which would
    need a page table at entry 127)
    */
    const int PageSize = 0x1000;

    stats.firstAddress = pageAddress;
    pageAddress += PageSize * 4;

    for (uint64_t i = 4; i < count; i++) {
        auto page = reinterpret_cast<uint64_t volatile*>(pageAddress & ~0xfff);
        *page = stats.nextFreeAddress;
        stats.nextFreeAddress = pageAddress;
        pageAddress += PageSize;
    }

    stats.totalPages += count;
}

template<typename T>
T* alignCast(uint32_t address, int alignment) {
    alignment--;
    return reinterpret_cast<T*>((address + alignment) & ~alignment);
}

int currentLine {0};

struct Configuration {
    PhysicalMemStats physicalMemoryStats;
    uint64_t moduleStart {0};
    uint64_t moduleEnd {0};
    int maxPrograms {1};
    Elf::Program programs[1];
    Multiboot::MemoryMap* map;
    bool foundMemoryMap {false};
    bool foundKernelModule {false};
    int currentProgram {0};
    int kernelProgram {-1};
};

int strcmp(const char* lhs, const char* rhs) {
    while (*lhs != '\0' && *rhs != '\0') {
        if (static_cast<unsigned char>(*lhs) < static_cast<unsigned char>(*rhs)) {
            return -1;
        }
        else if (*lhs > *rhs) {
            return 1;
        }

        lhs++;
        rhs++;
    }

    return 0;
}

void loadModule(Multiboot::Modules* tag, Configuration& config) {
    #if VERBOSE
        printInteger(tag->start, currentLine, 0);
        printInteger(tag->end, currentLine, 11);
        currentLine++;
        printString(reinterpret_cast<const char*>(&tag->moduleStringStart), currentLine, 0);
        currentLine++;
    #endif
    
    if (tag->start < config.moduleStart 
        || config.moduleStart == 0) {
        config.moduleStart = tag->start;
    }

    if (tag->end > config.moduleEnd 
        || config.moduleEnd == 0) {
        config.moduleEnd = tag->end;
    }

    if (config.currentProgram == config.maxPrograms) {
        panic("Found too many kernel modules");
    }

    if (!Elf::loadElfExecutable(tag->start, config.programs[config.currentProgram])) {
        panic("Kernel module's ELF header is broken");
    }

    if (strcmp(reinterpret_cast<const char*>(&tag->moduleStringStart)
        , "SATURN_KERNEL") == 0) {
        config.kernelProgram = config.currentProgram;
        config.foundKernelModule = true;
    }

    config.currentProgram++;
}

void handleBasicMemory(Multiboot::BasicMemoryInfo* tag) {
    printString("Lower Mem (kB): ", currentLine);
    printInteger(tag->lowerAmount, currentLine, 16);
    printString("Upper Mem (kB): ", currentLine, 27);
    printInteger(tag->upperAmount, currentLine, 43);
    currentLine++;
}

void printMemoryMap(Multiboot::MemoryMap* tag) {
    using namespace Multiboot;
    auto count = (tag->size - sizeof(*tag)) / sizeof(MemoryMapEntry);

    for (decltype(count) i = 0; i < count; i++) {
        auto ptr = reinterpret_cast<MemoryMapEntry*>(&tag->entries) ;
        auto entry = ptr + i;
        printString("Base Address: ", currentLine, 0);
        printInteger(entry->baseAddress >> 32, currentLine, 14);
        printInteger(entry->baseAddress & 0xFFFFFFFF, currentLine, 25);
        currentLine++;
        printString("Length: ", currentLine, 0);
        printInteger(entry->length >> 32, currentLine, 8);
        printInteger(entry->length & 0xFFFFFFFF, currentLine, 19);
        currentLine++;
        printString("Type: ", currentLine, 0);
        printInteger(entry->type, currentLine, 6);
        currentLine++;
    }

    currentLine++;
}

void createFreePageList(Configuration& config) {

    using namespace Multiboot;
    auto count = (config.map->size - sizeof(*config.map)) / sizeof(MemoryMapEntry);

    for (decltype(count) i = 0; i < count; i++) {
        auto ptr = reinterpret_cast<MemoryMapEntry*>(&config.map->entries) ;
        auto entry = ptr + i;

        switch (static_cast<MemoryType>(entry->type)) {
            case MemoryType::AvailableRAM: {

                #if VERBOSE
                    printString("Available: ", currentLine);
                    printInteger(entry->baseAddress, currentLine, 11);
                    currentLine++;
                #endif

                if (entry->baseAddress < 0x100000) {
                    #if VERBOSE
                        printString("First megabyte wrongly marked as available", currentLine++);
                    #endif

                    continue;
                }

                if (entry->baseAddress <= config.moduleStart
                    || entry->baseAddress <= config.moduleEnd) {  
                    
                    auto oldStartAddress = entry->baseAddress;
                    entry->baseAddress = (config.moduleEnd & ~0xFFF) + 0x1000;
                    auto skippedPages = (entry->baseAddress - oldStartAddress) / 0x1000;

                    freePhysicalPages(entry->baseAddress, (entry->length / 0x1000) - skippedPages, config.physicalMemoryStats);
                }
                else {
                    freePhysicalPages(entry->baseAddress, entry->length / 0x1000, config.physicalMemoryStats);
                }

                break;
            }
            case MemoryType::UsableACPI: {

                #if VERBOSE
                    printString("Usable: ", currentLine);
                    printInteger(entry->baseAddress, currentLine, 11);
                    currentLine++;
                #endif

                break;
            }
            case MemoryType::Reserved: {

                #if VERBOSE
                    printString("Reserved: ", currentLine);
                    printInteger(entry->baseAddress, currentLine, 11);
                    currentLine++;
                #endif

                break;
            }
            case MemoryType::Defective: {

                #if VERBOSE
                    printString("Defective: ", currentLine);
                    printInteger(entry->baseAddress, currentLine, 11);
                    currentLine++;
                #endif

                break;
            }
            default: {

                #if VERBOSE
                    printString("Unknown!", currentLine);
                    printInteger(entry->type, currentLine, 11);
                    currentLine++;
                #endif

                break;
            }
        }
    }
}

void walkMultibootTags(Multiboot::BootInfo* info, Configuration& config) {
    using namespace Multiboot;

    auto tag = reinterpret_cast<Tag*>(info + 1);

    while (static_cast<TagTypes>(tag->type) != TagTypes::End) {

        switch (static_cast<TagTypes>(tag->type)) {
            case TagTypes::BootCommandLine: {
                #if VERBOSE
                    printString("[Tag] Boot Command Line", currentLine++);
                #endif
                break;
            }
            case TagTypes::BootLoaderName: {
                #if VERBOSE
                    printString("[Tag] Boot Loader Name", currentLine++);
                #endif
                break;
            }
            case TagTypes::Modules: {
                #if VERBOSE
                    printString("[Tag] Modules", currentLine++);
                #endif

                auto module = static_cast<Modules*>(tag);
                loadModule(module, config);

                break;
            }
            case TagTypes::BasicMemory: {
                #if VERBOSE
                    printString("[Tag] Basic Memory", currentLine++);
                    handleBasicMemory(static_cast<BasicMemoryInfo*>(tag));
                #endif

                break;
            }
            case TagTypes::BIOSBootDevice: {
                #if VERBOSE
                    printString("[Tag] BIOS Boot Device", currentLine++);
                #endif
                break;
            }
            case TagTypes::MemoryMap: {
                #if VERBOSE
                    printString("[Tag] Memory Map", currentLine++);
                #endif

                config.map = static_cast<MemoryMap*>(tag);
                config.foundMemoryMap = true;
                break;
            }
            case TagTypes::VBEInfo: {
                #if VERBOSE
                    printString("[Tag] VBE Info", currentLine++);
                #endif

                break;
            }
            case TagTypes::FramebufferInfo: {
                #if VERBOSE
                    printString("[Tag] Framebuffer Info", currentLine++);
                #endif
                
                break;
            }
            case TagTypes::ELFSymbols: {
                #if VERBOSE
                    printString("[Tag] ELF Symbols", currentLine++);
                #endif

                break;
            }
            case TagTypes::APMTable: {
                #if VERBOSE
                    printString("[Tag] APM Table", currentLine++);
                #endif
                break;
            }
            case TagTypes::EFI32Table: {
                #if VERBOSE
                    printString("[Tag] EFI32 Table", currentLine++);
                #endif
                break;
            }
            case TagTypes::EFI64Table: {
                #if VERBOSE
                    printString("[Tag] EFI64 Table", currentLine++);
                #endif
                break;
            }
            case TagTypes::SMBIOSTable: {
                #if VERBOSE
                    printString("[Tag] SMBIOS Table", currentLine++);
                #endif
                break;
            }
            case TagTypes::OldRDSP: {
                #if VERBOSE
                    printString("[Tag] Old RDSP", currentLine++);
                #endif

                break;
            }
            case TagTypes::NewRDSP: {
                #if VERBOSE
                    printString("[Tag] New RDSP", currentLine++);
                #endif

                break;
            }
            case TagTypes::NetworkingInfo: {
                #if VERBOSE
                    printString("[Tag] Networking Info", currentLine++);
                #endif
                break;
            }
            case TagTypes::EFIMemoryMap: {
                #if VERBOSE
                    printString("[Tag] EFI Memory Map", currentLine++);
                #endif
                break;
            }
            case TagTypes::End: {
                break;
            }
        }        

        auto currentAddress = reinterpret_cast<uintptr_t>(tag);
        tag = alignCast<Tag>(currentAddress + tag->size, 8);
    }
}

/*
Loader shouldn't have any dependencies including libc, so
this basically replaces memset
*/
void clearScreen() {
    auto buffer = reinterpret_cast<uint16_t*>(0xb8000);

    for (int i = 0; i < 80 * 24; i++) {
        *buffer++ = 0;
    }
}

/*
Physical pages are stored in a linked list where the first 8 bytes
stores an address to the next free page.

Allocating a page means taking the head off the list, reading
that page's next pointer and setting it as the head
*/
uint64_t allocatePage(Configuration& config) {
    auto address = config.physicalMemoryStats.nextFreeAddress;
    auto nextPage = *reinterpret_cast<uint64_t*>(address);
    config.physicalMemoryStats.nextFreeAddress = nextPage;
    return address;
}

void clearPage(uint64_t* page) {
    for (int i = 0; i < 512; i++) {
        *page++ = 0;
    }
}

extern "C" void loadTopLevelPage(uint32_t address);

void setupInitialPaging(Configuration& config) {
    /*
    We set aside the first four physical pages to be used for the 
    page directory and the first page table earlier in freePhysicalPages
    */
    uint32_t pageMapLevel4Address = config.physicalMemoryStats.firstAddress;
    auto pageMapLevel4 = reinterpret_cast<uint64_t*>(pageMapLevel4Address);
    auto pageDirectoryPointer = reinterpret_cast<uint64_t*>(pageMapLevel4Address + 0x1000);
    auto pageDirectory = reinterpret_cast<uint64_t*>(pageMapLevel4Address + 0x2000);
    auto pageTable = reinterpret_cast<uint64_t*>(pageMapLevel4Address + 0x3000);

    clearPage(pageMapLevel4);
    clearPage(pageDirectoryPointer);
    clearPage(pageDirectory);
    clearPage(pageTable);

    auto flags = 3;

    *pageMapLevel4 = reinterpret_cast<uint64_t>(pageDirectoryPointer) | flags;
    *(pageMapLevel4 + 510) = reinterpret_cast<uintptr_t>(pageMapLevel4) | flags;
    *pageDirectoryPointer = reinterpret_cast<uint64_t>(pageDirectory) | flags;
    *pageDirectory = reinterpret_cast<uint64_t>(pageTable) | flags;

    /*
    Identity-map the first 2 megabytes
    */
    for (int i = 0; i < 512; i++) {
        auto page = 0x1000 * i;
        page |= flags;
        *pageTable++ = page;
    }

    //map the kernel
    pageDirectoryPointer = reinterpret_cast<uint64_t*>(allocatePage(config));
    pageDirectory = reinterpret_cast<uint64_t*>(allocatePage(config));
    pageTable = reinterpret_cast<uint64_t*>(allocatePage(config));

    clearPage(pageDirectoryPointer);
    clearPage(pageDirectory);
    clearPage(pageTable);

    auto kernelVMA = config.programs[config.kernelProgram].destinationVirtualAddress;
    auto pml4 = (kernelVMA >> 39) & 511;
    auto pdp = (kernelVMA >> 30) & 511;
    auto pd = (kernelVMA >> 21) & 511;
    auto totalPages = (config.programs[config.kernelProgram].length / 0x1000) + 1;

    *(pageMapLevel4 + pml4) = reinterpret_cast<uint64_t>(pageDirectoryPointer) | flags;
    *(pageDirectoryPointer + pdp) = reinterpret_cast<uint64_t>(pageDirectory) | flags;
    *(pageDirectory + pd) = reinterpret_cast<uint64_t>(pageTable) | flags;

    for (auto i = 0u; i < totalPages; i++) {
        auto page = config.programs[config.kernelProgram].sourcePhysicalAddress + i * 0x1000;
        page |= flags;
        *(pageTable + i) = page;
    }

    loadTopLevelPage(pageMapLevel4Address);
}

void checkForLongMode() {
    
    /*
    Check if CPU supports extended processor info/feature bits
    */
    uint32_t highestExtendedFunction = 0;
    asm("movl $0x80000000, %%eax \n"
        "cpuid"
        : "=a" (highestExtendedFunction));

    if (highestExtendedFunction < 0x8000'0001) {
        panic("Long mode is not supported");
    }

    /*
    Check extended processor info/feature bits
    */
    uint32_t edx = 0;
    asm("movl $0x80000001, %%eax \n"
        "cpuid"
        : "=d" (edx));

    /*
    bit 29 is LM (Long Mode) in register EDX
    */
    if (!(edx & (1 << 29))) {
        panic("Long mode is not supported");
    }
}

extern "C" void finalEnter(uint64_t entryPoint, uint64_t nextFreeAddress, uint64_t totalFreePages);
void enterLongMode(uint64_t entryPoint, uint64_t nextFreeAddress, uint64_t totalFreePages) {
    GDT::setup64();
    finalEnter(entryPoint, nextFreeAddress, totalFreePages);
}

extern "C"
void startup(uint32_t address, uint32_t magicNumber) {
   
    auto EXPECTED_MAGIC_NUMBER = 0x36d76289u;

    if (magicNumber != EXPECTED_MAGIC_NUMBER) {
        panic("Multiboot magic number invalid");
    }

    clearScreen();

    checkForLongMode();

    IDT::setup();
    GDT::setup32();

    Configuration config;
    walkMultibootTags(reinterpret_cast<Multiboot::BootInfo*>(address), config);

    if (!config.foundMemoryMap) {
        panic("Error: loader did not find memory map");
    }

    if (!config.foundKernelModule) {
        panic("Error: loader did not find Saturn kernel module");
    }

    if (config.currentProgram != config.maxPrograms) {
        panic("Error: loader expected to find more kernel modules");
    }

    createFreePageList(config);
    setupInitialPaging(config);
    
    checkForLongMode();
    enterLongMode(config.programs[config.kernelProgram].entryPoint, 
        config.physicalMemoryStats.nextFreeAddress,
        config.physicalMemoryStats.totalPages);

    panic("Should never get here");
}