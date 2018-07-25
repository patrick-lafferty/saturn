#include "multiboot.h"

void printInteger(uint32_t i, int line = 0, int column = 0, bool isNegative = false, int base = 10, bool upper = false) {
    char buffer[8 * sizeof(int) - 1];
    char hexDigits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f'};
    int digits {0};

    do {
        buffer[digits] = hexDigits[i % base];

        if (upper && buffer[digits] >= 'a') {
            buffer[digits] -= 32;
        }

        digits++;
        i /= base;
    } while (i > 0);

    if (isNegative) {
        buffer[digits++] = '-';
    }

    digits--;

    auto position = line * 80 + column;
    auto screen = reinterpret_cast<uint16_t*>(0xb8000) + position;

    while (digits >= 0) {
        *screen++ = buffer[digits] | (0xF << 8);
        digits--;
    }
}

void printString(const char* message, int line = 0, int column = 0) {

    auto position = line * 80 + column;
    auto buffer = reinterpret_cast<uint16_t*>(0xb8000) + position;

    while(message && *message != '\0') {
        *buffer++ = *message++ | (0xF << 8);
    }
}

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
    uintptr_t nextFreeAddress;
    uint32_t totalPages;
    uint64_t acpiLocation;
};

const int PageSize = 0x1000;

void freePhysicalPage(uintptr_t pageAddress, uint32_t count, PhysicalMemStats& stats) {

    for (uint32_t i = 0; i < count; i++) {
        auto page = reinterpret_cast<uint32_t volatile*>(pageAddress & ~0xfff);
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
    printInteger(tag->start, currentLine, 0);
    printInteger(tag->end, currentLine, 11);
    currentLine++;
    printString(reinterpret_cast<const char*>(&tag->moduleStringStart), currentLine, 0);
    currentLine++;
}

void handleBasicMemory(Multiboot::BasicMemoryInfo* tag) {
    printString("Lower Mem (kB): ", currentLine);
    printInteger(tag->lowerAmount, currentLine, 16);
    printString("Upper Mem (kB): ", currentLine, 27);
    printInteger(tag->upperAmount, currentLine, 43);
    currentLine++;
}

void handleMemoryMap(Multiboot::MemoryMap* tag) {
    using namespace Multiboot;
    auto count = (tag->size - sizeof(*tag)) / sizeof(MemoryMapEntry);

    for (int i = 0; i < count; i++) {
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

void walkMultibootTags(Multiboot::BootInfo* info) {
    using namespace Multiboot;

    auto tag = reinterpret_cast<Tag*>(info + 1);

    while (static_cast<TagTypes>(tag->type) != TagTypes::End) {

        switch (static_cast<TagTypes>(tag->type)) {
            case TagTypes::BootCommandLine: 
            case TagTypes::BootLoaderName: {
                break;
            }
            case TagTypes::Modules: {
                printString("[Tag] Modules", currentLine++);
                handleModules(static_cast<Modules*>(tag));
                break;
            }
            case TagTypes::BasicMemory: {
                printString("[Tag] Basic Memory", currentLine++);
                handleBasicMemory(static_cast<BasicMemoryInfo*>(tag));
                break;
            }
            case TagTypes::BIOSBootDevice: {
                break;
            }
            case TagTypes::MemoryMap: {
                handleMemoryMap(static_cast<MemoryMap*>(tag));
                break;
            }
            case TagTypes::VBEInfo: {
                printString("[Tag] VBE Info", currentLine++);
                auto& value = *static_cast<VBEInfo*>(tag);
                break;
            }
            case TagTypes::FramebufferInfo: {
                printString("[Tag] Framebuffer Info", currentLine++);
                auto& value = *static_cast<FramebufferInfo*>(tag);
                break;
            }
            case TagTypes::ELFSymbols: {
                printString("[Tag] ELF Symbols", currentLine++);
                auto& value = *static_cast<ElfSymbols*>(tag);
                break;
            }
            case TagTypes::APMTable: {
                break;
            }
            case TagTypes::EFI32Table: {
                break;
            }
            case TagTypes::EFI64Table: {
                break;
            }
            case TagTypes::SMBIOSTable: {
                break;
            }
            case TagTypes::OldRDSP: {
                printString("[Tag] Old RDSP", currentLine++);
                auto& value = *static_cast<OldRDSP*>(tag);
                break;
            }
            case TagTypes::NewRDSP: {
                break;
            }
            case TagTypes::NetworkingInfo: {
                break;
            }
            case TagTypes::EFIMemoryMap: {
                break;
            }
        }        

        auto currentAddress = reinterpret_cast<uintptr_t>(tag);
        tag = alignCast<Tag>(currentAddress + tag->size, 8);
    }
}

void clearScreen() {
    auto buffer = reinterpret_cast<uint16_t*>(0xb8000);

    for (int i = 0; i < 80 * 24; i++) {
        *buffer++ = 0;
    }
}

extern "C"
void startup(uint32_t address, uint32_t magicNumber) {
   
    auto EXPECTED_MAGIC_NUMBER = 0x36d76289;

    if (magicNumber != EXPECTED_MAGIC_NUMBER) {
        panic("Multiboot magic number invalid");
    }

    clearScreen();

    walkMultibootTags(reinterpret_cast<Multiboot::BootInfo*>(address));

    printString("Startup has finished successfully", currentLine);
}