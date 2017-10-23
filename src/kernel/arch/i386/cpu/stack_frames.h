#pragma once

#include <stdint.h>

namespace CPU {
    struct InterruptStackFrame {
        uint32_t gs, fs, es, ds;
        uint32_t edi, esi, ebp, esp;
        uint32_t ebx, edx, ecx, eax;
        uint32_t interruptNumber;
        uint32_t errorCode;
        uint32_t eip, cs, eflags, resp, ss;
    };
}