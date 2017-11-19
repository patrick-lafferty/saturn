#include "vga.h"
#include <services.h>
#include "terminal.h"
#include <system_calls.h>

using namespace Kernel;

namespace VGA {

    uint32_t PrintMessage::MessageId;

    void registerMessages() {
        IPC::registerMessage<PrintMessage>();
    }

    void messageLoop(Terminal* terminal) {
        IPC::MaximumMessageBuffer buffer;

        while (true) {
            receive(&buffer);

            if (buffer.messageId == PrintMessage::MessageId) {
                auto message = IPC::extractMessage<PrintMessage>(buffer);

                auto iterator = message.buffer;

                while (*iterator) {
                    terminal->writeCharacter(*iterator++);
                }
            }
        }
    }

    void service() {
        RegisterService registerRequest {};
        registerRequest.type = ServiceType::VGA;

        IPC::MaximumMessageBuffer buffer;
        Terminal* terminal {nullptr};

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
        receive(&buffer);

        if (buffer.messageId == RegisterServiceDenied::MessageId) {
            //can't print to screen, how to notify?
            print(99, 0);   
        }
        else if (buffer.messageId == VGAServiceMeta::MessageId) {
            registerMessages();

            auto vgaMeta = IPC::extractMessage<VGAServiceMeta>(buffer);
            terminal = new Terminal(reinterpret_cast<uint16_t*>(vgaMeta.vgaAddress));
            
            messageLoop(terminal);
        }
    }

    uint8_t getColour(Colours foreground, Colours background) {
        return static_cast<uint8_t>(foreground) | static_cast<uint8_t>(background) << 4;
    }

    uint16_t prepareCharacter(uint8_t character, uint8_t colour) {
        return static_cast<uint16_t>(character) | static_cast<uint16_t>(colour) << 8;
    }
}