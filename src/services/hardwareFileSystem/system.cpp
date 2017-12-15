#include "system.h"
#include <services.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <vector>
#include <parsing>
#include "cpu/cpu.h"
#include "pci/pci.h"

using namespace Kernel;
using namespace VirtualFileSystem;
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
        HardwareObject* instance;
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
        OpenResult result;
        result.requestId = request.requestId;
        result.serviceType = ServiceType::VFS;

        auto addDescriptor = [&](auto instance, auto id, auto type) {
            failed = false;
            result.success = true;
            openDescriptors.push_back({instance, {static_cast<uint32_t>(id)}, type});
            result.fileDescriptor = openDescriptors.size() - 1;
        };

        auto tryOpenObject = [&](auto object, auto name) {
            auto functionId = object->getFunction(name);

            if (functionId >= 0) {
                addDescriptor(object, functionId, DescriptorType::Function);
                return true;
            }
            else {
                auto propertyId = object->getProperty(name);

                if (propertyId >= 0) {
                    addDescriptor(object, propertyId, DescriptorType::Property);
                    return true;
                }
            }

            return false;
        };

        auto words = split({request.path, strlen(request.path)}, '/');

        if (words.size() > 0) {
            Object* object;
            auto found = findObject(objects, words[0], &object);

            if (found) {
                
                uint32_t lastObjectWord {0};
                for (auto i = 1u; i < words.size(); i++) {
                    auto nestedObject = object->getNestedObject(words[i]);

                    if (nestedObject != nullptr) {
                        object = nestedObject;
                        lastObjectWord = i;
                    }
                }

                if (!tryOpenObject(object, words[words.size() - 1])) {
                    if (lastObjectWord == (words.size() - 1)) {
                        addDescriptor(object, 0, DescriptorType::Object);
                    }
                }
            }
        }
        else {
            //its the hardware toplevel
            addDescriptor(nullptr, 0, DescriptorType::Object);
        }

        if (failed) {
            result.success = false;
        }

        send(IPC::RecipientType::ServiceName, &result);
    }

    HardwareObject* createObject(std::string_view name) {
        if (name.compare("cpu") == 0) {
            return new CPUObject();
        }
        else if (name.compare("pci") == 0) {
            return new PCI::Object();
        }

        return nullptr;
    }

    void handleCreateRequest(CreateRequest& request, std::vector<NamedObject>& objects) {
        bool failed {true};
        CreateResult result;
        result.requestId = request.requestId;
        result.serviceType = ServiceType::VFS;

        auto words = split({request.path, strlen(request.path)}, '/');

        /*
        when the VFS forwards this request, it strips off /system/hardware, leaving / and the path
        for example /cpu, /pci
        */
        if (words.size() == 1) {
            bool alreadyExists {false};

            for (auto& object : objects) {
                if (words[0].compare(object.name) == 0) {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists) {
                auto object = createObject(words[0]);
                if (object != nullptr) {
                    objects.push_back({});
                    auto& namedObject = objects[objects.size() - 1];
                    words[0].copy(namedObject.name, words[0].length());
                    namedObject.instance = object;

                    failed = false;
                    result.success = true;
                }
            }
        }
        else {
            //TODO: its a subobject
        }
        

        if (failed) {
            result.success = false;
        }

        send(IPC::RecipientType::ServiceName, &result);
    }

    void handleReadRequest(ReadRequest& request, std::vector<NamedObject>& objects, std::vector<FileDescriptor>& openDescriptors) {
        auto& descriptor = openDescriptors[request.fileDescriptor];

        if (descriptor.type == DescriptorType::Object) {

            if (descriptor.instance == nullptr) {

                ReadResult result;
                result.requestId = request.requestId;
                result.success = true;
                ArgBuffer args{result.buffer, sizeof(result.buffer)};
                args.writeType(ArgTypes::Property);
                //its the main /system/hardware thing, return a list of all objects

                for (auto& object : objects) {
                    args.writeValueWithType(object.name, ArgTypes::Cstring);
                }

                args.writeType(ArgTypes::EndArg);
                result.recipientId = request.senderTaskId;

                send(IPC::RecipientType::TaskId, &result);
            }
            else {
                //its a hardware object, return a summary of it
                descriptor.read(request.senderTaskId, request.requestId);
            }

        }
        else {
            descriptor.read(request.senderTaskId, request.requestId);
        }
    }

    void handleWriteRequest(WriteRequest& request, std::vector<FileDescriptor>& openDescriptors) {
        ArgBuffer args{request.buffer, sizeof(request.buffer)};
        openDescriptors[request.fileDescriptor].write(request.senderTaskId, request.requestId, args);
    }

    void handleCloseRequest(CloseRequest& request, std::vector<FileDescriptor>& openDescriptors) {
        //TODO: this is broken
        //openDescriptors.erase(&openDescriptors[request.fileDescriptor]);
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
            else if (buffer.messageId == ReadRequest::MessageId) {
                auto request = IPC::extractMessage<ReadRequest>(buffer);
                handleReadRequest(request, objects, openDescriptors);
            }
            else if (buffer.messageId == WriteRequest::MessageId) {
                auto request = IPC::extractMessage<WriteRequest>(buffer);
                handleWriteRequest(request, openDescriptors);
            }
            else if (buffer.messageId == CloseRequest::MessageId) {
                auto request = IPC::extractMessage<CloseRequest>(buffer);
                handleCloseRequest(request, openDescriptors);
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