// kernel/main.c

// 声明 uart.c 中的 uart_puts 函数，以便在这里调用
void uart_puts(const char *s);

// 内核的 C 语言入口函数
void kmain(void) {
    uart_puts("Hello OS\n");

    // 无限循环，让操作系统保持运行
    while (1);
}