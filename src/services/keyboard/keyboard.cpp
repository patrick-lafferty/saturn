#include "keyboard.h"
#include <services.h>
#include <system_calls.h>

using namespace Kernel;

namespace Keyboard {

    void registerMessages() {

    }

    void messageLoop() {
        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);
        }
    }

    void service() {
        RegisterService registerRequest {};
        registerRequest.type = ServiceType::Keyboard;

        IPC::MaximumMessageBuffer buffer;

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
        receive(&buffer);

        if (buffer.messageId == RegisterServiceDenied::MessageId) {
            //can't register keyboard device?
        }
        else if (buffer.messageId == GenericServiceMeta::MessageId) {
            registerMessages();
            messageLoop();
        }
    }
}