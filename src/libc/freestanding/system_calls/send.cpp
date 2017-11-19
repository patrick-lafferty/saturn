#include <system_calls.h>
#include <ipc.h>

void send(IPC::RecipientType recipient, IPC::Message* message) {
    uint32_t systemCall = 3;

    asm volatile("int $0xFF"
        : //no outputs
        : "a" (systemCall),
          "b" (recipient),
          "c" (message));
}