#include <system_calls.h>
#include <services.h>

using namespace Kernel;

uint32_t run(uintptr_t entryPoint) {
    RunProgram run;
    run.entryPoint = entryPoint;
    send(IPC::RecipientType::ServiceRegistryMailbox, &run);

    IPC::MaximumMessageBuffer buffer;
    receive(&buffer);

    if (buffer.messageId == RunResult::MessageId) {
        auto result = IPC::extractMessage<RunResult>(buffer);
        return result.pid;
    }

    return 0;
}