#ifndef __STDIO_H__
#define __STDIO_H__

#include <stddef.h>

struct _IO_FILE;
typedef struct _IO_FILE FILE;

extern struct _IO_FILE *stdin;
extern struct _IO_FILE *stdout;
extern struct _IO_FILE *stderr;

extern int fclose(FILE *);
extern int fflush(FILE *);
extern FILE *fopen(const char *, const char *);
extern int printf(const char *, ...);
extern int fprintf(FILE *, const char *, ...);
extern size_t fwrite(const void *, size_t, size_t, FILE *);

int	snprintf(char * __restrict __str, size_t __size, const char * __restrict __format, ...) {
    // nothing
    return 0;
}

int	 fputs(const char * __restrict a, FILE * __restrict b) {
    return 0;
}

// int vsnprintf(char * __restrict __str, size_t __size, const char * __restrict __format, struct __va_list_tag *);

#define	stdout	NULL
#define	stderr	NULL

#endif /* __STDIO_H__ */
