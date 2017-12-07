#pragma once

#include <stdint.h>

namespace Kernel {

    void grantIOPort8(uint16_t port, uint8_t volatile* iopb);
    void grantIOPort16(uint16_t port, uint8_t volatile* iopb);
    void blockIOPort16(uint16_t port, uint8_t volatile* iopb); 

    void grantIOPortRange(uint16_t portStart, uint16_t portEnd, uint8_t volatile* iopb);
}