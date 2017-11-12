#include <system_calls.h>

void sleep(uint32_t milliseconds) {
    uint32_t systemCall = 1;

    asm volatile("int $0xFF"
        : //no outputs
        : "a" (systemCall), "b" (milliseconds)
    );
}