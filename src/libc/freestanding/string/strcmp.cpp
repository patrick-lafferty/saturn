#include <string.h>

int strcmp(const char* lhs, const char* rhs) {
    while (*lhs != '\0' && *rhs != '\0') {
        if (static_cast<unsigned char>(*lhs) < static_cast<unsigned char>(*rhs)) {
            return -1;
        }
        else if (*lhs > *rhs) {
            return 1;
        }

        lhs++;
        rhs++;
    }

    return 0;
}

int strncmp(const char* lhs, const char* rhs, size_t count) {
    while (count > 0 && *lhs != '\0' && *rhs != '\0') {
        if (static_cast<unsigned char>(*lhs) < static_cast<unsigned char>(*rhs)) {
            return -1;
        }
        else if (*lhs > *rhs) {
            return 1;
        }

        lhs++;
        rhs++;
        count--;
    }

    return 0;
}