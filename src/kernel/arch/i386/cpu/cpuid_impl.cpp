#include "cpuid.h"
#include <stdio.h>

void cpuid_vendor_impl(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
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
                : "eax", "ebx", "ecx", "edx"
            );

            extractCharacters(buffer, i * 16, eax, ebx, ecx, edx);
        }
    }

    void printVersionInformation() {
        uint32_t eax, ebx, ecx, edx;

        asm("cpuid"
            : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
            : "a" (1)
            : "eax", "ebx", "ecx", "edx"
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

        if (familyId > 0x0F && model >= 0x03 || brandIndex) {
            printf("Brand Index is not supported\n");
        }
        else {
            printf("FamilyID: %x, Model: %x\n", familyId, model);

            char brandString[49];
            brandString[48] = '\0';
            getBrandString(brandString);
            printf("BrandString: %s\n", brandString);
        }

    }
}