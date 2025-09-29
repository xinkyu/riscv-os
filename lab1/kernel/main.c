// kernel/main.c for Experiment 1
#include "defs.h"

// 在 uart.c 中定义的函数，用于打印字符串
void uart_puts(const char *s);

void kmain(void) {
    // 直接调用串口输出 "Hello OS"
    uart_puts("Hello OS!\n");

    // 内核不退出，进入无限循环
    while (1);
}