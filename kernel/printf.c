// kernel/printf.c
#include <stdarg.h>
#include "defs.h" // 引入函数声明

// 静态辅助函数，用于打印数字
// 重要：将此函数内部的 uart_putc 调用也改为 cons_putc
static void print_int(long long xx, int base, int sign) {
    char digits[] = "0123456789abcdef";
    char buf[32];
    int i = 0;
    unsigned long long x;

    if (sign && xx < 0) {
        cons_putc('-'); // <--- 修改点
        x = -xx;
    } else {
        x = xx;
    }

    do {
        buf[i++] = digits[x % base];
    } while ((x /= base) != 0);

    while (--i >= 0) {
        cons_putc(buf[i]); // <--- 修改点
    }
}

// 内核的 printf 函数
void printf(const char *fmt, ...) {
    va_list args;
    char *s;
    int c;

    if (fmt == 0) {
        return;
    }

    va_start(args, fmt);
    for (c = *fmt; c != '\0'; c = *++fmt) {
        if (c != '%') {
            cons_putc(c); // <--- 修改点
            continue;
        }

        c = *++fmt;
        if (c == '\0') break;

        switch (c) {
            case 'd':
                print_int(va_arg(args, int), 10, 1);
                break;
            case 'x':
                print_int(va_arg(args, int), 16, 0);
                break;
            case 's':
                s = va_arg(args, char *);
                if (s == 0) {
                    s = "(null)";
                }
                // 循环调用 cons_putc 来输出字符串
                while (*s != '\0') {
                    cons_putc(*s);
                    s++;
                }
                break;
            case '%':
                cons_putc('%'); // <--- 修改点
                break;
            default:
                cons_putc('%'); // <--- 修改点
                cons_putc(c); // <--- 修改点
                break;
        }
    }
    va_end(args);
}

// 清屏函数
void clear_screen() {
    // 使用循环调用 cons_putc 来发送转义序列
    const char *seq = "\033[2J\033[H";
    while (*seq) {
        cons_putc(*seq++);
    }
}