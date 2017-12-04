#pragma once

#include <stdint.h>

namespace Kernel {

    //TODO: port8/32 bit variants
    void grantIOPort16(uint16_t port, uint8_t volatile* iopb);
    void blockIOPort16(uint16_t port, uint8_t volatile* iopb); 
}