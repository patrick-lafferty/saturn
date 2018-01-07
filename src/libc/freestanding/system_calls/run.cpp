#include <system_calls.h>
#include <services.h>

using namespace Kernel;

uint32_t run(uintptr_t entryPoint) {
    RunProgram run;
    run.entryPoint = entryPoint;
    send(IPC::RecipientType::ServiceRegistryMailbox, &run);

    IPC::MaximumMessageBuffer buffer;
    receive(&buffer);

    if (buffer.messageNamespace == IPC::MessageNamespace::ServiceRegistry 
        && buffer.messageId == static_cast<uint32_t>(MessageId::RunResult)) {
        auto result = IPC::extractMessage<RunResult>(buffer);
        return result.pid;
    }

    return 0;
}

uint32_t run(char* path) {
    RunProgram run;
    memcpy(run.path, path, strlen(path));
    send(IPC::RecipientType::ServiceRegistryMailbox, &run);

    IPC::MaximumMessageBuffer buffer;
    receive(&buffer);

    if (buffer.messageNamespace == IPC::MessageNamespace::ServiceRegistry 
        && buffer.messageId == static_cast<uint32_t>(MessageId::RunResult)) {
        auto result = IPC::extractMessage<RunResult>(buffer);
        return result.pid;
    }

    return 0;
}