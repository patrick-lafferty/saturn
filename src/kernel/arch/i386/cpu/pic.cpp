/*
Copyright (c) 2017, Patrick Lafferty
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
#include "pic.h"

namespace PIC {

    void disable() {
        remapIRQs();
        maskAllInterrupts();
    }

    void remapIRQs() {
        /*
        Initialization Control Word 1
        Bit 0: we want to send IC4 during initialization
        Bit 1: one pic = set, 2+ pics = unset
        Bit 2: Ignored by x86
        Bit 3: Level triggered = 1, Edge Triggered = 0
        Bit 4: Must be 1 to initialize pic
        Bits 5-7: Must be 0
        */
        const uint8_t initializationControlWord1 = 0b00010001;

        asm("outb %0, %1"
            : //no output
            : "a" (initializationControlWord1),
              "Nd" (MasterCommandRegister));

        asm("outb %0, %1"
            : //no output
            : "a" (initializationControlWord1),
              "Nd" (SlaveCommandRegister));

        /*
        Initialization Control Word 2
        Bits 0-2: Must be 0
        Bits 3-7 Interrupt Vector Address
        */
        //mapping IRQs 0 to 7 to interrupts 32 to 39
        uint8_t remappedIrq = 32;
        asm("outb %0, %1"
            : //no output
            : "a"(remappedIrq), "Nd"(MasterDataRegister));
        //mapping IRQs 8 to 15 to interrupts 40 to 48
        remappedIrq = 40;
        asm("outb %0, %1"
            : //no output
            : "a"(remappedIrq), "Nd"(SlaveDataRegister));

        /*
        Initialization Control Word 3
        
        For master: used to let master know what IRQ line is connected to slave
        For slave: the IRQ the master uses to connect to

        Note: x86 uses IRQ line 2 to connect master to slave
        Line 2 is bit 3, 100 in binary is 8
        For the slave we write the line in binary notation
        eg IRQ0 is 000, IRQ1 is 011, IRQ2 is 010 etc
        */
        uint8_t icw3 = 4;
        asm("outb %0, %1"
            : //no output
            : "a"(icw3), "Nd" (MasterDataRegister));

        icw3 = 2;
        asm("outb %0, %1"
            : //no output
            : "a"(icw3), "Nd" (SlaveDataRegister));

        /*
        Initialization Control Word 4
        Bit 0: Is in 80x86 mode or not
        Bit 1: Automatically performs End of Interrupt if set
        Bit 2: [Only used if bit 3 set] Buffer Master = 1, Buffer Slave = 0
        Bit 3: If set use buffered mode
        Bit 4: Special fully nested mode [not used in x86]
        Bits 5-7: Must be 0
        */
        uint8_t icw4 = 1;
        asm("outb %0, %1"
            : //no output
            : "a" (icw4), "Nd"(MasterDataRegister));

        asm("outb %0, %1"
            : //no output
            : "a"(icw4), "Nd" (SlaveDataRegister));

    }

    void maskAllInterrupts() {
        uint8_t mask = 0xFF;
        asm("outb %0, %1 \n"
            "outb %0, %2"
            : //no output
            : "a"(mask), "Nd"(MasterDataRegister), "Nd" (SlaveDataRegister));
    }
}