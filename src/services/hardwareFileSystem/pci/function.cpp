#include "function.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <algorithm>

using namespace VFS;
using namespace Vostok;

namespace HardwareFileSystem::PCI {

    FunctionObject::FunctionObject() {
        vendorId = 0xFFFF;
    }

    void FunctionObject::readSelf(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType("classCode", ArgTypes::Cstring);
        args.writeValueWithType("subclassCode", ArgTypes::Cstring);
        args.writeValueWithType("bar0", ArgTypes::Cstring);
        args.writeValueWithType("bar1", ArgTypes::Cstring);
        args.writeValueWithType("bar2", ArgTypes::Cstring);
        args.writeValueWithType("bar3", ArgTypes::Cstring);
        args.writeValueWithType("bar4", ArgTypes::Cstring);
        args.writeValueWithType("bar5", ArgTypes::Cstring);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object function support
    */
    int FunctionObject::getFunction(std::string_view name) {
        if (name.compare("summary") == 0) {
            return static_cast<int>(FunctionId::Summary);
        }

        return -1;
    }

    void FunctionObject::readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) {
        describeFunction(requesterTaskId, requestId, functionId);
    }

    void FunctionObject::writeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId, ArgBuffer& args) {
        auto type = args.readType();

        if (type != ArgTypes::Function) {
            replyWriteSucceeded(requesterTaskId, requestId, false);
            return;
        }

        switch(functionId) {
            case static_cast<uint32_t>(FunctionId::Summary): {

                if (!args.hasErrors()) {
                    summary(requesterTaskId, requestId);
                }
                else {
                    replyWriteSucceeded(requesterTaskId, requestId, false);
                }

                break;
            }
            default: {
                replyWriteSucceeded(requesterTaskId, requestId, false);
            }
        }
    }

    void FunctionObject::describeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) {
        ReadResult result {};
        result.requestId = requestId;
        result.success = true;
        ArgBuffer args{result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Function);
        
        switch(functionId) {
            case static_cast<uint32_t>(FunctionId::Summary): {
                args.writeType(ArgTypes::Void);
                args.writeType(ArgTypes::Cstring);
                args.writeType(ArgTypes::EndArg);
                break;
            }
            default: {
                result.success = false;
            }
        }

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object property support
    */
    int FunctionObject::getProperty(std::string_view name) {
        if (name.compare("vendorId") == 0) {
            return static_cast<int>(PropertyId::VendorId);
        }
        else if (name.compare("classCode") == 0) {
            return static_cast<int>(PropertyId::ClassCode);
        }
        else if (name.compare("subclassCode") == 0) {
            return static_cast<int>(PropertyId::SubclassCode);
        }
        else if (name.compare("bar0") == 0) {
            return static_cast<int>(PropertyId::Bar0);
        }
        else if (name.compare("bar1") == 0) {
            return static_cast<int>(PropertyId::Bar1);
        }
        else if (name.compare("bar2") == 0) {
            return static_cast<int>(PropertyId::Bar2);
        }
        else if (name.compare("bar3") == 0) {
            return static_cast<int>(PropertyId::Bar3);
        }
        else if (name.compare("bar4") == 0) {
            return static_cast<int>(PropertyId::Bar4);
        }
        else if (name.compare("bar5") == 0) {
            return static_cast<int>(PropertyId::Bar5);
        }

        return -1;
    }

    void FunctionObject::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        switch(static_cast<PropertyId>(propertyId)) {
            case PropertyId::VendorId: {
                args.writeValueWithType(vendorId, ArgTypes::Uint32);
                break;
            }
            case PropertyId::ClassCode: {
                args.writeValueWithType(classCode, ArgTypes::Uint32);
                break;
            }
            case PropertyId::SubclassCode: {
                args.writeValueWithType(subclassCode, ArgTypes::Uint32);
                break;
            }
            case PropertyId::Bar0: {
                args.writeValueWithType(bar0, ArgTypes::Uint32);
                break;
            }
            case PropertyId::Bar1: {
                args.writeValueWithType(bar1, ArgTypes::Uint32);
                break;
            }
            case PropertyId::Bar2: {
                args.writeValueWithType(bar2, ArgTypes::Uint32);
                break;
            }
            case PropertyId::Bar3: {
                args.writeValueWithType(bar3, ArgTypes::Uint32);
                break;
            }
            case PropertyId::Bar4: {
                args.writeValueWithType(bar4, ArgTypes::Uint32);
                break;
            }
            case PropertyId::Bar5: {
                args.writeValueWithType(bar5, ArgTypes::Uint32);
                break;
            }
        }

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void FunctionObject::writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, ArgBuffer& args) {
        auto type = args.readType();

        if (type != ArgTypes::Property) {
            replyWriteSucceeded(requesterTaskId, requestId, false);
            return;
        }

        switch(static_cast<PropertyId>(propertyId)) {
            case PropertyId::VendorId: {
                auto x = args.read<uint32_t>(ArgTypes::Uint32);

                if (!args.hasErrors()) {
                    vendorId = x;
                    replyWriteSucceeded(requesterTaskId, requestId, true);
                    return;
                }

                break;
            }
            case PropertyId::ClassCode: {
                auto x = args.read<uint32_t>(ArgTypes::Uint32);

                if (!args.hasErrors()) {
                    classCode = x;
                    replyWriteSucceeded(requesterTaskId, requestId, true);
                    return;
                }

                break;
            }
            case PropertyId::SubclassCode: {
                auto x = args.read<uint32_t>(ArgTypes::Uint32);

                if (!args.hasErrors()) {
                    subclassCode = x;
                    replyWriteSucceeded(requesterTaskId, requestId, true);
                    return;
                }

                break;
            }
            case PropertyId::Bar0: 
            case PropertyId::Bar1: 
            case PropertyId::Bar2: 
            case PropertyId::Bar3: 
            case PropertyId::Bar4: 
            case PropertyId::Bar5:  {
                auto x = args.read<uint32_t>(ArgTypes::Uint32);

                if (!args.hasErrors()) {
                    switch(static_cast<PropertyId>(propertyId)) {
                        case PropertyId::Bar0: {
                            bar0 = x;
                            break;
                        }
                        case PropertyId::Bar1: {
                            bar1 = x;
                            break;
                        }
                        case PropertyId::Bar2: {
                            bar2 = x;
                            break;
                        }
                        case PropertyId::Bar3: {
                            bar3 = x;
                            break;
                        }
                        case PropertyId::Bar4: {
                            bar4 = x;
                            break;
                        }
                        case PropertyId::Bar5: {
                            bar5 = x;
                            break;
                        }
                        default: {
                            //should never get here
                        }
                    }

                    replyWriteSucceeded(requesterTaskId, requestId, true);
                    return;
                }

                break;
            }
            
        }

        replyWriteSucceeded(requesterTaskId, requestId, false);
    }

    ::Object* FunctionObject::getNestedObject(std::string_view) {

        return nullptr;
    }

    /*
    FunctionObject specific implementation
    */

    const char* FunctionObject::getName() {
        return "function";
    }

    bool FunctionObject::exists() const {
        return vendorId != 0xFFFF;
    }

    void FunctionObject::summary(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        char buffer[250];
        memset(buffer, '\0', sizeof(buffer));
        /*auto length = sprintf(buffer, "vendor: %x\n", vendorId);
        length += sprintf(buffer + length, "class: %x\n", classCode);
        length += sprintf(buffer + length, "subclass: %x\n", subclassCode);
        */

        args.writeValueWithType(buffer, ArgTypes::Cstring);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }
}