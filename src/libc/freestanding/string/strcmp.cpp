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