#include <system_calls.h>
#include <services.h>

using namespace Kernel;

void* map(uint32_t address, uint32_t size, uint32_t flags) {
    MapMemory request;
    request.address = address;
    request.size = size;
    request.flags = flags;

    send(IPC::RecipientType::ServiceRegistryMailbox, &request);

    IPC::MaximumMessageBuffer buffer;
    receive(&buffer);

    if (buffer.messageNamespace == IPC::MessageNamespace::ServiceRegistry
            && buffer.messageId == static_cast<uint32_t>(MessageId::MapMemoryResult)) {
        return IPC::extractMessage<MapMemoryResult>(buffer).start;
    }
    else {
        return nullptr;
    }
}