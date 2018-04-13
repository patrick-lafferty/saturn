/*
Copyright (c) 2017, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdlib.h>
#include <ctype.h>

long strtol(const char* str, char** str_end, int base) {
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
            }

            goto end;
        }

        str++;
    }

    end:

    if (negate) {
        result *= -1;
    }

    return result;
}

long long int strtoll(const char* restrict nptr, char** restrict endptr, int base) {
    //TODO: implement stub
    return 0;
}