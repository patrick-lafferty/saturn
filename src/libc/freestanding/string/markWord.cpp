#include <string.h>

char* markWord(char* input, char separator) {

    if (*input == '\0') {
        return nullptr;
    }

    while (*input != '\0' && *input != separator) {
        input++;
    }

    *input = '\0';
    return input;
}