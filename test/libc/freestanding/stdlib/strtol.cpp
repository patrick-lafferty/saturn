#include <test/libc/stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

template<typename T>
void assertEqual(T expected, T actual, const char* message) {
    if (expected != actual) {
        printf("    expected: %x, actual: %x, %s\n", expected, actual, message);
    }
}

bool canParseBase10_noExtraChars_baseGiven() {
    auto x = strtol("123456", nullptr, 10);
    assertEqual(123456L, x, "parsed wrong number");
    return x == 123456;
}

bool canParseBase10_ExtraHexCharsAfter_baseGiven() {
    char* end;
    auto x = strtol("123456abc", &end, 10);
    assertEqual(123456L, x, "parsed wrong number");
    assertEqual('a', *end, "str_end set incorrectly");
    return x == 123456 && *end == 'a';
}

bool canParseBase10_ExtraCharsAfter_baseGiven() {
    char* end;
    auto x = strtol("123456pat", &end, 10);
    assertEqual(123456L, x, "parsed wrong number");
    assertEqual('p', *end, "str_end set incorrectly");
    return x == 123456 && *end == 'p';
}

bool cantParseBase10_ExtraCharsBefore_baseGiven() {
    char* end;
    auto x = strtol("pat123456", &end, 10);
    assertEqual(0L, x, "parsed wrong number");
    assertEqual('p', *end, "str_end set incorrectly");
    return x == 0 && *end == 'p';
}

struct Test {
    bool (*func)();
    const char* funcName;
};

Test tests[] = {
    {canParseBase10_noExtraChars_baseGiven, "canParseBase10_noExtraChars_baseGiven"},
    {canParseBase10_ExtraHexCharsAfter_baseGiven, "canParseBase10_ExtraHexCharsAfter_baseGiven"},
    {canParseBase10_ExtraCharsAfter_baseGiven, "canParseBase10_ExtraCharsAfter_baseGiven"},
    {cantParseBase10_ExtraCharsBefore_baseGiven, "cantParseBase10_ExtraCharsBefore_baseGiven"}
};

int strtolTestRunner() {
    int passedTests = 0;

    for(auto& test: tests) {
        if (test.func()) {
            passedTests++;
        }
        else {
            printf("    strtol test failed: %s\n", test.funcName);
            return passedTests;
        }
    }

    return passedTests;
}

void runStrtolTests() {
    const int NumberOfTests {sizeof(tests) / sizeof(tests[0])};
    printf("Running strtol tests\n");

    auto passedTests = strtolTestRunner();

    if (passedTests == NumberOfTests) {
        printf("\n    All tests passed [%d/%d]\n", passedTests, NumberOfTests);
    }
    else {
        printf("\n    %d of %d tests passed\n", passedTests, NumberOfTests);
    }
}