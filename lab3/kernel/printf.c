// kernel/printf.c
#include <stdarg.h> // 引入标准库头文件，用于处理可变参数
#include "defs.h"   // 引入我们自己的函数声明

// 静态辅助函数，用于打印不同进制的数字。'static' 表示这个函数只能在本文件内被调用。
static void print_int(long long xx, int base, int sign) {
    char digits[] = "0123456789abcdef"; // 用于进制转换的字符映射表
    char buf[32]; // 缓冲区，用于存放转换后的数字字符串（倒序）
    int i = 0;
    unsigned long long x;

    // 如果是带符号的十进制数且为负数
    if (sign && xx < 0) {
        cons_putc('-'); // 先输出一个负号
        x = -xx;        // 然后转为正数处理
    } else {
        x = xx;
    }

    // 核心的进制转换逻辑：通过循环取模和除法，得到每一位的数字
    do {
        buf[i++] = digits[x % base];
    } while ((x /= base) != 0);

    // 因为上面得到的是倒序的，所以这里需要反向输出
    while (--i >= 0) {
        cons_putc(buf[i]);
    }
}

// 内核的 printf 函数本体
// '...' 表示这个函数可以接受不定数量的参数
void printf(const char *fmt, ...) {
    va_list args; // 定义一个 va_list 类型的变量，用于访问可变参数
    char *s;
    int c;

    if (fmt == 0) { // 如果格式化字符串是空指针，直接返回
        return;
    }

    va_start(args, fmt); // 初始化 args，使其指向第一个可变参数
    // 遍历格式化字符串
    for (c = *fmt; c != '\0'; c = *++fmt) {
        if (c != '%') { // 如果不是格式化字符 '%'
            cons_putc(c); // 直接输出该字符
            continue;
        }

        // 如果是 '%'，则检查下一个字符来确定格式
        c = *++fmt;
        if (c == '\0') break; // 如果 '%' 是最后一个字符，则中断

        switch (c) {
            case 'd': // 十进制整数
                print_int(va_arg(args, int), 10, 1);
                break;
            case 'x': // 十六进制整数
                print_int(va_arg(args, int), 16, 0);
                break;
            case 's': // 字符串
                s = va_arg(args, char *);
                if (s == 0) { // 对空指针做特殊处理
                    s = "(null)";
                }
                while (*s != '\0') {
                    cons_putc(*s++);
                }
                break;
            case '%': // 输出一个 '%'
                cons_putc('%');
                break;
                // 在 printf 函数的 switch 语句中添加:
            case 'p': // 指针（十六进制）
                cons_putc('0');
                cons_putc('x');
                print_int(va_arg(args, unsigned long long), 16, 0);
                break;
            default: // 不认识的格式，原样输出
                cons_putc('%');
                cons_putc(c);
                break;
        }
    }
    va_end(args); // 清理可变参数列表
}

// 清屏函数
void clear_screen() {
    // "\033[2J" 和 "\033[H" 是 ANSI 转义序列
    // "\033[2J": 清除整个屏幕
    // "\033[H": 将光标移动到左上角 (第一行第一列)
    // 现代终端大多支持这种标准
    const char *seq = "\033[2J\033[H";
    while (*seq) {
        cons_putc(*seq++);
    }
}