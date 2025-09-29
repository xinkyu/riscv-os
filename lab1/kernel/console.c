// kernel/console.c
#include "defs.h"

// 控制台输出一个字符
// 目前它只是简单地调用 UART 驱动
void cons_putc(char c) {
    uart_putc(c);
}