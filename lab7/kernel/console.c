// kernel/console.c
#include "defs.h"

// 控制台输出一个字符
// 目前它只是简单地调用 UART 驱动
void cons_putc(char c) {
    uart_putc(c);
}

int consolewrite(int user_src, uint64 src, int n) {
    (void)user_src;
    char *p = (char*)src;
    for (int i = 0; i < n; i++) {
        cons_putc(p[i]);
    }
    return n;
}

int consoleread(int user_dst, uint64 dst, int n) {
    (void)user_dst;
    (void)dst;
    (void)n;
    return -1;
}