#pragma once

/*
The Hardware Filesystem is similar to the Process Filesystem,
in that it creates a bunch of Vostok Objects to represent
different hardware devices. Its mounted at /system/hardware,
and allows user programs to query hardware information,
for example cpu stats at /system/hardware/cpu, or 
pci information at /system/hardware/pci
*/

namespace HardwareFileSystem {
    void service();
}