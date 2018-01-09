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

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int kprintf(const char* format, ...);
int printf(const char* format, ...);
int sprintf(char* buffer, const char* format, ...);

typedef struct _file FILE;
typedef struct fpost fpos_t;

#define EOF -1
#define FOPEN_MAX 8
#define FILENAME_MAX 255
#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 3

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

int fclose(FILE* stream);
int fflush(FILE* stream);
FILE* fopen(const char* restrict filename, const char* restrict mode);
int fprintf(FILE* restrict stream,
    const char* restrict format, ...);
int fscanf(FILE* restrict stream,
    const char* restrict format, ...);

int fgetc(FILE* stream);
char* fgets(char* restrict s, int n,
    FILE* restrict stream);
int fputc(int c, FILE* stream);
int fputs(const char* restrict s,
    FILE* restrict stream);
int getc(FILE* stream);
int putc(int c, FILE* stream);

size_t fread(void* restrict ptr, size_t size, size_t count, FILE* restrict stream);
size_t fwrite(const void* restrict ptr,
    size_t size, size_t count,
    FILE* restrict stream);
int fgetpos(FILE* restrict stream,
    fpos_t* restrict pos);
int fseek(FILE* stream, long int offset, int whence);
int fsetpos(FILE* stream, const fpos_t* pos);
long int ftell(FILE* stream);


#ifdef __cplusplus
}
#endif