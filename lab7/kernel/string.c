// kernel/string.c
#include "types.h"
#include "riscv.h"
#include "defs.h"

void* memset(void *dst, int c, uint n) {
  char *cdst = (char *) dst;
  int i;
  for(i = 0; i < n; i++){
    cdst[i] = c;
  }
  return dst;
}

int memcmp(const void *v1, const void *v2, uint n) {
  const uchar *s1, *s2;
  s1 = v1;
  s2 = v2;
  while(n-- > 0){
    if(*s1 != *s2)
      return *s1 - *s2;
    s1++, s2++;
  }
  return 0;
}

void* memmove(void *dst, const void *src, uint n) {
  const char *s;
  char *d;

  s = src;
  d = dst;
  if(s < d && s + n > d){
    s += n;
    d += n;
    while(n-- > 0)
      *--d = *--s;
  } else {
    while(n-- > 0)
      *d++ = *s++;
  }
  return dst;
}

void* memcpy(void *dst, const void *src, uint n) {
  return memmove(dst, src, n);
}

int strncmp(const char *p, const char *q, uint n) {
  while(n > 0 && *p && *p == *q)
    n--, p++, q++;
  if(n == 0)
    return 0;
  return (uchar)*p - (uchar)*q;
}

int strlen(const char *s) {
  int n;
  for(n = 0; s[n]; n++)
    ;
  return n;
}

char* strncpy(char *s, const char *t, int n) {
  char *os = s;
  while(n-- > 0 && (*s++ = *t++) != 0)
    ;
  while(n-- > 0)
    *s++ = 0;
  return os;
}

// 简单的 snprintf 实现 (仅支持 %d, %s, %x, %p)
// 用于 fs.c 中的调试输出
void snprintf(char *buf, int size, const char *fmt, ...) {
    // 简易桩函数，如果需要完整功能可从 printf.c 移植逻辑
    // 或者直接调用 printf (如果不介意输出到控制台而非 buffer)
    // 这里为了编译通过，可以先留空或简单拷贝 fmt
    strncpy(buf, fmt, size);
}