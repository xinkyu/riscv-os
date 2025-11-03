// lab5/kernel/defs.h

// console.c
void cons_putc(char c);

// printf.c
void printf(const char *fmt, ...);
void clear_screen();

// uart.c
void uart_putc(char c);

// kalloc.c
void kinit();
void freerange(void *pa_start, void *pa_end);
void kfree(void *pa);
void *kalloc(void);

// vm.c
void kvminit(void);
void kvminithart(void);

// trap.c
void trap_init(void);
void clock_init(void);

// --- Lab5 新增 ---

// 在C语言中，当一个头文件需要引用一个在其他地方
// 定义的结构体指针时，需要一个“前向声明”。
struct context;

// proc.c
void procinit(void);
void userinit(void);
void scheduler(void) __attribute__((noreturn));
void yield(void);
void sleep(void*);
void wakeup(void*);

// swtch.S
void swtch(struct context*, struct context*);