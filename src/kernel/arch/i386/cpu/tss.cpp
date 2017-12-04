#include "tss.h"

namespace CPU {
    TSS* setupTSS(uint32_t address) {
        TSS* tss = static_cast<TSS*>(reinterpret_cast<void*>(address));
        fillTSS(tss);

        auto offset = sizeof(TSS) - sizeof(TSS::ioPermissionBitmap);
        tss->ioMapBaseAddress = offset << 16;

        loadTSS();
        return tss;
    }
}