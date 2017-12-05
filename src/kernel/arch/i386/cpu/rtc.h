#pragma once

#include <stdint.h>

namespace RTC {

    void enable(uint8_t rate);
    void disable();
    void reset();
}   