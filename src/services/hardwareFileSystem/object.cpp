#include "object.h"
#include <services/virtualFileSystem/virtualFileSystem.h>

using namespace VFS;

namespace HardwareFileSystem {
    void replyWriteSucceeded(uint32_t requesterTaskId, uint32_t requestId, bool success) {
        WriteResult result;
        result.requestId = requestId;
        result.success = success;
        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }
}