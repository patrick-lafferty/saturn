#include "permissions.h"

namespace Kernel {
    
    void grantIOPort16(uint16_t port, uint8_t volatile* iopb) {
        auto index = port / 8;
        auto bit = port % 8;

        auto byte = iopb + index;
        *byte &= ~(1 << bit);

        if (bit == 7) {
            byte++;
            *byte &= ~1;
        }
        else {
            *byte &= ~(1 << (bit + 1));
        }
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
}