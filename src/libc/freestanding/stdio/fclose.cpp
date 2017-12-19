#include <stdio.h>
#include "file.h"
#include <system_calls.h>

int fclose(FILE* stream) {
    close(stream->descriptor);
    delete stream;
    return 0;
}