#pragma once

#include <stdint.h>

namespace Kernel {
    struct TaskContext {
        uint32_t esp;
        uint32_t kernelESP;
    };
}