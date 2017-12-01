#pragma once

extern "C" {
    int kprintf(const char* format, ...);
    //int printf(const char* format, ...);
    int sprintf(char* buffer, const char* format, ...);
}