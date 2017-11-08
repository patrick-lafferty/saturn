#include "gdt.h"

GDT::Descriptor gdt[6];
GDT::DescriptorPointer gp;

namespace GDT {

    template<typename... Arg> uint32_t combineFlags(Arg... args) {
        return (static_cast<uint32_t>(args) | ...);
    }

    int nextGDTIndex = 0;

    void setup() {
        gp.limit = sizeof(Descriptor) * 6 - 1;
        gp.base = reinterpret_cast<uint32_t>(&gdt);

        //null descriptor
        gdt[0] = encodeEntry(0, 0, 0, 0);

        /*
        The upper 4 bits of flags is [Gr, Sz, 0, 0], where:
        Gr = Granularity (0 = Byte granularity, 1 = 4K Page granularity)
        Sz = Size (0 = 16 bit protected mode, 1 = 32 bit protected mode)

        Normally we want Gr = 1 and Sz = 1 for regular pages
        */
        
        //kernel code segment 
        gdt[1] = encodeEntry(0, 0xFFFFF, combineFlags(
            AccessCodeSegment::Present,
            AccessCodeSegment::Executable,
            AccessCodeSegment::Readable
        ), combineFlags(Flags::Size, Flags::Granularity));
        //kernel data segment 
        gdt[2] = encodeEntry(0, 0xFFFFF, combineFlags(
            AccessDataSegment::Present,
            AccessDataSegment::Writeable
        ), combineFlags(Flags::Size, Flags::Granularity));

        //user code segment 
        gdt[3] = encodeEntry(0, 0xFFFFF, combineFlags(
            AccessCodeSegment::Present,
            AccessCodeSegment::Executable,
            AccessCodeSegment::Readable,
            AccessCodeSegment::Ring3
        ), combineFlags(Flags::Size, Flags::Granularity));
        //user data segment 
        gdt[4] = encodeEntry(0, 0xFFFFF, combineFlags(
            AccessDataSegment::Present,
            AccessDataSegment::Writeable,
            AccessDataSegment::Ring3
        ), combineFlags(Flags::Size, Flags::Granularity));
        
        nextGDTIndex = 5;
        
        gdt_flush();
    }

    Descriptor encodeEntry(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
        Descriptor descriptor;

        descriptor.baseLow = base & 0xFFFF;
        descriptor.baseMiddle = (base >> 16) & 0xFF;
        descriptor.baseHigh = (base >> 24) & 0xFF;

        descriptor.limitLow = limit & 0xFFFF;
        descriptor.flags = (limit >> 16) & 0x0F;
        descriptor.flags |= flags;
        //bit 4 is always 1
        descriptor.access = access | (1 << 4);

        return descriptor;
    }

    void addTSSEntry(uint32_t address, uint32_t size) {
        //tss per cpu (hardware thread?)
        gdt[nextGDTIndex] = encodeEntry(address, size, combineFlags(
            AccessCodeSegment::Accessed,
            AccessCodeSegment::Executable,
            AccessCodeSegment::Present
        ), combineFlags(Flags::Size));

        nextGDTIndex++;

        //TODO: is this necessary?
        gdt_flush();
    }
}