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

#ifdef __cplusplus
extern "C" {
#endif

void* malloc(size_t size);
void free(void* ptr);

void* aligned_alloc(size_t alignment, size_t size);
void* realloc(void* ptr, size_t size);

//TODO: we claim to be msvc-like to avoid using posix_memalign, so libcxx expects this
#define _aligned_malloc(size, alignment) aligned_alloc(alignment, size)
#define _aligned_free free

long strtol(const char* str, char** str_end, int base);

void qsort(void* base, size_t count, size_t size,
    int (*compare)(const void*, const void*));

//TODO
struct div_t {
	int quot;
	int rem;
};

struct ldiv_t {
	long int quot;
	long int rem;
};

struct lldiv_t {
	long long int quot;
	long long int rem;
};

div_t div(int numer, int denom);
ldiv_t ldiv(long int numer, long int denom);
lldiv_t lldiv(long long int numer, long long int denon);

double atof(const char* nptr);
int atoi(const char* nptr);
long int atol(const char* nptr);
long long int atoll(const char* nptr);

double strtod(const char* restrict nptr, char** restrict endptr);
float strtof(const char* restrict nptr, char** restrict endptr);
long double strtold(const char* restrict nptr, char** restrict endptr);

long long int strtoll(const char* restrict nptr, char** restrict endptr, int base);
unsigned long int strtoul(const char* restrict nptr, char** restrict endptr, int base);
unsigned long long int strtoull(const char* restrict nptr, char** restrict endptr, int base);

int rand(void);
void srand(unsigned int seed);

void* calloc(size_t nmemb, size_t size);

_Noreturn void abort(void);
int atexit(void (*func)(void));
_Noreturn void exit(int status);
_Noreturn void _Exit(int status);

char* getenv(const char* name);
int system(const char* string);

void* bsearch(const void* key, 
	const void* base, 
	size_t nmemb, 
	size_t size, 
	int (*compar)(const void*));

int abs(int j);
long int labs(long int j);
long long int llabs(long long int j);

int mblen(const char* s, size_t n);
int mbtowc(wchar_t* restrict pwc, const char* restrict s, size_t n);
int wctomb(char* s, wchar_t wchar);
size_t mbstowcs(wchar_t* restrict pwcs, const char* restrict s, size_t n);
size_t wcstombs(char* restrict s, const wchar_t* restrict pwcs, size_t n);

#define MB_CUR_MAX 0

typedef int errno_t;
errno_t rand_s(unsigned int*);

#ifdef __cplusplus
}
#endif