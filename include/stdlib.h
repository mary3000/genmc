#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <stddef.h>

void exit(int);

void abort(void);
int abs(int);
int atoi(const char *);
void free(void *);
char *getenv(const char *);
void *malloc(size_t);

void arc4random_buf(void * __buf, size_t __nbytes) {

}
char *realpath(const char * __restrict a, char * __restrict b)  {
    return NULL;
}
int	 atexit(void (* a)(void)) {
    return 0;
}
long strtol(const char * a, char ** b, int c) {
    return 0;
}


#endif /* __STDLIB_H__ */
