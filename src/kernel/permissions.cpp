#include "permissions.h"
#include <stdio.h>

namespace Kernel {
    
    void grantIOPort8(uint16_t port, uint8_t volatile* iopb) {
        auto index = port / 8;
        auto bit = port % 8;

        auto byte = iopb + index;
        *byte &= ~(1 << bit);
    }
    
    void grantIOPort16(uint16_t port, uint8_t volatile* iopb) {
        grantIOPort8(port, iopb);
        grantIOPort8(port + 1, iopb);
    }

    void blockIOPort16(uint16_t port, uint8_t volatile* iopb) {
        auto index = port / 8;
        auto bit = port % 8;

        auto byte = iopb + index;
        *byte |= (1 << bit);

        if (bit == 7) {
            byte++;
            *byte |= 1;
        }
        else {
            *byte |= (1 << (bit + 1));
        }
    }

    void grantIOPortRange(uint16_t portStart, uint16_t portEnd, uint8_t volatile* iopb) {

        for (auto port = portStart; port != portEnd + 1; port++) {
            grantIOPort8(port, iopb);
        }
    }
}