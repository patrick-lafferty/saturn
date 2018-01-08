#include "fakeFileSystem.h"
#include <system_calls.h>
#include <services.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <string.h>
#include <services/terminal/vga.h>
#include <services/terminal/terminal.h>
#include <services/ps2/ps2.h>
#include <services/keyboard/keyboard.h>
#include <userland/shell/shell.h>
#include <services/drivers/bochsGraphicsAdaptor/driver.h>
#include <services/massStorageFileSystem/system.h>
#include <applications/dsky/dsky.h>
#include <services/windows/manager.h>

using namespace VirtualFileSystem;

namespace FakeFileSystem {

    void registerService() {
        MountRequest request;
        const char* path = "/bin";
        auto pathLength = strlen(path) + 1;
        memcpy(request.path, path, pathLength);
        request.serviceType = Kernel::ServiceType::VFS;
        request.cacheable = false;
        request.writeable = false;

        send(IPC::RecipientType::ServiceName, &request);
    }

    struct FileDescriptor {
        uintptr_t entryPoint;
    };

    void messageLoop() {
        uint32_t nextFileDescriptor {0};
        FileDescriptor descriptors[10];

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            switch (buffer.messageNamespace) {
                case IPC::MessageNamespace::VFS: {
                    
                    switch (static_cast<MessageId>(buffer.messageId)) {
                        case MessageId::OpenRequest: {
                            /*
                            All open requests are guaranteed to succeed so long as
                            the path matches one of the below
                            */
                            auto request = IPC::extractMessage<OpenRequest>(buffer);
                            OpenResult result;
                            result.success = true;
                            result.serviceType = Kernel::ServiceType::VFS;
                            result.requestId = request.requestId;

                            auto descriptor = nextFileDescriptor++;
                            result.fileDescriptor = descriptor;

                            uintptr_t entryPoint {0};

                            if (strcmp(request.path, "/vga.service") == 0) {
                                entryPoint = reinterpret_cast<uintptr_t>(VGA::service);
                            }
                            else if (strcmp(request.path, "/terminal.service") == 0) {
                                entryPoint = reinterpret_cast<uintptr_t>(Terminal::service);
                            }
                            else if (strcmp(request.path, "/ps2.service") == 0) {
                                entryPoint = reinterpret_cast<uintptr_t>(PS2::service);
                            }
                            else if (strcmp(request.path, "/keyboard.service") == 0) {
                                entryPoint = reinterpret_cast<uintptr_t>(Keyboard::service);
                            }
                            else if (strcmp(request.path, "/shell") == 0) {
                                entryPoint = reinterpret_cast<uintptr_t>(Shell::main);
                            }
                            else if (strcmp(request.path, "/bochsGraphicsAdaptor.service") == 0) {
                                entryPoint = reinterpret_cast<uintptr_t>(BGA::service);
                            }
                            else if (strcmp(request.path, "/massStorage.service") == 0) {
                                entryPoint = reinterpret_cast<uintptr_t>(MassStorageFileSystem::service);
                            }
                            else if (strcmp(request.path, "/dsky.bin") == 0) {
                                entryPoint = reinterpret_cast<uintptr_t>(dsky_main);
                            }
                            else if (strcmp(request.path, "/windows.service") == 0) {
                                entryPoint = reinterpret_cast<uintptr_t>(Window::main);
                            }
                            else {
                                result.success = false;
                            }

                            descriptors[descriptor].entryPoint = entryPoint;

                            send(IPC::RecipientType::ServiceName, &result);
                            break;
                        }
                        case MessageId::ReadRequest: {
                            auto request = IPC::extractMessage<ReadRequest>(buffer);

                            ReadResult result;
                            result.success = true;
                            result.requestId = request.requestId;
                            memcpy(result.buffer, &descriptors[request.fileDescriptor].entryPoint, sizeof(uintptr_t));
                            result.serviceType = Kernel::ServiceType::VFS;
                            send(IPC::RecipientType::ServiceName, &result);
                            break;
                        }
                        default:
                            break;
                    }

                    break;
                }
                default:
                    break;
            }

        }
    }

    void service() {

        waitForServiceRegistered(Kernel::ServiceType::VFS);
        registerService();
        messageLoop();
    }
}