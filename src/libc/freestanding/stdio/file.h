#pragma once

#include <stdint.h>

struct _file {
    uint32_t descriptor;
    uint32_t position;
    int error;
    uint32_t length;
};