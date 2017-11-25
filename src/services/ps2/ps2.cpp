#include "ps2.h"
#include <services.h>
#include <system_calls.h>
#include <services/terminal/terminal.h>
#include <string.h>

using namespace Kernel;

namespace PS2 {
    uint32_t KeyboardInput::MessageId;
    uint32_t MouseInput::MessageId;

    void registerMessages() {
        IPC::registerMessage<KeyboardInput>();
        IPC::registerMessage<MouseInput>();
    }

    void printDebugString(char* s) {
        Terminal::PrintMessage message {};
        message.serviceType = Kernel::ServiceType::Terminal;
        memcpy(message.buffer, s, strlen(s));
        send(IPC::RecipientType::ServiceName, &message);
    }

    void messageLoop() {
        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == KeyboardInput::MessageId) {

            }
            else if (buffer.messageId == MouseInput::MessageId) {

            }
        }
    }

    void service() {
        RegisterService registerRequest {};
        registerRequest.type = ServiceType::PS2;

        IPC::MaximumMessageBuffer buffer;

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
        receive(&buffer);

        if (buffer.messageId == RegisterServiceDenied::MessageId) {
            //can't register ps2 device?
        }
        else if (buffer.messageId == GenericServiceMeta::MessageId) {
            registerMessages();
            messageLoop();
        }
    }
}