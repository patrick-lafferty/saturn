#include <string.h>

int memcmp(const void* first, const void* second, size_t count) {

    auto lhs = static_cast<const unsigned char*>(first);
    auto rhs = static_cast<const unsigned char*>(second);

    for(auto i = 0; i < count; i++) {
        if (lhs[i] > rhs[i]) {
            return 1;
        }
        else if (lhs[i] < rhs[i]) {
            return -1;
        }
    }

    return 0;
}