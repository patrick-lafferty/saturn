#include "loader.h"
#include "pci.h"

namespace Discovery {
    void discoverDevices() {
        discoverPCIDevices();
    }
}