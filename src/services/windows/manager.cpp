#include "manager.h"
#include "messages.h"
#include <services.h>
#include <system_calls.h>
#include <vector>
#include <list>
#include <services/startup/startup.h>

using namespace Kernel;

namespace Window {

    uint32_t registerService() {
        RegisterService registerRequest;
        registerRequest.type = ServiceType::WindowManager;

        IPC::MaximumMessageBuffer buffer;

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
        receive(&buffer);

        if (buffer.messageNamespace == IPC::MessageNamespace::ServiceRegistry) {

            if (buffer.messageId == static_cast<uint32_t>(Kernel::MessageId::RegisterServiceDenied)) {
                return 0;
            }
            else if (buffer.messageId == static_cast<uint32_t>(Kernel::MessageId::VGAServiceMeta)) {

                auto msg = IPC::extractMessage<VGAServiceMeta>(buffer);

                NotifyServiceReady ready;
                send(IPC::RecipientType::ServiceRegistryMailbox, &ready);

                return msg.vgaAddress;
            }
        }

        return 0;
    }

    void launchShell() {
        //run("/applications/dsky/dsky.bin");
        uint32_t descriptor {0};
        auto result = openSynchronous("/bin/dsky.bin");

        auto entryPointResult = readSynchronous(result.fileDescriptor, 4);
        uint32_t entryPoint {0};
        memcpy(&entryPoint, entryPointResult.buffer, 4);

        if (entryPoint > 0) {
            auto pid = run(entryPoint);
        }
    }

    struct Window {
        WindowBuffer* buffer;
        uint32_t taskId;
    };

    int main() {
        auto vgaAddress = registerService();

        if (vgaAddress == 0) {
            return 1;
        }

        auto linearFrameBuffer = reinterpret_cast<uint32_t volatile*>(vgaAddress);
        std::vector<Window> windows;
        std::list<Window> windowsWaitingToShare;

        launchShell();

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            switch (buffer.messageNamespace) {
                case IPC::MessageNamespace::WindowManager: {

                    switch (static_cast<MessageId>(buffer.messageId)) {
                        case MessageId::CreateWindow: {
                            auto message = IPC::extractMessage<CreateWindow>(buffer);
                            auto windowBuffer = new WindowBuffer;
                            memset(windowBuffer->buffer, 0, 800 * 600 * 4);

                            Window window {windowBuffer, buffer.senderTaskId};
                            windowsWaitingToShare.push_back(window);

                            ShareMemory share;
                            share.ownerAddress = reinterpret_cast<uintptr_t>(windowBuffer);
                            share.sharedAddress = message.bufferAddress;
                            share.sharedTaskId = message.senderTaskId;
                            share.size = 800 * 600 * 4;

                            send(IPC::RecipientType::ServiceRegistryMailbox, &share);

                            break;
                        }
                        case MessageId::Update: {

                            auto message = IPC::extractMessage<Update>(buffer);
                            uint32_t* windowBuffer = nullptr;

                            for(auto& it : windows) {
                                if (it.taskId == message.senderTaskId) {
                                    windowBuffer = it.buffer->buffer;
                                    break;
                                }
                            }

                            if (windowBuffer == nullptr) {
                                continue;
                            }

                            for (int y = message.y; y < message.height; y++) {
                                auto offset = y * 800;

                                for (int x = message.x; x < message.width; x++) {
                                    linearFrameBuffer[x + offset] = windowBuffer[x + offset];
                                }
                            }

                            break;
                        }
                    }

                    break;
                }
                case IPC::MessageNamespace::ServiceRegistry: {
                    
                    switch (static_cast<Kernel::MessageId>(buffer.messageId)) {
                        case Kernel::MessageId::ShareMemoryResult: {

                            auto message = IPC::extractMessage<ShareMemoryResult>(buffer);

                            auto it = windowsWaitingToShare.begin();
                            CreateWindowSucceeded success;
                            success.recipientId = 0;

                            while (it != windowsWaitingToShare.end()) {
                                if (it->taskId == message.sharedTaskId) {
                                    success.recipientId = it->taskId;
                                    windows.push_back(*it);
                                    windowsWaitingToShare.erase(it);
                                    break;
                                }

                                it.operator++();
                            }

                            if (success.recipientId != 0) {
                                send(IPC::RecipientType::TaskId, &success);
                            }

                            break;
                        }
                    }

                    break;
                }
            }
        }
    }
}