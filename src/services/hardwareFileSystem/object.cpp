#include "object.h"
#include <services/virtualFileSystem/virtualFileSystem.h>

using namespace VirtualFileSystem;
using namespace Vostok;

namespace HardwareFileSystem {
    void replyWriteSucceeded(uint32_t requesterTaskId, uint32_t requestId, bool success) {
        WriteResult result;
        result.requestId = requestId;
        result.success = success;
        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    int HardwareObject::getFunction(std::string_view) {
        return -1;
    }

    void HardwareObject::readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) {
        describeFunction(requesterTaskId, requestId, functionId);
    }

    void HardwareObject::writeFunction(uint32_t, uint32_t, uint32_t, ArgBuffer&) {
    }

    void HardwareObject::describeFunction(uint32_t, uint32_t, uint32_t) {
    }

    Object* HardwareObject::getNestedObject(std::string_view) {
        return nullptr;
    }
}