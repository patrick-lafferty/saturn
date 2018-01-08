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