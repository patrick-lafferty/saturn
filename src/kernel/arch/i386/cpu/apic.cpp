#include "apic.h"
#include <stdio.h>
#include <string.h>

namespace APIC {

    void writeRegister(uintptr_t address, uint8_t registerIndex, uint32_t value) {
        uint32_t volatile* selector = reinterpret_cast<uint32_t volatile*>(address);
        uint32_t volatile* val = reinterpret_cast<uint32_t volatile*>(address + 0x10);
        //ptr[0] = registerIndex & 0xFF;
        *selector = registerIndex;
        *val = value;
        //ptr[4] = value;
    }

    void writeLocalAPICRegister(uint32_t offset, uint32_t value) {
        uint32_t volatile* ptr = reinterpret_cast<uint32_t volatile*>(0xFEE00000 + offset);
        *ptr = value;
    }

    void initialize() {
    }

    void loadAPICStructures(uintptr_t address, uint32_t byteLength) {

        /*
        TODO: which of these are necessary?
        */
        writeLocalAPICRegister(0x80, 0x0);
        writeLocalAPICRegister(0xe0, 0xffffffff);
        writeLocalAPICRegister(0xd0, 0x01000000);
        writeLocalAPICRegister(0x320, 0x10000);
        writeLocalAPICRegister(0x340, 0x10000);
        writeLocalAPICRegister(0x350, 0x8700);
        writeLocalAPICRegister(0x360, 0x0400);
        writeLocalAPICRegister(0x370, 0x10000);
        writeLocalAPICRegister(0xF0, 0x113);

        uint8_t* ptr = static_cast<uint8_t*>(reinterpret_cast<void*>(address));
        uint32_t localAPICAddress;
        memcpy(&localAPICAddress, ptr, 4);
        ptr += 4;

        printf("[APIC] Local APIC Address: %x\n", localAPICAddress);

        uint32_t apicFlags;
        memcpy(&apicFlags, ptr, 4);
        ptr += 4;

        //printf("[APIC] Flags: %x\n", apicFlags);

        byteLength -= 8;

        if (apicFlags & static_cast<uint32_t>(DescriptionFlags::PCAT_COMPAT)) {
            printf("[APIC] Dual 8259 detected, will be disabled\n");
        }

        //printf("[APIC] Bytes remaining: %d, sizes: local %d io %d\n", byteLength, sizeof(LocalAPICHeader), sizeof(IOAPICHeader));

        while(byteLength > 0) {
            uint8_t type = *ptr;

            switch(static_cast<APICType>(type)) {
                case APICType::Local: {
                    auto localAPIC = reinterpret_cast<LocalAPICHeader*>(ptr);
                    printf("[APIC] Loading Local APIC structure\n");
                    printf("[APIC] Local APIC Id: %d, address: %d\n", localAPIC->apicId);
                    byteLength -= sizeof(LocalAPICHeader);
                    ptr += sizeof(LocalAPICHeader);
                    break;
                }

                case APICType::IO: {
                    auto ioAPIC = reinterpret_cast<IOAPICHeader*>(ptr);
                    printf("[APIC] Loading IO APIC structure\n");
                    printf("[APIC] IOAPIC id: %d, address: %x, sysVecBase: %d\n", 
                        ioAPIC->apicId, ioAPIC->address, ioAPIC->systemVectorBase);
                    byteLength -= sizeof(IOAPICHeader);
                    ptr += sizeof(IOAPICHeader);
                    
                    /*for(int i = 0; i < 10; i++) {
                        writeRegister(ioAPIC->address, 0x10 + i * 2, 20);
                    }*/

                    writeRegister(ioAPIC->address, 0x10 + 1 * 2, 48);

                    break;
                }

                case APICType::InterruptSourceOverride: {
                    auto interruptOverride = reinterpret_cast<InterruptSourceOverride*>(ptr);
                    printf("[APIC] Loading InterruptSourceOverride structure\n");
                    printf("[APIC] NMI bus: %d, source: %d, globSysIntV: %d, flags: %d\n",
                        interruptOverride->bus, interruptOverride->source,
                        interruptOverride->globalSystemInterruptVector,
                        interruptOverride->flags);
                    byteLength -= sizeof(InterruptSourceOverride);
                    ptr += sizeof(InterruptSourceOverride);
                    break;
                }

                case APICType::NMI: {
                    printf("[APIC] Loading NMI structure\n");
                    byteLength -= sizeof(LocalAPICNMI);
                    ptr += sizeof(LocalAPICNMI);
                    break;
                }

                default: {
                    uint8_t length = *(ptr + 1);
                    printf("[APIC] Skipping reserved APIC structure type: %d length: %d\n", type, length);
                    byteLength -= length;
                    ptr += length;
                }
            }

        }

    }
}