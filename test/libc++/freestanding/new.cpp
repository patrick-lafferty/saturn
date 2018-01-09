/*
Copyright (c) 2017, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
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