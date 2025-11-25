// kernel/printf.c
#include <stdarg.h>
#include "defs.h"
#include "riscv.h" // 引入 riscv.h 以使用 uint64 类型

// 静态辅助函数，用于打印不同进制的数字。
static void print_int(long long xx, int base, int sign) {
    char digits[] = "0123456789abcdef";
    char buf[32];
    int i = 0;
    unsigned long long x;

    if (sign && xx < 0) {
        cons_putc('-');
        x = -xx;
    } else {
        x = xx;
    }

    do {
        buf[i++] = digits[x % base];
    } while ((x /= base) != 0);

    while (--i >= 0) {
        cons_putc(buf[i]);
    }
}

// --- 新增：专门用于打印64位地址的辅助函数 ---
static void print_ptr(uint64 x) {
    cons_putc('0');
    cons_putc('x');
    int i;
    char buf[16];
    for (i = 0; i < 16; i++) {
        buf[i] = "0123456789abcdef"[x % 16];
        x /= 16;
    }
    for (i = 15; i >= 0; i--) {
        cons_putc(buf[i]);
    }
}

// 内核的 printf 函数本体
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
            cons_putc(c);
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
            case 'p':
                print_ptr(va_arg(args, uint64));
                break;
            case 's':
                s = va_arg(args, char *);
                if (s == 0) {
                    s = "(null)";
                }
                while (*s != '\0') {
                    cons_putc(*s++);
                }
                break;
            // [新增] 支持 %l 前缀 (如 %ld, %lu)
            case 'l':
                c = *++fmt; // 获取 'l' 后面的字符
                if (c == 'd') {
                    print_int(va_arg(args, long), 10, 1);
                } else if (c == 'u') {
                    print_int(va_arg(args, long), 10, 0); // 无符号十进制
                } else if (c == 'x') {
                    print_int(va_arg(args, long), 16, 0);
                } else {
                    // 如果是不支持的格式，回退一点
                    cons_putc('%');
                    cons_putc('l');
                    cons_putc(c);
                }
                break;
            case '%':
                cons_putc('%');
                break;
            default:
                // 打印未知的格式符
                cons_putc('%');
                cons_putc(c);
                break;
        }
    }
    va_end(args);
}

// 清屏函数
void clear_screen() {
    const char *seq = "\033[2J\033[H";
    while (*seq) {
        cons_putc(*seq++);
    }
}

void panic(const char *msg) {
    printf("\npanic: %s\n", msg);
    while (1) { }
}