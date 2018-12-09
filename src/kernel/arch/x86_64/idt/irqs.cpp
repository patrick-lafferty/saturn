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
#include "irqs.h"
#include "descriptor.h"
#include <log.h>
#include <cpu/apic.h>

namespace IDT {

    void loadIRQs() {
        idt[206] = encodeEntry(reinterpret_cast<uintptr_t>(&irq0), 0x08);
        idt[207] = encodeEntry(reinterpret_cast<uintptr_t>(&irq1), 0x08);
        idt[49] = encodeEntry(reinterpret_cast<uintptr_t>(&irq2), 0x08);
        idt[35] = encodeEntry(reinterpret_cast<uintptr_t>(&irq3), 0x08);
        idt[36] = encodeEntry(reinterpret_cast<uintptr_t>(&irq4), 0x08);
        idt[37] = encodeEntry(reinterpret_cast<uintptr_t>(&irq5), 0x08);
        idt[38] = encodeEntry(reinterpret_cast<uintptr_t>(&irq6), 0x08);
        idt[39] = encodeEntry(reinterpret_cast<uintptr_t>(&irq7), 0x08);
        idt[51] = encodeEntry(reinterpret_cast<uintptr_t>(&irq8), 0x08);
        idt[41] = encodeEntry(reinterpret_cast<uintptr_t>(&irq9), 0x08);
        idt[42] = encodeEntry(reinterpret_cast<uintptr_t>(&irq10), 0x08);
        idt[43] = encodeEntry(reinterpret_cast<uintptr_t>(&irq11), 0x08);
        idt[44] = encodeEntry(reinterpret_cast<uintptr_t>(&irq12), 0x08);
        idt[45] = encodeEntry(reinterpret_cast<uintptr_t>(&irq13), 0x08);
        idt[46] = encodeEntry(reinterpret_cast<uintptr_t>(&irq14), 0x08);
        idt[47] = encodeEntry(reinterpret_cast<uintptr_t>(&irq15), 0x08);
    }
}

void irqHandler(IrqFrame* frame) {

   log("irq %d", frame->index);
   APIC::signalEndOfInterrupt();
}
