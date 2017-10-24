#pragma once

#include <stdint.h>

extern "C" void cpuid_vendor();
extern "C" void cpuid_vendor_impl(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

namespace CPUID {

    void printVersionInformation();
}
