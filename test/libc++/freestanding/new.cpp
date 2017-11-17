#include <test/libc++/new.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

namespace NewTests {
    const int NumberOfTests {3};

    bool canAllocateSequentialInts() {

        int* x = new int;
        uint32_t startingAddress = reinterpret_cast<uint32_t>(x);

        for (int i = 1; i < 100; i++) {
            int* something = new int;
            auto address = reinterpret_cast<uint32_t>(something);

            if (address != startingAddress + i * sizeof(int)) {
                return false;
            }
        }

        return true;
    }

    bool canAllocateMoreThanAPage() {
        int* x = new int;
        uint32_t startingAddress = reinterpret_cast<uint32_t>(x); 

        for (int i = 1; i < 2000; i++) {
            int* something = new int; 
            auto address = reinterpret_cast<uint32_t>(something);

            if (address != startingAddress + i * sizeof(int)) {
                return false;
            }
        }

        return true;
    }

    class Test {
    public:

        Test(bool* constructorCalled) {
            *constructorCalled = true;
        }
    
    private:
        int x, y, z;
    };

    bool canAllocateClass() {
        bool constructorCalled {false};
        auto test = new Test(&constructorCalled);
        return test != nullptr && constructorCalled;
    }

    int runner() {
        int tests = 0;

        if (canAllocateSequentialInts()) {
            tests++;    
        }
        else {
            printf("    New test failed: canAllocateSequentialInts\n");
            return tests;
        }

        if (canAllocateMoreThanAPage()) {
            tests++;    
        }
        else {
            printf("    New test failed: canAllocateMoreThanAPage\n");
            return tests;
        }

        if (canAllocateClass()) {
            tests++;    
        }
        else {
            printf("    New test failed: canAllocateClass\n");
            return tests;
        }

        return tests;
    }
}

void runNewTests() {
    printf("\nRunning new tests\n");

    auto passedTests = NewTests::runner();

    if (passedTests == NewTests::NumberOfTests) {
        printf("\n    All tests passed [%d/%d]\n", passedTests, NewTests::NumberOfTests);
    }
    else {
        printf("\n    %d of %d tests passed\n", passedTests, NewTests::NumberOfTests);
    }
}