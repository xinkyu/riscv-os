// kernel/defs.h

// uart.c 中声明的函数
void uart_putc(char c);
void uart_puts(const char *s);

// printf.c 中声明的函数
void printf(const char *fmt, ...);
void clear_screen();