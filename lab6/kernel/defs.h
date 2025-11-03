// kernel/defs.h

// *** 修正点：包含 riscv.h 来获取 uint64 等类型定义 ***
#include "riscv.h" 

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
extern pagetable_t kernel_pagetable; // Lab6: 暴露内核页表

// trap.c
void trap_init(void);
void clock_init(void);
extern void kerneltrap(); // Lab6: 暴露陷阱处理器

// --- Lab5 新增 ---

// 在C语言中，当一个头文件需要引用一个在其他地方
// 定义的结构体指针时，需要一个“前向声明”。
struct context;
struct trapframe; // Lab6 新增

// proc.c
void procinit(void);
void userinit(void);
void scheduler(void) __attribute__((noreturn));
void yield(void);
void sleep(void*);
void wakeup(void*);
struct proc* myproc(void); 

// swtch.S
void swtch(struct context*, struct context*);

// --- Lab6 新增 ---
// syscall.c
void syscall(void);
int argint(int n, int *ip);
int argaddr(int n, uint64 *ip);

// sysproc.c
int sys_getpid(void);
int sys_yield(void);
int sys_fork(void);
int sys_exit(void);