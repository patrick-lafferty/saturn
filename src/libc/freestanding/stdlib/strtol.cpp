#include <stdlib.h>
#include <ctype.h>

long strtol(const char* restrict str, char** restrict str_end, int base) {
    while(*str != '\0') {
        if (isspace(*str)) {
            str++;
        }
        else {
            break;
        }
    }

    long result {0};
    bool prefixAllowed {base == 0 || base == 8 || base == 16};
    bool negate = false;

    while(*str != '\0') {
        if (isxdigit(*str)) {
            switch(*str) {
                case '+': {
                    break;
                }
                case '-': {
                    negate = true;
                    break;
                }
                case '0': {
                    if (prefixAllowed) {
                        if (*(str + 1) == 'x' || *(str + 1) == 'X') {
                            if (base == 0) {
                                base = 16;
                            }
                        }
                        else if (base == 0) {
                            base = 8;
                        }

                        prefixAllowed = false;
                    }
                    [[fallthrough]];
                }
                default: {
                    if (prefixAllowed) {
                        //we haven't found an octal or hex prefix yet, and instead have a (possibly hex) digit
                        prefixAllowed = false;
                        if (base == 0) {
                            base = 10;
                        }
                    }

                    if (base <= 10 && isalpha(*str)) {
                        //found a letter in a-f or A-F but we're in base 10 or less, stop here
                        if (str_end != nullptr) {
                            *str_end = const_cast<char*>(str);
                        }

                        goto end;
                    }

                    result *= base;

                    if (isdigit(*str)) {
                        result += *str - '0';
                    } 
                    else if (isupper(*str)) {
                        result += (*str - 'A') + 10;
                    }
                    else {
                        result += (*str - 'a') + 10;
                    }

                    break;
                }
            }
        }
        else {
            if (str_end != nullptr) {
                *str_end = const_cast<char*>(str);
                goto end;
            }
        }

        str++;
    }

    end:

    if (negate) {
        result *= -1;
    }

    return result;
}