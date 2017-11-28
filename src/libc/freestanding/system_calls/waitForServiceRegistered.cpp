#include <system_calls.h>
#include <services.h>

void waitForServiceRegistered(Kernel::ServiceType type) {
    Kernel::SubscribeServiceRegistered subscribe;
    subscribe.type = type;

    send(IPC::RecipientType::ServiceRegistryMailbox, &subscribe);

    while (true) {
        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageId == Kernel::NotifyServiceRegistered::MessageId) {
            auto notify = IPC::extractMessage<Kernel::NotifyServiceRegistered>(buffer);

            if (notify.type == type) {
                return;
            }
        }
    }
}