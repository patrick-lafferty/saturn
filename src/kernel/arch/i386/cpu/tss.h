#pragma once

#include <stdint.h>

namespace CPU {
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
        uint8_t ioPermissionBitmap[0xf90];
    } __attribute__((packed));

    TSS* setupTSS(uint32_t address);
}

extern "C" void fillTSS(CPU::TSS* tss);
extern "C" void loadTSS();