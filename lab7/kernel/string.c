#include "riscv.h"

int memcmp(const void *v1, const void *v2, uint n) {
    const uchar *s1 = v1;
    const uchar *s2 = v2;
    while (n-- > 0) {
        if (*s1 != *s2) {
            return *s1 - *s2;
        }
        s1++;
        s2++;
    }
    return 0;
}

void *memset(void *dst, int c, uint n) {
    char *d = dst;
    for (uint i = 0; i < n; i++) {
        d[i] = c;
    }
    return dst;
}

void *memmove(void *dst, const void *src, uint n) {
    const char *s = src;
    char *d = dst;
    if (s < d && s + n > d) {
        s += n;
        d += n;
        while (n-- > 0) {
            *--d = *--s;
        }
    } else {
        while (n-- > 0) {
            *d++ = *s++;
        }
    }
    return dst;
}

void *memcpy(void *dst, const void *src, uint n) {
    return memmove(dst, src, n);
}

int strncmp(const char *p, const char *q, uint n) {
    while (n > 0 && *p && *p == *q) {
        p++;
        q++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return (uchar)*p - (uchar)*q;
}

char *strncpy(char *s, const char *t, int n) {
    int i;
    for (i = 0; i < n && t[i]; i++) {
        s[i] = t[i];
    }
    for (; i < n; i++) {
        s[i] = 0;
    }
    return s;
}

int strlen(const char *s) {
    int n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

char *safestrcpy(char *s, const char *t, int n) {
    if (n <= 0) {
        return s;
    }
    int i;
    for (i = 0; i < n - 1 && t[i]; i++) {
        s[i] = t[i];
    }
    s[i] = 0;
    return s;
}
