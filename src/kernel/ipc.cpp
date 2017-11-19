#include <ipc.h>

namespace IPC {
    uint32_t getId() {
        static uint32_t messageId {0};
        return messageId++;
    }
}