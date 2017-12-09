#include <stdint.h>
#include <stdio.h>

uintptr_t __stack_chk_guard = 0xe1a2c3d9;

extern "C" __attribute__((noreturn))
void __stack_chk_fail(void) {
    printf("Stack smashing detected!\n");

    while(true){}
}