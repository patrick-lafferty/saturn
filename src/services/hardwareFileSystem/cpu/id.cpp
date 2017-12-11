#include "id.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <algorithm>

using namespace VirtualFileSystem;
using namespace Vostok;

namespace HardwareFileSystem {

    CPUIdObject::CPUIdObject() {
        
    }

    void CPUIdObject::readSelf(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType("model", ArgTypes::Cstring);
        args.writeValueWithType("familyId", ArgTypes::Cstring);
        args.writeValueWithType("brand", ArgTypes::Cstring);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object property support
    */
    int CPUIdObject::getProperty(std::string_view name) {
        if (name.compare("model") == 0) {
            return static_cast<int>(PropertyId::Model);
        }
        else if (name.compare("familyId") == 0) {
            return static_cast<int>(PropertyId::FamilyId);
        }
        else if (name.compare("brand") == 0) {
            return static_cast<int>(PropertyId::Brand);
        }

        return -1;
    }

    void CPUIdObject::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        switch(static_cast<PropertyId>(propertyId)) {
            case PropertyId::Model: {
                args.writeValueWithType(model, ArgTypes::Uint32);
                break;
            }
            case PropertyId::FamilyId: {
                args.writeValueWithType(familyId, ArgTypes::Uint32);
                break;
            }
            case PropertyId::Brand: {
                args.writeValueWithType(brand, ArgTypes::Cstring);
                break;
            }
        }

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void CPUIdObject::writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, ArgBuffer& args) {
        auto type = args.readType();

        if (type != ArgTypes::Property) {
            replyWriteSucceeded(requesterTaskId, requestId, false);
            return;
        }

        switch(static_cast<PropertyId>(propertyId)) {
            case PropertyId::Model: {
                auto x = args.read<uint32_t>(ArgTypes::Uint32);

                if (!args.hasErrors()) {
                    model = x;
                    replyWriteSucceeded(requesterTaskId, requestId, true);
                    return;
                }
            }
            case PropertyId::FamilyId: {
                auto x = args.read<uint32_t>(ArgTypes::Uint32);

                if (!args.hasErrors()) {
                    familyId = x;
                    replyWriteSucceeded(requesterTaskId, requestId, true);
                    return;
                }
            }
            case PropertyId::Brand: {
                auto x = args.read<char*>(ArgTypes::Cstring);

                if (!args.hasErrors()) {
                    memcpy(brand, x, std::min(static_cast<size_t>(sizeof(brand)), strlen(x)));
                    replyWriteSucceeded(requesterTaskId, requestId, true);
                    return;
                }
            }
        }

        replyWriteSucceeded(requesterTaskId, requestId, false);
    }

    /*
    CPU Id Object specific implementation
    */

    const char* CPUIdObject::getName() {
        return "id";
    }

    void extractCharacters(char* buffer, int index, uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
        uint32_t masks[] = {0xff, 0xff00, 0xff0000, 0xff000000};
        uint32_t shifts[] = {0, 8, 16, 24};
        uint32_t values[] = {eax, ebx, ecx, edx};

        for(int i = 0; i < 16; i++) {
            buffer[index + i] = (values[i / 4] & masks[i % 4]) >> (shifts[i % 4]);
        }
    }

    void getBrandString(char* buffer) {
        uint32_t eax, ebx, ecx, edx;

        for (auto i = 0; i < 3; i++) {
            asm("cpuid"
                : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
                : "a" (0x80000002 + i)
            );

            extractCharacters(buffer, i * 16, eax, ebx, ecx, edx);
        }
    }

    void detectId(uint32_t eax, uint32_t ebx) {
        auto model = (eax & 0xf0) >> 4;
        auto familyId = (eax & 0xf00) >> 8;
        auto brandIndex = (ebx & 0xff);

        writeTransaction("/system/hardware/cpu/id/model", model, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/id/familyId", familyId, ArgTypes::Uint32);
        
        if (!((familyId > 0x0F && model >= 0x03) || brandIndex)) {
            char brandString[49];
            brandString[48] = '\0';
            getBrandString(brandString);
            writeTransaction("/system/hardware/cpu/id/brand", brandString, ArgTypes::Cstring);
        }
    }
}