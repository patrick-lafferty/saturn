#include "dsky.h"
#include <services/windows/messages.h>
#include <system_calls.h>
#include <services.h>
#include <services/windows/lib/text.h>

using namespace Window;

bool createWindow(uint32_t address) {
    
    CreateWindow create;
    create.serviceType = Kernel::ServiceType::WindowManager;
    create.bufferAddress = address;
    
    send(IPC::RecipientType::ServiceName, &create);
    
    IPC::MaximumMessageBuffer buffer;
    receive(&buffer);

    switch (buffer.messageNamespace) {
        case IPC::MessageNamespace::WindowManager: {
            switch (static_cast<MessageId>(buffer.messageId)) {
                case MessageId::CreateWindowSucceeded: {

                    return true;
                    break;
                }
            }

            break;
        }
    }

    return false;
}

int dsky_main() {
    auto windowBuffer = new WindowBuffer;
    auto address = reinterpret_cast<uintptr_t>(windowBuffer);

    if (!createWindow(address)) {
        delete windowBuffer;
        return 1;
    }

    auto renderer = Window::Text::createRenderer(windowBuffer->buffer);
    renderer->drawText("abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789");

    while (true) {

        Update update;
        update.serviceType = Kernel::ServiceType::WindowManager;
        update.x = 0;
        update.y = 0;
        update.width = 800;
        update.height = 600;

        send(IPC::RecipientType::ServiceName, &update);

        sleep(100);
    }
}