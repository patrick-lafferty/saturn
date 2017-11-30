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

    printf("vendor: %s\n", vendor);
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

        printf("\nCPUID\n-----\n");

        switch(processorType) {
            case 0: {
                printf("Processor Type: Original OEM Processor\n");
                break;
            }
            case 1: {
                printf("ProcessorType: Intel OverDrive Processor\n");
                break;
            }
            case 2: {
                printf("Processor Type: Dual processor\n");
                break;
            }
            case 3: {
                printf("Processor Type: Intel reserved\n");
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
            printf("Brand Index is not supported\n");
        }
        else {
            printf("FamilyID: %x, Model: %x\n", familyId, model);

            char brandString[49];
            brandString[48] = '\0';
            getBrandString(brandString);
            printf("BrandString: %s\n", brandString);
        }

        printf("\nFeatures\n--------\n");

        printf("From ECX:\n");
        
        printf("SSE3: %d, ", (edx & (1u << 0)) > 0);    
        printf("PCLMULQDQ: %d, ", (edx & (1u << 1)) > 0);    
        printf("DTES64: %d, ", (edx & (1u << 2)) > 0);    
        printf("MONITOR: %d, ", (edx & (1u << 3)) > 0);    
        printf("DS-CPL: %d, ", (edx & (1u << 4)) > 0);    
        printf("VMX: %d, ", (edx & (1u << 5)) > 0);    
        printf("SMX: %d, ", (edx & (1u << 6)) > 0);    
        printf("EIST: %d", (edx & (1u << 7)) > 0);    

        printf("TM2: %d, ", (edx & (1u << 8)) > 0);    
        printf("SSSE3: %d, ", (edx & (1u << 9)) > 0);    
        printf("CNXT-ID: %d, ", (edx & (1u << 10)) > 0);    
        printf("SDBG: %d, ", (edx & (1u << 11)) > 0);    
        printf("FMA: %d, ", (edx & (1u << 12)) > 0);    
        printf("CMPXCHG16B: %d, ", (edx & (1u << 13)) > 0);    
        printf("xTPR: %d, ", (edx & (1u << 14)) > 0);    
        printf("PDCM: %d\n", (edx & (1u << 15)) > 0);    

        //bit 16 is reserved 
        printf("PCID: %d, ", (edx & (1u << 17)) > 0);    
        printf("DCA: %d, ", (edx & (1u << 18)) > 0);    
        printf("SSE4.1: %d, ", (edx & (1u << 19)) > 0);
        printf("SSE4.2: %d, ", (edx & (1u << 20)) > 0);
        printf("x2APIC: %d, ", (edx & (1u << 21)) > 0);    
        printf("MOVBE: %d, ", (edx & (1u << 22)) > 0);    
        printf("POPCNT: %d\n", (edx & (1u << 23)) > 0);   

        printf("TSC-Deadline: %d, ", (edx & (1u << 24)) > 0);  
        printf("AESNI: %d, ", (edx & (1u << 25)) > 0);    
        printf("XSAVE: %d, ", (edx & (1u << 26)) > 0);    
        printf("OSXSAVE: %d, ", (edx & (1u << 27)) > 0);    
        printf("AVX: %d, ", (edx & (1u << 28)) > 0);    
        printf("F16C: %d, ", (edx & (1u << 29)) > 0);    
        printf("RDRAND: %d\n", (edx & (1u << 30)) > 0);    
        //bit 31 is reserved

        /*EDX:
        */
        printf("\nFrom EDX:\n");

        printf("FPU: %d, ", (edx & (1u << 0)) > 0);    
        printf("VMU: %d, ", (edx & (1u << 1)) > 0);    
        printf("DE: %d, ", (edx & (1u << 2)) > 0);    
        printf("PSE: %d, ", (edx & (1u << 3)) > 0);    
        printf("TSC: %d, ", (edx & (1u << 4)) > 0);    
        printf("MSR: %d, ", (edx & (1u << 5)) > 0);    
        printf("PAE: %d, ", (edx & (1u << 6)) > 0);    
        printf("MCE: %d\n", (edx & (1u << 7)) > 0);    

        printf("CX8: %d, ", (edx & (1u << 8)) > 0);    
        printf("APIC: %d, ", (edx & (1u << 9)) > 0);    
        //bit 10 is reserved
        printf("SEP: %d, ", (edx & (1u << 11)) > 0);    
        printf("MTRR: %d, ", (edx & (1u << 12)) > 0);    
        printf("PGE: %d, ", (edx & (1u << 13)) > 0);    
        printf("MCA: %d, ", (edx & (1u << 14)) > 0);    
        printf("CMOV: %d\n", (edx & (1u << 15)) > 0);    

        printf("PAT: %d, ", (edx & (1u << 16)) > 0);    
        printf("PSE-36: %d, ", (edx & (1u << 17)) > 0);    
        printf("PSN: %d, ", (edx & (1u << 18)) > 0);    
        printf("CLFSH: %d, ", (edx & (1u << 19)) > 0);
        //bit 20 is reserved    
        printf("DS: %d, ", (edx & (1u << 21)) > 0);    
        printf("ACPI: %d, ", (edx & (1u << 22)) > 0);    
        printf("MMX: %d\n", (edx & (1u << 23)) > 0);   

        printf("FXSR: %d, ", (edx & (1u << 24)) > 0);  
        printf("SSE: %d, ", (edx & (1u << 25)) > 0);    
        printf("SSE2: %d, ", (edx & (1u << 26)) > 0);    
        printf("SS: %d, ", (edx & (1u << 27)) > 0);    
        printf("HTT: %d, ", (edx & (1u << 28)) > 0);    
        printf("TM: %d, ", (edx & (1u << 29)) > 0);    
        //bit 30 is reserved
        printf("PBE: %d\n", (edx & (1u << 31)) > 0);    

    }
}