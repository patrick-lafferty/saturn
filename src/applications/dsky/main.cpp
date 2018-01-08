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

    int colourId = 0;
    uint32_t colours[] = {
        0xFF'00'00'00, 
        0x00'FF'00'00, 
        0x00'00'FF'00, 
        0x00'00'00'FF, 
    };

    auto renderer = Window::Text::createRenderer(windowBuffer->buffer);
    renderer->drawText("Hello, world!");

    while (true) {

        Update update;
        update.serviceType = Kernel::ServiceType::WindowManager;
        update.x = 0;
        update.y = 0;
        update.width = 800;
        update.height = 600;

        /*for (int y = 0; y < 100; y++) {
            for (int x = 0; x < 100; x++) {
                windowBuffer->buffer[x + y * 800] = colours[colourId]; 
            }
        }*/

        send(IPC::RecipientType::ServiceName, &update);

        colourId++;

        if (colourId == 4) {
            colourId = 0;
        }

        sleep(100);
    }
}