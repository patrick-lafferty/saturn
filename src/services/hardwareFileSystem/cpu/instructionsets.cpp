#include "instructionsets.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <algorithm>

using namespace VFS;
using namespace Vostok;

namespace HardwareFileSystem {

    CPUInstructionSetsObject::CPUInstructionSetsObject() {
        features = 0;
    }

    void CPUInstructionSetsObject::readSelf(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType("sse3", ArgTypes::Cstring);
        args.writeValueWithType("ssse3", ArgTypes::Cstring);
        args.writeValueWithType("sse4_1", ArgTypes::Cstring);
        args.writeValueWithType("sse4_2", ArgTypes::Cstring);
        args.writeValueWithType("avx", ArgTypes::Cstring);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object function support
    */
    int CPUInstructionSetsObject::getFunction(std::string_view name) {
        return -1;
    }

    void CPUInstructionSetsObject::readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) {
        describeFunction(requesterTaskId, requestId, functionId);
    }

    void CPUInstructionSetsObject::writeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId, ArgBuffer& args) {
    }

    void CPUInstructionSetsObject::describeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) {
    }

    /*
    Vostok Object property support
    */
    int CPUInstructionSetsObject::getProperty(std::string_view name) {
        if (name.compare("sse3") == 0) {
            return static_cast<int>(PropertyId::SSE3);
        }
        else if (name.compare("ssse3") == 0) {
            return static_cast<int>(PropertyId::SSSE3);
        }
        else if (name.compare("sse4_1") == 0) {
            return static_cast<int>(PropertyId::SSE4_1);
        }
        else if (name.compare("sse4_2") == 0) {
            return static_cast<int>(PropertyId::SSE4_2);
        }
        else if (name.compare("avx") == 0) {
            return static_cast<int>(PropertyId::AVX);
        }

        return -1;
    }

    void CPUInstructionSetsObject::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType((features & (1 << propertyId)) > 0, ArgTypes::Uint32);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void CPUInstructionSetsObject::writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, ArgBuffer& args) {
        auto type = args.readType();

        if (type != ArgTypes::Property) {
            replyWriteSucceeded(requesterTaskId, requestId, false);
            return;
        }

        auto x = args.read<uint32_t>(ArgTypes::Uint32);

        if (!args.hasErrors()) {
            if (x > 0) {
                features |= (1 << propertyId);
            }
            replyWriteSucceeded(requesterTaskId, requestId, true);
            return;
        }

        replyWriteSucceeded(requesterTaskId, requestId, false);
    }

    Object* CPUInstructionSetsObject::getNestedObject(std::string_view name) {
        return nullptr;
    }

    /*
    CPU Features Object specific implementation
    */

    const char* CPUInstructionSetsObject::getName() {
        return "instruction_sets";
    }

    void detectInstructionSets(uint32_t ecx, uint32_t edx) {
        auto sse3 = (ecx & (1u << 0));
        auto ssse3 = (ecx & (1u << 9));
        auto sse4_1 = (ecx & (1u << 19));
        auto sse4_2 = (ecx & (1u << 20));
        auto avx = (ecx & (1u << 28));

        writeTransaction("/system/hardware/cpu/features/instruction_sets/sse3", sse3, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instruction_sets/ssse3", ssse3, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instruction_sets/sse4_1", sse4_1, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instruction_sets/sse4_2", sse4_2, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instruction_sets/avx", avx, ArgTypes::Uint32);
    }
}