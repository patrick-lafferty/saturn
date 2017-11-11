#include "tss.h"

namespace CPU {
    void setupTSS(uint32_t address) {
        TSS* tss = static_cast<TSS*>(reinterpret_cast<void*>(address));
        fillTSS(tss);
        loadTSS();
    }
}