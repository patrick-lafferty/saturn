#pragma once

#include <stdint.h>

namespace GDT {

    struct Descriptor {
        uint16_t limitLow;
        uint16_t baseLow;
        uint8_t baseMiddle;
        uint8_t access;
        uint8_t flags;
        uint8_t baseHigh;
    } __attribute__((packed));

    enum class AccessDataSegment {
        Writeable = 1 << 1,
        Direction = 1 << 2,
        Present = 1 << 7
    };

    enum class AccessCodeSegment {
        Readable = 1 << 1,
        Conforming = 1 << 2,
        Executable = 1 << 3,
        Present = 1 << 7
    };

    enum class Flags {
        Size = 1 << 6,
        Granularity = 1 << 7
    };

    struct DescriptorPointer {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed));

    void setup();

    Descriptor encodeEntry(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags);
}

extern "C" GDT::Descriptor gdt[6];
extern "C" GDT::DescriptorPointer gp;

extern "C" void gdt_flush();