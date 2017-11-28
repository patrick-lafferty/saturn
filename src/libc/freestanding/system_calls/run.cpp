#include <system_calls.h>
#include <services.h>

using namespace Kernel;

void run(uintptr_t entryPoint) {
    RunProgram run;
    run.entryPoint = entryPoint;
    send(IPC::RecipientType::ServiceRegistryMailbox, &run);
}