#include "../runner.h"
#include <test/libc/stdlib.h>

void runAllLibCTests() {
    runMallocTests();
    runStrtolTests();
}