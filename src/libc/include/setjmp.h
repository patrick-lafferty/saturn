#pragma once

typedef int jmp_buf[6];

extern int setjmp(jmp_buf env);
extern _Noreturn void longjmp(jmp_buf env, int val);