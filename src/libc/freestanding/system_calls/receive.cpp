#include <system_calls.h>

void receive(IPC::Message* buffer) {
    uint32_t systemCall = 4;

    asm volatile("int $0xFF"
        : //no outputs
        : "a" (systemCall),
          "b" (buffer));
}