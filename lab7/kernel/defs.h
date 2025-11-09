// kernel/defs.h

#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "buf.h"
#include "fs.h"
#include "file.h"

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

// proc.c
void procinit(void);
void userinit(void);
void scheduler(void) __attribute__((noreturn));
void yield(void);
void sleep(void*, struct spinlock*); // <-- 签名已更新
void wakeup(void*);

// swtch.S
void swtch(struct context*, struct context*);

// spinlock.c
void initspinlock(struct spinlock*, char*); // <-- 新增
void acquire(struct spinlock*);
void release(struct spinlock*);
int holding(struct spinlock*); // <-- 新增
void push_off(void);
void pop_off(void);

// sleeplock.c
void initsleeplock(struct sleeplock*, char*);
void acquiresleep(struct sleeplock*);
void releasesleep(struct sleeplock*);
int holdingsleep(struct sleeplock*);

// syscall.c
void syscall(struct trapframe *tf);
int argint(int n, int *ip);
int argaddr(int n, uint64 *ip);

// sysproc.c
uint64 sys_getpid(void);