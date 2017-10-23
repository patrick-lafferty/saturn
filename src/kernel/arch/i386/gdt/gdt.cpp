#include "gdt.h"

GDT::Descriptor gdt[3];
GDT::DescriptorPointer gp;

namespace GDT {

    void setup() {
        gp.limit = sizeof(Descriptor) * 3 - 1;
        gp.base = reinterpret_cast<uint32_t>(&gdt);

        gdt[0] = encodeEntry(0, 0, 0, 0);
        gdt[1] = encodeEntry(0, 0xFFFFFFFF, 0x9A, 0xCF);
        gdt[2] = encodeEntry(0, 0xFFFFFFFF, 0x92, 0xCF);

        gdt_flush();
    }

    Descriptor encodeEntry(uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity) {
        Descriptor descriptor;

        descriptor.baseLow = base & 0xFFFF;
        descriptor.baseMiddle = (base >> 16) & 0xFF;
        descriptor.baseHigh = (base >> 24) & 0xFF;

        descriptor.limitLow = limit & 0xFFFF;
        descriptor.granularity = (limit >> 16) & 0x0F;

        descriptor.granularity |= granularity & 0xF0;
        descriptor.access = access;

        return descriptor;
    }
}