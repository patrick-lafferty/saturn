#include <test/libc/stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

const int NumberOfTests {2};

bool canAllocateSequentialInts() {

    int* x = static_cast<int*>(malloc(sizeof(int)));
    uint32_t startingAddress = reinterpret_cast<uint32_t>(x);

    for (int i = 1; i < 100; i++) {
        int* something = static_cast<int*>(malloc(sizeof(int)));
        auto address = reinterpret_cast<uint32_t>(something);

        if (address != startingAddress + i * sizeof(int)) {
            return false;
        }
    }

    return true;
}

bool canAllocateMoreThanAPage() {
    int* x = static_cast<int*>(malloc(sizeof(int)));
    uint32_t startingAddress = reinterpret_cast<uint32_t>(x); 
    auto size = sizeof(int) * 40;

    for (int i = 1; i < 1000; i++) {
        int* something = static_cast<int*>(malloc(size));
        auto address = reinterpret_cast<uint32_t>(something);

        if (address != startingAddress + i * size) {
            return false;
        }
    }

    return true;
}

int mallocTestRunner() {
    int tests = 0;

    if (canAllocateSequentialInts()) {
        tests++;    
    }
    else {
        printf("    Malloc test failed: canAllocateSequentialInts\n");
        return tests;
    }

    if (canAllocateMoreThanAPage()) {
        tests++;    
    }
    else {
        printf("    Malloc test failed: canAllocateMoreThanAPage\n");
        return tests;
    }

    return tests;
}

void runMallocTests() {
    printf("Running malloc tests\n");

    auto passedTests = mallocTestRunner();

    if (passedTests == NumberOfTests) {
        printf("\n    All tests passed [%d/%d]\n", passedTests, NumberOfTests);
    }
    else {
        printf("\n    %d of %d tests passed\n", passedTests, NumberOfTests);
    }
}