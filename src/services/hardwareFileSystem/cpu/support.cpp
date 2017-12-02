#include "support.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <algorithm>

using namespace VFS;
using namespace Vostok;

namespace HardwareFileSystem {

    CPUSupportObject::CPUSupportObject() {
        support = 0;
    }

    void CPUSupportObject::readSelf(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType("dtes64", ArgTypes::Cstring);
        args.writeValueWithType("monitor", ArgTypes::Cstring);
        args.writeValueWithType("ds_cpl", ArgTypes::Cstring);
        args.writeValueWithType("cnxt_id", ArgTypes::Cstring);
        args.writeValueWithType("sdbg", ArgTypes::Cstring);
        args.writeValueWithType("xtpr", ArgTypes::Cstring);
        args.writeValueWithType("pdcm", ArgTypes::Cstring);
        args.writeValueWithType("pcid", ArgTypes::Cstring);
        args.writeValueWithType("dca", ArgTypes::Cstring);
        args.writeValueWithType("x2apic", ArgTypes::Cstring);
        args.writeValueWithType("tsc_deadline", ArgTypes::Cstring);
        args.writeValueWithType("mtrr", ArgTypes::Cstring);
        args.writeValueWithType("pge", ArgTypes::Cstring);
        args.writeValueWithType("pat", ArgTypes::Cstring);
        args.writeValueWithType("psn", ArgTypes::Cstring);
        args.writeValueWithType("apic", ArgTypes::Cstring);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object function support
    */
    int CPUSupportObject::getFunction(std::string_view name) {
        return -1;
    }

    void CPUSupportObject::readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) {
        describeFunction(requesterTaskId, requestId, functionId);
    }

    void CPUSupportObject::writeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId, ArgBuffer& args) {
    }

    void CPUSupportObject::describeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) {
    }

    /*
    Vostok Object property support
    */
    int CPUSupportObject::getProperty(std::string_view name) {
        if (name.compare("dtes64") == 0) {
            return static_cast<int>(PropertyId::DTES64);
        }
        else if (name.compare("monitor") == 0) {
            return static_cast<int>(PropertyId::MONITOR);
        }
        else if (name.compare("ds_cpl") == 0) {
            return static_cast<int>(PropertyId::DS_CPL);
        }
        else if (name.compare("cnxt_id") == 0) {
            return static_cast<int>(PropertyId::CNXT_ID);
        }
        else if (name.compare("sdbg") == 0) {
            return static_cast<int>(PropertyId::SDBG);
        }
        else if (name.compare("xtpr") == 0) {
            return static_cast<int>(PropertyId::XTPR);
        }
        else if (name.compare("pdcm") == 0) {
            return static_cast<int>(PropertyId::PDCM);
        }
        else if (name.compare("pcid") == 0) {
            return static_cast<int>(PropertyId::PCID);
        }
        else if (name.compare("dca") == 0) {
            return static_cast<int>(PropertyId::DCA);
        }
        else if (name.compare("x2apic") == 0) {
            return static_cast<int>(PropertyId::X2APIC);
        }
        else if (name.compare("tsc_deadline") == 0) {
            return static_cast<int>(PropertyId::TSC_DEADLINE);
        }
        else if (name.compare("mtrr") == 0) {
            return static_cast<int>(PropertyId::MTRR);
        }
        else if (name.compare("pge") == 0) {
            return static_cast<int>(PropertyId::PGE);
        }
        else if (name.compare("pat") == 0) {
            return static_cast<int>(PropertyId::PAT);
        }
        else if (name.compare("psn") == 0) {
            return static_cast<int>(PropertyId::PSN);
        }
        else if (name.compare("apic") == 0) {
            return static_cast<int>(PropertyId::APIC);
        }

        return -1;
    }

    void CPUSupportObject::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType((support & (1 << propertyId)) > 0, ArgTypes::Uint32);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void CPUSupportObject::writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, ArgBuffer& args) {
        auto type = args.readType();

        if (type != ArgTypes::Property) {
            replyWriteSucceeded(requesterTaskId, requestId, false);
            return;
        }

        auto x = args.read<uint32_t>(ArgTypes::Uint32);

        if (!args.hasErrors()) {
            if (x > 0) {
                support |= (1 << propertyId);
            }
            replyWriteSucceeded(requesterTaskId, requestId, true);
            return;
        }

        replyWriteSucceeded(requesterTaskId, requestId, false);
    }

    Object* CPUSupportObject::getNestedObject(std::string_view name) {
        return nullptr;
    }

    /*
    CPU Features Object specific implementation
    */

    const char* CPUSupportObject::getName() {
        return "support";
    }

    void detectSupport(uint32_t ecx, uint32_t edx) {
        auto dtes64 = (ecx & (1u << 2));
        auto monitor = (ecx & (1u << 3));
        auto ds_cpl = (ecx & (1u << 4));
        auto cnxt_id = (ecx & (1u << 10));
        auto sdbg = (ecx & (1u << 11));
        auto xtpr = (ecx & (1u << 14));
        auto pdcm = (ecx & (1u << 15));
        auto pcid = (ecx & (1u << 16));
        auto dca = (ecx & (1u << 17));
        auto x2apic = (ecx & (1u << 21));
        auto tsc_deadline = (ecx & (1u << 24));
        auto mtrr = (edx & (1u << 12));
        auto pge = (edx & (1u << 13));
        auto pat = (edx & (1u << 16));
        auto psn = (edx & (1u << 18));
        auto apic = (edx & (1u << 9));

        writeTransaction("/system/hardware/cpu/features/support/dtes64", dtes64, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/support/monitor", monitor, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/support/ds_cpl", ds_cpl, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/support/cnxt_id", cnxt_id, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/support/sdbg", sdbg, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/support/xtpr", xtpr, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/support/pdcm", pdcm, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/support/pcid", pcid, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/support/dca", dca, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/support/x2apic", x2apic, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/support/tsc_deadline", tsc_deadline, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/support/mtrr", mtrr, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/support/pge", pge, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/support/pat", pat, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/support/psn", psn, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/support/apic", apic, ArgTypes::Uint32);
    }
}