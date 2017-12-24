#include <system_calls.h>
#include <services.h>

void waitForServiceRegistered(Kernel::ServiceType type) {
    Kernel::SubscribeServiceRegistered subscribe;
    subscribe.type = type;

    send(IPC::RecipientType::ServiceRegistryMailbox, &subscribe);

    while (true) {
        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageNamespace == static_cast<uint32_t>(IPC::MessageNamespace::ServiceRegistry)
                && buffer.messageId == static_cast<uint32_t>(Kernel::MessageId::NotifyServiceRegistered)) {
            auto notify = IPC::extractMessage<Kernel::NotifyServiceRegistered>(buffer);

            if (notify.type == type) {
                return;
            }
        }
    }
}