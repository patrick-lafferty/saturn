/*
Copyright (c) 2018, Patrick Lafferty
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
#pragma once

#ifdef __cplusplus
extern "C" {
#endif


struct lconv {
    char *decimal_point;       // "."
    char *thousands_sep;       // ""
    char *grouping;            // ""
    char *mon_decimal_point;   // ""
    char *mon_thousands_sep;   // ""
    char *mon_grouping;        // ""
    char *positive_sign;       // ""
    char *negative_sign;       // ""
    char *currency_symbol;     // ""
    char frac_digits;          // CHAR_MAX
    char p_cs_precedes;        // CHAR_MAX
    char n_cs_precedes;        // CHAR_MAX
    char p_sep_by_space;       // CHAR_MAX
    char n_sep_by_space;       // CHAR_MAX
    char p_sign_posn;          // CHAR_MAX
    char n_sign_posn;          // CHAR_MAX
    char *int_curr_symbol;     // ""
    char int_frac_digits;      // CHAR_MAX
    char int_p_cs_precedes;    // CHAR_MAX
    char int_n_cs_precedes;    // CHAR_MAX
    char int_p_sep_by_space;   // CHAR_MAX
    char int_n_sep_by_space;   // CHAR_MAX
    char int_p_sign_posn;      // CHAR_MAX
    char int_n_sign_posn;      // CHAR_MAX
};

#define LC_ALL 0
#define LC_COLLATE 1
#define LC_CTYPE 2
#define LC_MONETARY 3
#define LC_NUMERIC 4
#define LC_TIME 5

char *setlocale(int category, const char *locale);
struct lconv *localeconv(void);

//posix junk that will never be used, but libc++ wants it
typedef void* locale_t;

locale_t uselocale(locale_t);
void freelocale(locale_t);
locale_t newlocale(int, const char*, int);

#define LC_ALL_MASK (LC_COLLATE | LC_CTYPE | LC_MONETARY | LC_NUMERIC | LC_TIME)
#define LC_COLLATE_MASK LC_COLLATE
#define LC_CTYPE_MASK LC_CTYPE
#define LC_MONETARY_MASK LC_MONETARY
#define LC_NUMERIC_MASK LC_NUMERIC
#define LC_TIME_MASK LC_TIME
#define LC_MESSAGES_MASK 0

int isascii(int);

double strtod_l(const char* restrict nptr, char** restrict endptr, locale_t);
float strtof_l(const char* restrict nptr, char** restrict endptr, locale_t);
long double strtold_l(const char* restrict nptr, char** restrict endptr, locale_t);
long long int strtoll_l(const char* restrict nptr, char** restrict endptr, int base, locale_t);
unsigned long long int strtoull_l(const char* restrict nptr, char** restrict endptr, int base, locale_t);

int isxdigit_l(int, locale_t);
int isdigit_l(int, locale_t);

int strcoll_l(const char* s1, const char* s2, locale_t);
size_t strxfrm_l(char* restrict s1, const char* restrict s2, size_t n, locale_t);

int wcscoll_l(const wchar_t* s1, const wchar_t* s2, locale_t);
size_t wcsxfrm_l(wchar_t* restrict s1, const wchar_t* restrict s2, size_t n, locale_t);

int iswlower_l(wint_t, locale_t);
int islower_l(int, locale_t);
int isupper_l(int, locale_t);
int toupper_l(wchar_t, locale_t);
int tolower_l(int, locale_t);
int iswspace_l(int, locale_t);
int iswprint_l(int, locale_t);
int iswcntrl_l(int, locale_t);
int iswupper_l(int, locale_t);
int iswalpha_l(int, locale_t);
int iswdigit_l(int, locale_t);
int iswpunct_l(int, locale_t);
int iswxdigit_l(int, locale_t);
int iswblank_l(int, locale_t);
int towupper_l(int, locale_t);
int towlower_l(int, locale_t);

size_t strftime_l(char* restrict s,
    size_t maxSize,
    const char* restrict format,
    const struct tm* restrict timeptr, locale_t);

#ifdef __cplusplus
}
#endif