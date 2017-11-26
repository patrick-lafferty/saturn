#include "keyboard.h"
#include <services.h>
#include <system_calls.h>
#include <services/terminal/terminal.h>
#include <string.h>
#include <stdio.h>

using namespace Kernel;

namespace Keyboard {

    uint32_t KeyEvent::MessageId;

    void registerMessages() {
        IPC::registerMessage<KeyEvent>();
    }

    uint8_t en_US[128];

    void loadKeymap() {
        en_US[0x1C] = 'a';
        en_US[0x32] = 'b';
        en_US[0x21] = 'c';
        en_US[0x23] = 'd';
        en_US[0x24] = 'e';
        en_US[0x2B] = 'f';
        en_US[0x34] = 'g';
        en_US[0x33] = 'h';
        en_US[0x43] = 'i';
        en_US[0x3B] = 'j';
        en_US[0x42] = 'k';
        en_US[0x4B] = 'l';
        en_US[0x3A] = 'm';
        en_US[0x31] = 'n';
        en_US[0x44] = 'o';
        en_US[0x4D] = 'p';
        en_US[0x15] = 'q';
        en_US[0x2D] = 'r';
        en_US[0x1B] = 's';
        en_US[0x2C] = 't';
        en_US[0x3C] = 'u';
        en_US[0x2A] = 'v';
        en_US[0x1D] = 'w';
        en_US[0x22] = 'x';
        en_US[0x35] = 'y';
        en_US[0x1A] = 'z';
    }

    uint8_t translateKeyEvent(PS2::PhysicalKey key) {
        /*TODO: keymaps should be stored in files, but since
        we don't have filesystem support yet, just hardcode
        en_US in the above array
        */
        return en_US[static_cast<uint32_t>(key)];
    }

    void echo(uint8_t key) {
        Terminal::PrintMessage message {};
        message.serviceType = Kernel::ServiceType::Terminal;
        char s[] = {(char)key, '\0'};
        memcpy(message.buffer, s, strlen(s));
        send(IPC::RecipientType::ServiceName, &message);
    }

    void messageLoop() {
        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == KeyEvent::MessageId) {
                auto event = IPC::extractMessage<KeyEvent>(buffer);
                auto key = translateKeyEvent(event.key);
                echo(key);
            }
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
            loadKeymap();
            messageLoop();
        }
    }
}