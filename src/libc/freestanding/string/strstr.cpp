#include <string.h>

char* strstr(const char* string, const char* substring) {
    auto substringLength = strlen(substring);

    if (substringLength == 0) {
        return const_cast<char*>(string);
    }

    auto stringLength = strlen(string);

    for (auto i = 0u; i < stringLength; i++) {
        if (string[i] == substring[i] 
            && (stringLength - i) >= substringLength) {

            bool matches {true};

            for (auto j = i + 1; j < substringLength; j++) {
                if (string[j] != substring[j]) {
                    matches = false;
                    break;
                }
            }

            if (matches) {
                return const_cast<char*>(string + i);
            }
        }        
    }

    return nullptr;
}