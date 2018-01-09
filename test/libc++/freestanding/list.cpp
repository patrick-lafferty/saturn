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
#include <test/libc++/list.h>
#include <stdint.h>
#include <stdio.h>
#include <list>

namespace ListTests {
    const int NumberOfTests {2};

    bool canPushFront() {
        std::list<int> list;

        int i = 0;
        for (; i < 1024; i++) {
            list.push_front(i);
        }

        i--;

        for (auto item : list) {
            if (item != i) {
                return false;
            }

            i--;
        }

        return true;
    }

    bool canPopFront() {
        std::list<int> list;

        int i = 0;
        for (; i < 1024; i++) {
            list.push_front(i);
        }

        i--;

        while (!list.empty()) {
            auto value = list.front();
            list.pop_front();

            if (value != i) {
                return false;
            }

            i--;
        }

        return true;
    }


    int runner() {
        int tests = 0;

        if (canPushFront()) {
            tests++;
        }
        else {
            kprintf("    List test failed: canPushFront\n");
            return tests;
        }

        if (canPopFront()) {
            tests++;
        }
        else {
            kprintf("    List test failed: canPopFront\n");
            return tests;
        }

        return tests;
    }
}

void runListTests() {
    kprintf("\nRunning std::list tests\n");

    auto passedTests = ListTests::runner();

    if (passedTests == ListTests::NumberOfTests) {
        kprintf("\n    All tests passed [%d/%d]\n", passedTests, ListTests::NumberOfTests);
    }
    else {
        kprintf("\n    %d of %d tests passed\n", passedTests, ListTests::NumberOfTests);
    }
}