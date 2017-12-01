#include "cpuid.h"
#include <stdio.h>

void cpuid_vendor_impl([[maybe_unused]] uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    char vendor[13];

    vendor[0] = ebx & 0xff; 
    vendor[1] = (ebx & 0xff00) >> 8;
    vendor[2] = (ebx & 0xff0000) >> 16;
    vendor[3] = (ebx & 0xff000000) >> 24;
    vendor[4] = edx & 0xff; 
    vendor[5] = (edx & 0xff00) >> 8;
    vendor[6] = (edx & 0xff0000) >> 16;
    vendor[7] = (edx & 0xff000000) >> 24;
    vendor[8] = ecx & 0xff;
    vendor[9] = (ecx & 0xff00) >> 8;
    vendor[10] = (ecx & 0xff0000) >> 16;
    vendor[11] = (ecx & 0xff000000) >> 24; 
    vendor[12] = '\0';

    kprintf("vendor: %s\n", vendor);
}

namespace CPUID {
    /*see https://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-vol-2a-manual.html
    for where these values come from
    */

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
                //: "eax", "ebx", "ecx", "edx"
            );

            extractCharacters(buffer, i * 16, eax, ebx, ecx, edx);
        }
    }

    void printVersionInformation() {
        uint32_t eax, ebx, ecx, edx;

        asm("cpuid"
            : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
            : "a" (1)
            //: "eax", "ebx", "ecx", "edx"
        );

        /*
        EAX:
        bits 0-3: stepping id
        bits 4-7: model
        bits 8-11: family id
        bits 12-13: processor type
        bits 14-15: reserved
        bits 16-19: extended model id
        bits 20-27: extended family id
        bits 28-31: reserved
        */
        auto processorType = (eax & 0x3000) >> 12;

        kprintf("\nCPUID\n-----\n");

        switch(processorType) {
            case 0: {
                kprintf("Processor Type: Original OEM Processor\n");
                break;
            }
            case 1: {
                kprintf("ProcessorType: Intel OverDrive Processor\n");
                break;
            }
            case 2: {
                kprintf("Processor Type: Dual processor\n");
                break;
            }
            case 3: {
                kprintf("Processor Type: Intel reserved\n");
                break;
            }
        }

        /*
        EBX:
        low byte: brand index
        second btte: CLFLUSH instruction cache line size
        high byte: local apic id
        */
        auto model = (eax & 0xf0) >> 4;
        auto familyId = (eax & 0xf00) >> 8;
        auto brandIndex = (ebx & 0xff);

        if ((familyId > 0x0F && model >= 0x03) || brandIndex) {
            kprintf("Brand Index is not supported\n");
        }
        else {
            kprintf("FamilyID: %x, Model: %x\n", familyId, model);

            char brandString[49];
            brandString[48] = '\0';
            getBrandString(brandString);
            kprintf("BrandString: %s\n", brandString);
        }

        kprintf("\nFeatures\n--------\n");

        kprintf("From ECX:\n");
        
        kprintf("SSE3: %d, ", (edx & (1u << 0)) > 0);    
        kprintf("PCLMULQDQ: %d, ", (edx & (1u << 1)) > 0);    
        kprintf("DTES64: %d, ", (edx & (1u << 2)) > 0);    
        kprintf("MONITOR: %d, ", (edx & (1u << 3)) > 0);    
        kprintf("DS-CPL: %d, ", (edx & (1u << 4)) > 0);    
        kprintf("VMX: %d, ", (edx & (1u << 5)) > 0);    
        kprintf("SMX: %d, ", (edx & (1u << 6)) > 0);    
        kprintf("EIST: %d", (edx & (1u << 7)) > 0);    

        kprintf("TM2: %d, ", (edx & (1u << 8)) > 0);    
        kprintf("SSSE3: %d, ", (edx & (1u << 9)) > 0);    
        kprintf("CNXT-ID: %d, ", (edx & (1u << 10)) > 0);    
        kprintf("SDBG: %d, ", (edx & (1u << 11)) > 0);    
        kprintf("FMA: %d, ", (edx & (1u << 12)) > 0);    
        kprintf("CMPXCHG16B: %d, ", (edx & (1u << 13)) > 0);    
        kprintf("xTPR: %d, ", (edx & (1u << 14)) > 0);    
        kprintf("PDCM: %d\n", (edx & (1u << 15)) > 0);    

        //bit 16 is reserved 
        kprintf("PCID: %d, ", (edx & (1u << 17)) > 0);    
        kprintf("DCA: %d, ", (edx & (1u << 18)) > 0);    
        kprintf("SSE4.1: %d, ", (edx & (1u << 19)) > 0);
        kprintf("SSE4.2: %d, ", (edx & (1u << 20)) > 0);
        kprintf("x2APIC: %d, ", (edx & (1u << 21)) > 0);    
        kprintf("MOVBE: %d, ", (edx & (1u << 22)) > 0);    
        kprintf("POPCNT: %d\n", (edx & (1u << 23)) > 0);   

        kprintf("TSC-Deadline: %d, ", (edx & (1u << 24)) > 0);  
        kprintf("AESNI: %d, ", (edx & (1u << 25)) > 0);    
        kprintf("XSAVE: %d, ", (edx & (1u << 26)) > 0);    
        kprintf("OSXSAVE: %d, ", (edx & (1u << 27)) > 0);    
        kprintf("AVX: %d, ", (edx & (1u << 28)) > 0);    
        kprintf("F16C: %d, ", (edx & (1u << 29)) > 0);    
        kprintf("RDRAND: %d\n", (edx & (1u << 30)) > 0);    
        //bit 31 is reserved

        /*EDX:
        */
        kprintf("\nFrom EDX:\n");

        kprintf("FPU: %d, ", (edx & (1u << 0)) > 0);    
        kprintf("VMU: %d, ", (edx & (1u << 1)) > 0);    
        kprintf("DE: %d, ", (edx & (1u << 2)) > 0);    
        kprintf("PSE: %d, ", (edx & (1u << 3)) > 0);    
        kprintf("TSC: %d, ", (edx & (1u << 4)) > 0);    
        kprintf("MSR: %d, ", (edx & (1u << 5)) > 0);    
        kprintf("PAE: %d, ", (edx & (1u << 6)) > 0);    
        kprintf("MCE: %d\n", (edx & (1u << 7)) > 0);    

        kprintf("CX8: %d, ", (edx & (1u << 8)) > 0);    
        kprintf("APIC: %d, ", (edx & (1u << 9)) > 0);    
        //bit 10 is reserved
        kprintf("SEP: %d, ", (edx & (1u << 11)) > 0);    
        kprintf("MTRR: %d, ", (edx & (1u << 12)) > 0);    
        kprintf("PGE: %d, ", (edx & (1u << 13)) > 0);    
        kprintf("MCA: %d, ", (edx & (1u << 14)) > 0);    
        kprintf("CMOV: %d\n", (edx & (1u << 15)) > 0);    

        kprintf("PAT: %d, ", (edx & (1u << 16)) > 0);    
        kprintf("PSE-36: %d, ", (edx & (1u << 17)) > 0);    
        kprintf("PSN: %d, ", (edx & (1u << 18)) > 0);    
        kprintf("CLFSH: %d, ", (edx & (1u << 19)) > 0);
        //bit 20 is reserved    
        kprintf("DS: %d, ", (edx & (1u << 21)) > 0);    
        kprintf("ACPI: %d, ", (edx & (1u << 22)) > 0);    
        kprintf("MMX: %d\n", (edx & (1u << 23)) > 0);   

        kprintf("FXSR: %d, ", (edx & (1u << 24)) > 0);  
        kprintf("SSE: %d, ", (edx & (1u << 25)) > 0);    
        kprintf("SSE2: %d, ", (edx & (1u << 26)) > 0);    
        kprintf("SS: %d, ", (edx & (1u << 27)) > 0);    
        kprintf("HTT: %d, ", (edx & (1u << 28)) > 0);    
        kprintf("TM: %d, ", (edx & (1u << 29)) > 0);    
        //bit 30 is reserved
        kprintf("PBE: %d\n", (edx & (1u << 31)) > 0);    

    }
}