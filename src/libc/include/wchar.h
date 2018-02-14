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
#pragma once

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mbstate_t {
    char placeholder[8];
};

typedef unsigned int wint_t;
struct tm;

#define WEOF -1

wchar_t* wcscpy(wchar_t* restrict s1, const wchar_t* restrict s2);
wchar_t* wcsncpy(wchar_t* restrict s1, const wchar_t* restrict s2, size_t n);
wchar_t* wmemmove(wchar_t* s1, const wchar_t* s2, size_t n);
wchar_t* wmemcpy(wchar_t* restrict s1, const wchar_t* restrict s2, size_t n);
wchar_t* wmemset(wchar_t* s, wchar_t c, size_t n);
wchar_t* wcschr(const wchar_t *s, wchar_t c);
wchar_t* wcspbrk(const wchar_t* s1, const wchar_t* s2);
wchar_t* wcsrchr(const wchar_t* s, wchar_t c);
wchar_t* wcsstr(const wchar_t* s1, const wchar_t* s2);
wchar_t* wmemchr(const wchar_t* s, wchar_t c, size_t n);
wchar_t* wcscat(wchar_t* restrict s1, const wchar_t* restrict s2);
wchar_t* wcsncat(wchar_t* restrict s1, const wchar_t* restrict s2, size_t n);

int wcscmp(const wchar_t* s1, const wchar_t* s2);
int wcscoll(const wchar_t* s1, const wchar_t* s2);
int wcsncmp(const wchar_t* s1, const wchar_t* s2, size_t n);
size_t wcsxfrm(wchar_t* restrict s1, const wchar_t* restrict s2, size_t n);
wchar_t* wcscspn(const wchar_t* s1, const wchar_t* s2);
size_t wcslen(const wchar_t* s);
size_t wcsspn(const wchar_t* s1, const wchar_t* s2);

wchar_t* wcstok(wchar_t* restrict s1, const wchar_t* restrict s2, wchar_t** restrict ptr);
int wmemcmp(const wchar_t* s1, const wchar_t* s2, size_t n);

size_t wcsftime(wchar_t* restrict s, 
	size_t maxsize,
	const wchar_t* restrict format,
	const struct tm* restrict timeptr);

wint_t btowc(int c);
int wctob(wint_t c);

int mbsinit(const mbstate_t* ps);
size_t mbrlen(const char* restrict s, size_t n, mbstate_t* restrict ps);
size_t mbrtowc(wchar_t* restrict pwc, const char* restrict s, size_t n, mbstate_t* restrict ps);
size_t wcrtomb(char* restrict s, wchar_t wc, mbstate_t* restrict ps);
size_t mbsrtowcs(wchar_t* restrict dst, const char** restrict src, size_t len, mbstate_t* restrict ps);
size_t wcsrtombs(char* restrict dst, const wchar_t** restrict src, size_t len, mbstate_t* restrict ps);

int fwprintf(FILE* restrict stream, const wchar_t* restrict format, ...);
int swprintf(wchar_t* restrict s, size_t n, const wchar_t* restrict format, ...);
int vfwprintf(FILE* restrict stream, const wchar_t* restrict format, va_list arg);
int vswprintf(wchar_t* restrict s, size_t n, const wchar_t* restrict format, va_list arg);
int vwprintf(const wchar_t* restrict format, va_list arg);
int wprintf(const wchar_t* restrict format, ...);

int fwscanf(FILE* restrict stream, const wchar_t* restrict format, ...);
int swscanf(const wchar_t* restrict s, const wchar_t* restrict format, ...);
int vfwscanf(FILE* restrict stream, const wchar_t* restrict format, va_list arg);
int vswscanf(const wchar_t* restrict s, const wchar_t* restrict format, va_list arg);
int vwscanf(const wchar_t* restrict format, va_list arg);
int wscanf(const wchar_t* restrict format, ...);

wint_t fgetwc(FILE* stream);
wchar_t* fgetws(wchar_t* restrict s, int n, FILE* restrict stream);
wint_t fputwc(wchar_t c, FILE* stream);
int fputws(const wchar_t* restrict s, FILE* restrict stream);
int fwide(FILE* stream, int mode);
wint_t getwc(FILE* stream);
wint_t getwchar(void);
wint_t putwc(wchar_t c, FILE* stream);
wint_t putwchar(wchar_t c);
wint_t ungetwc(wint_t c, FILE* stream);

double wcstod(const wchar_t* restrict nptr, wchar_t** restrict endptr);
float wcstof(const wchar_t* restrict nptr, wchar_t** restrict endptr);
long double wcstold(const wchar_t* restrict nptr, wchar_t** restrict endptr);
long int wcstol(const wchar_t* restrict nptr, wchar_t** restrict endptr, int base);
long long int wcstoll(const wchar_t* restrict nptr, wchar_t** restrict endptr, int base);
unsigned long int wcstoul(const wchar_t* restrict nptr, wchar_t** restrict endptr, int base);
unsigned long long int wcstoull(const wchar_t* restrict nptr,
	wchar_t** restrict endptr,
	int base);

size_t wcsnrtombs(char*, const wchar_t**, size_t, size_t, mbstate_t*);
size_t mbsnrtowcs(wchar_t*, const char**, size_t, size_t, mbstate_t*);

#ifdef __cplusplus
}
#endif