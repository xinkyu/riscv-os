// kernel/string.c
#include "defs.h"

void* memset(void *dst, int c, uint64 n) {
    char *cdst = (char *) dst;
    for (uint64 i = 0; i < n; i++) {
        cdst[i] = c;
    }
    return dst;
}

void* memmove(void *dst, const void *src, uint64 n) {
    const char *s = src;
    char *d = dst;
    
    if (s < d && s + n > d) {
        s += n;
        d += n;
        while (n-- > 0)
            *--d = *--s;
    } else {
        while (n-- > 0)
            *d++ = *s++;
    }
    return dst;
}

int strlen(const char *s) {
    int n;
    for (n = 0; s[n]; n++)
        ;
    return n;
}

int strncmp(const char *p, const char *q, uint64 n) {
    while(n > 0 && *p && *p == *q)
        n--, p++, q++;
    if(n == 0)
        return 0;
    return (unsigned char)*p - (unsigned char)*q;
}

char* strncpy(char *s, const char *t, int n) {
    char *os = s;
    while(n-- > 0 && (*s++ = *t++) != 0)
        ;
    while(n-- > 0)
        *s++ = 0;
    return os;
}