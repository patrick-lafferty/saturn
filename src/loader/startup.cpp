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

/*
Used when the Saturn loader encounters an unrecoverable error
Prints a message and then halts
*/
[[noreturn]]
void panic(const char* message) {
    asm volatile("cli");

    printString(message);

    while (true) {
        asm volatile("hlt");
    }
}

struct PhysicalMemStats {
    uint64_t nextFreeAddress;
    uint64_t totalPages;
    uint64_t acpiLocation;
};

const int PageSize = 0x1000;

void freePhysicalPages(uint64_t pageAddress, uint64_t count, PhysicalMemStats& stats) {

    for (uint64_t i = 0; i < count; i++) {
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

void handleModules(Multiboot::Modules* tag) {
    #if VERBOSE
        printInteger(tag->start, currentLine, 0);
        printInteger(tag->end, currentLine, 11);
        currentLine++;
        printString(reinterpret_cast<const char*>(&tag->moduleStringStart), currentLine, 0);
        currentLine++;
    #endif

    if (!Elf::loadElfExecutable(tag->start)) {
        panic("Kernel module's ELF header is broken");
    }
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

void createFreePageList(Multiboot::MemoryMap* tag, PhysicalMemStats& stats) {

    using namespace Multiboot;
    auto count = (tag->size - sizeof(*tag)) / sizeof(MemoryMapEntry);

    for (decltype(count) i = 0; i < count; i++) {
        auto ptr = reinterpret_cast<MemoryMapEntry*>(&tag->entries) ;
        auto entry = ptr + i;

        switch (static_cast<MemoryType>(entry->type)) {
            case MemoryType::AvailableRAM: {

                #if VERBOSE
                    printString("Available: ", currentLine);
                    printInteger(entry->baseAddress, currentLine, 11);
                    currentLine++;
                #endif

                if (entry->baseAddress <= 0x100000) {
                    #if VERBOSE
                        printString("First megabyte wrongly marked as available", currentLine++);
                    #endif

                    continue;
                }

                freePhysicalPages(entry->baseAddress, entry->length / 0x1000, stats);

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

struct Configuration {
    PhysicalMemStats physicalMemoryStats;
};

Configuration walkMultibootTags(Multiboot::BootInfo* info) {
    using namespace Multiboot;
    Configuration config;

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

                handleModules(static_cast<Modules*>(tag));
                break;
            }
            case TagTypes::BasicMemory: {
                #if VERBOSE
                    printString("[Tag] Basic Memory", currentLine++);
                #endif

                handleBasicMemory(static_cast<BasicMemoryInfo*>(tag));
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

                createFreePageList(static_cast<MemoryMap*>(tag), config.physicalMemoryStats);
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

    return config;
}

void clearScreen() {
    auto buffer = reinterpret_cast<uint16_t*>(0xb8000);

    for (int i = 0; i < 80 * 24; i++) {
        *buffer++ = 0;
    }
}

extern "C"
void startup(uint32_t address, uint32_t magicNumber) {
   
    auto EXPECTED_MAGIC_NUMBER = 0x36d76289u;

    if (magicNumber != EXPECTED_MAGIC_NUMBER) {
        panic("Multiboot magic number invalid");
    }

    clearScreen();

    const auto& config = walkMultibootTags(reinterpret_cast<Multiboot::BootInfo*>(address));

    printString("Startup has finished successfully", currentLine);
}