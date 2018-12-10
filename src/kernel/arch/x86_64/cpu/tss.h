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
#pragma once

#include <stdint.h>

namespace Memory {
    template<class T> class BlockAllocator;
}

namespace CPU {

    struct TSS {
        uint32_t ignore;
        uint32_t rsp0_low;
        uint32_t rsp0_high;
        uint32_t rsp1_low;
        uint32_t rsp1_high;
        uint32_t rsp2_low;
        uint32_t rsp2_high;
        uint64_t ignore2;
        uint32_t ist1_low;
        uint32_t ist1_high;
        uint32_t ist2_low;
        uint32_t ist2_high;
        uint32_t ist3_low;
        uint32_t ist3_high;
        uint32_t ist4_low;
        uint32_t ist4_high;
        uint32_t ist5_low;
        uint32_t ist5_high;
        uint32_t ist6_low;
        uint32_t ist6_high;
        uint32_t ist7_low;
        uint32_t ist7_high;
        uint64_t ignore3;
        uint16_t ignore4;
        uint16_t iopbAddress;
    } __attribute__((packed));

    void setupTSS(Memory::BlockAllocator<TSS>& allocator);
    void loadTSSRegister();
}