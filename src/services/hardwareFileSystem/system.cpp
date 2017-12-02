#include "system.h"
#include <services.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <vector>
#include <parsing>
#include "cpu.h"

using namespace Kernel;
using namespace VFS;
using namespace Vostok;

namespace HardwareFileSystem {

    void registerService() {
        MountRequest request;
        const char* path = "/system/hardware";
        memcpy(request.path, path, strlen(path) + 1);
        request.serviceType = ServiceType::VFS;

        send(IPC::RecipientType::ServiceName, &request);
    }

    struct NamedObject {
        char name[20];
        Object* instance;
    };

    bool findObject(std::vector<NamedObject>& objects, std::string_view name, Object** found) {
        for (auto& object : objects) {
            if (name.compare(object.name) == 0) {
                *found = object.instance;
                return true;
            }
        }

        return false;
    }

    void handleOpenRequest(OpenRequest& request, std::vector<NamedObject> objects,  std::vector<FileDescriptor>& openDescriptors) {
        bool failed {true};
        OpenResult result{};
        result.serviceType = ServiceType::VFS;

        auto addDescriptor = [&](auto instance, auto id, auto type) {
            failed = false;
            result.success = true;
            openDescriptors.push_back({instance, {static_cast<uint32_t>(id)}, type});
            result.fileDescriptor = openDescriptors.size() - 1;
        };

        auto words = split({request.path, strlen(request.path)}, '/');
        if (words.size() >= 2
            && words[0].compare("system") == 0
            && words[1].compare("hardware") == 0) {

            if (words.size() > 2) {
                Object* object;
                auto found = findObject(objects, words[2], &object);

                if (found) {
                    if (words.size() > 3) {
                        auto functionId = object->getFunction(words[3]);

                        if (functionId >= 0) {
                            addDescriptor(object, functionId, DescriptorType::Function);
                        }
                        else {
                            auto propertyId = object->getProperty(words[3]);

                            if (propertyId >= 0) {
                                addDescriptor(object, propertyId, DescriptorType::Property);
                            }
                        }
                    }
                    else {
                        //its the object itself
                        addDescriptor(object, 0, DescriptorType::Object);
                    }
                }
            }
            else {
                //its the hardware toplevel
                addDescriptor(nullptr, 0, DescriptorType::Object);
            }
        }

        if (failed) {
            result.success = false;
        }

        send(IPC::RecipientType::ServiceName, &result);
    }

    Object* createObject(std::string_view name) {
        if (name.compare("cpu") == 0) {

        }
        else {
            return nullptr;
        }
    }

    void handleCreateRequest(CreateRequest& request, std::vector<NamedObject>& objects) {
        bool failed {true};
        CreateResult result;
        result.serviceType = ServiceType::VFS;

        auto words = split({request.path, strlen(request.path)}, '/');
        if (words.size() >= 3
            && words[0].compare("system") == 0
            && words[1].compare("hardware") == 0) {

            if (words.size() == 3) {
                bool alreadyExists {false};

                for (auto& object : objects) {
                    if (words[2].compare(object.name) == 0) {
                        alreadyExists = true;
                        break;
                    }
                }

                if (!alreadyExists) {
                    auto object = createObject(words[2]);
                    if (object != nullptr) {
                        objects.push_back({});
                        auto& namedObject = objects[objects.size() - 1];
                        words[2].copy(namedObject.name, words[2].length());
                        namedObject.instance = object;

                        failed = false;
                        result.success = true;
                    }
                }
            }
            else {
                //TODO: its a subobject
            }
        } 

        if (failed) {
            result.success = false;
        }

        send(IPC::RecipientType::ServiceName, &result);
    }

    void messageLoop() {

        std::vector<NamedObject> objects;
        std::vector<FileDescriptor> openDescriptors;

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == OpenRequest::MessageId) {
                auto request = IPC::extractMessage<OpenRequest>(buffer);
                handleOpenRequest(request, objects, openDescriptors);
            }
            else if (buffer.messageId == CreateRequest::MessageId) {
                auto request = IPC::extractMessage<CreateRequest>(buffer);
                handleCreateRequest(request, objects);
            }
        }
    }

    void service() {
        waitForServiceRegistered(ServiceType::VFS);
        registerService();
        messageLoop();
    }

    void detectHardware() {
        waitForServiceRegistered(ServiceType::VFS);

        detectCPU();
    }
}