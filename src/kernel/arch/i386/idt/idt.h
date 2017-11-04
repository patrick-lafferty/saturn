#pragma once

#include <stdint.h>
#include <cpu/stack_frames.h>

namespace IDT {
    struct Entry {
        uint16_t baseLow;
        uint16_t kernelSegment;
        uint8_t constantZero {0};
        uint8_t flags;
        uint16_t baseHigh;
    } __attribute__((packed));

    struct EntryPointer {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed));

    void setup();
    
    Entry encodeEntry(uint32_t base, uint16_t kernelSegment);
}

extern "C" IDT::Entry idt[256];
extern "C" IDT::EntryPointer idtPointer;
extern "C" void loadIDT();
extern "C" void interruptHandler(CPU::InterruptStackFrame* frame);

//stubs from isr_stubs.s
extern "C" void isr0();
extern "C" void isr1();
extern "C" void isr2();
extern "C" void isr3();
extern "C" void isr4();
extern "C" void isr5();
extern "C" void isr6();
extern "C" void isr7();
extern "C" void isr8();
extern "C" void isr9();
extern "C" void isr10();
extern "C" void isr11();
extern "C" void isr12();
extern "C" void isr13();
extern "C" void isr14();
extern "C" void isr15();
extern "C" void isr16();
extern "C" void isr17();
extern "C" void isr18();
extern "C" void isr19();
extern "C" void isr20();

//remapped IRQs
extern "C" void isr32();
extern "C" void isr33();
extern "C" void isr34();
extern "C" void isr35();
extern "C" void isr36();
extern "C" void isr37();
extern "C" void isr38();
extern "C" void isr39();
extern "C" void isr40();
extern "C" void isr41();
extern "C" void isr42();
extern "C" void isr43();
extern "C" void isr44();
extern "C" void isr45();
extern "C" void isr46();
extern "C" void isr47();

//APIC
extern "C" void isr48();
extern "C" void isr49();
extern "C" void isr50();
extern "C" void isr51();
extern "C" void isr52();
extern "C" void isr53();
extern "C" void isr54();
extern "C" void isr55();
extern "C" void isr56();
extern "C" void isr57();
extern "C" void isr58();
extern "C" void isr59();
extern "C" void isr255(); //APIC spurious interrupt