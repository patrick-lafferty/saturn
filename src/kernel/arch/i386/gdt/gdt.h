#pragma once

#include <stdint.h>

namespace GDT {

    struct Descriptor {
        uint16_t limitLow;
        uint16_t baseLow;
        uint8_t baseMiddle;
        uint8_t access;
        uint8_t granularity;
        uint8_t baseHigh;
    } __attribute__((packed));

    struct DescriptorPointer {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed));

    void setup();

    Descriptor encodeEntry(uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity);
}

extern "C" GDT::Descriptor gdt[3];
extern "C" GDT::DescriptorPointer gp;

extern "C" void gdt_flush();