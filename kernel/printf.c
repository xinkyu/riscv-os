// kernel/printf.c

#include <stdarg.h> // 用于处理可变参数

// 引入函数声明
#include "defs.h"

// 静态辅助函数，用于打印数字。
// base: 进制 (10 或 16)
// sign: 是否带符号 (1 表示带符号)
static void print_int(long long xx, int base, int sign) {
    char digits[] = "0123456789abcdef";
    char buf[32]; // 缓冲区足以存放64位整数
    int i = 0;
    unsigned long long x;

    if (sign && xx < 0) {
        uart_putc('-');
        x = -xx;
    } else {
        x = xx;
    }

    // 循环取余，将数字转换为字符存入缓冲区
    do {
        buf[i++] = digits[x % base];
    } while ((x /= base) != 0);

    // 逆序输出缓冲区中的字符
    while (--i >= 0) {
        uart_putc(buf[i]);
    }
}

// 内核的 printf 函数
void printf(const char *fmt, ...) {
    va_list args;
    char *s;
    int c;

    // 如果格式字符串为空，则不执行任何操作
    if (fmt == 0) {
        return;
    }

    va_start(args, fmt); // 初始化可变参数
    for (c = *fmt; c != '\0'; c = *++fmt) {
        if (c != '%') {
            uart_putc(c);
            continue;
        }

        c = *++fmt;
        if (c == '\0') break;

        switch (c) {
            case 'd': // 十进制整数
                print_int(va_arg(args, int), 10, 1);
                break;
            case 'x': // 十六进制整数
                print_int(va_arg(args, int), 16, 0);
                break;
            case 's': // 字符串
                s = va_arg(args, char *);
                if (s == 0) {
                    s = "(null)"; // 处理空指针的情况
                }
                uart_puts(s);
                break;
            case '%': // 百分号
                uart_putc('%');
                break;
            default: // 不支持的格式
                uart_putc('%');
                uart_putc(c);
                break;
        }
    }
    va_end(args); // 清理可变参数
}

// 清屏函数
void clear_screen() {
    // 使用ANSI转义序列: \033[2J 清屏, \033[H 光标归位
    uart_puts("\033[2J\033[H");
}