#include <stdio.h>
#include "file.h"

long int ftell(FILE* stream) {
    return stream->position;
}