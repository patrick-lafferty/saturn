#include <stdio.h>
#include "file.h"
#include <system_calls.h>

FILE* fopen(const char* restrict filename, const char* restrict mode) {
    auto result = openSynchronous(filename);

    if (result.success) {
        auto file = new FILE;
        file->descriptor = result.fileDescriptor;
        file->position = 0;
        file->error = 0;
        
        return file;
    }
    else {
        return nullptr;
    }

}