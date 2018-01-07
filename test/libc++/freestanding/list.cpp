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