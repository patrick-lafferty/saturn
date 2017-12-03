#include "instructions.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <algorithm>

using namespace VFS;
using namespace Vostok;

namespace HardwareFileSystem {

    CPUInstructionsObject::CPUInstructionsObject() {
        instructions = 0;
    }

    void CPUInstructionsObject::readSelf(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType("pclmulqdq", ArgTypes::Cstring);
        args.writeValueWithType("cmpxchg16b", ArgTypes::Cstring);
        args.writeValueWithType("movbe", ArgTypes::Cstring);
        args.writeValueWithType("popcnt", ArgTypes::Cstring);
        args.writeValueWithType("aesni", ArgTypes::Cstring);
        args.writeValueWithType("xsave", ArgTypes::Cstring);
        args.writeValueWithType("osxsave", ArgTypes::Cstring);
        args.writeValueWithType("f16c", ArgTypes::Cstring);
        args.writeValueWithType("rdrand", ArgTypes::Cstring);
        args.writeValueWithType("tsc", ArgTypes::Cstring);
        args.writeValueWithType("msr", ArgTypes::Cstring);
        args.writeValueWithType("cmpxchg8b", ArgTypes::Cstring);
        args.writeValueWithType("sep", ArgTypes::Cstring);
        args.writeValueWithType("cmov", ArgTypes::Cstring);
        args.writeValueWithType("clfsh", ArgTypes::Cstring);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object property support
    */
    int CPUInstructionsObject::getProperty(std::string_view name) {
        if (name.compare("pclmulqdq") == 0) {
            return static_cast<int>(PropertyId::PCLMULQDQ);
        }
        else if (name.compare("cmpxchg16b") == 0) {
            return static_cast<int>(PropertyId::CMPXCHG16B);
        }
        else if (name.compare("movbe") == 0) {
            return static_cast<int>(PropertyId::MOVBE);
        }
        else if (name.compare("popcnt") == 0) {
            return static_cast<int>(PropertyId::POPCNT);
        }
        else if (name.compare("aesni") == 0) {
            return static_cast<int>(PropertyId::AESNI);
        }
        else if (name.compare("xsave") == 0) {
            return static_cast<int>(PropertyId::XSAVE);
        }
        else if (name.compare("osxsave") == 0) {
            return static_cast<int>(PropertyId::OSXSAVE);
        }
        else if (name.compare("f16c") == 0) {
            return static_cast<int>(PropertyId::F16C);
        }
        else if (name.compare("rdrand") == 0) {
            return static_cast<int>(PropertyId::RDRAND);
        }
        else if (name.compare("tsc") == 0) {
            return static_cast<int>(PropertyId::TSC);
        }
        else if (name.compare("msr") == 0) {
            return static_cast<int>(PropertyId::MSR);
        }
        else if (name.compare("cmpxchg8b") == 0) {
            return static_cast<int>(PropertyId::CMPXCHG8B);
        }
        else if (name.compare("sep") == 0) {
            return static_cast<int>(PropertyId::SEP);
        }
        else if (name.compare("cmov") == 0) {
            return static_cast<int>(PropertyId::CMOV);
        }
        else if (name.compare("CLFSH") == 0) {
            return static_cast<int>(PropertyId::CLFSH);
        }

        return -1;
    }

    void CPUInstructionsObject::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType((instructions & (1 << propertyId)) > 0, ArgTypes::Uint32);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void CPUInstructionsObject::writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, ArgBuffer& args) {
        auto type = args.readType();

        if (type != ArgTypes::Property) {
            replyWriteSucceeded(requesterTaskId, requestId, false);
            return;
        }

        auto x = args.read<uint32_t>(ArgTypes::Uint32);

        if (!args.hasErrors()) {
            if (x > 0) {
                instructions |= (1 << propertyId);
            }
            replyWriteSucceeded(requesterTaskId, requestId, true);
            return;
        }

        replyWriteSucceeded(requesterTaskId, requestId, false);
    }

    /*
    CPU Features Object specific implementation
    */

    const char* CPUInstructionsObject::getName() {
        return "instructions";
    }

    void detectInstructions(uint32_t ecx, uint32_t edx) {
        auto pclmulqdq = (ecx & (1u << 1));
        auto cmpxchg16b = (ecx & (1u << 13));
        auto movbe = (ecx & (1u << 22));
        auto popcnt = (ecx & (1u << 23));
        auto aesni = (ecx & (1u << 25));
        auto xsave = (ecx & (1u << 26));
        auto osxsave = (ecx & (1u << 27));
        auto f16c = (ecx & (1u << 29));
        auto rdrand = (ecx & (1u << 30));
        auto tsc = (edx & (1u << 4));
        auto msr = (edx & (1u << 5));
        auto cmpxchg8b = (edx & (1u << 8));
        auto sep = (edx & (1u << 11));
        auto cmov = (edx & (1u << 15));
        auto clfsh = (edx & (1u << 19));

        writeTransaction("/system/hardware/cpu/features/instructions/pclmulqdq", pclmulqdq, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instructions/cmpxchg16b", cmpxchg16b, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instructions/movbe", movbe, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instructions/popcnt", popcnt, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instructions/aesni", aesni, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instructions/xsave", xsave, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instructions/osxsave", osxsave, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instructions/f16c", f16c, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instructions/rdrand", rdrand, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instructions/tsc", tsc, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instructions/msr", msr, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instructions/cmpxchg8b", cmpxchg8b, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instructions/sep", sep, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instructions/cmov", cmov, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instructions/clfsh", clfsh, ArgTypes::Uint32);
    }
}