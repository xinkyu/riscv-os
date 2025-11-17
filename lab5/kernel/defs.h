// kernel/defs.h (修正版)
#ifndef __DEFS_H__
#define __DEFS_H__

// 关键头文件，必须先包含
#include "riscv.h"
#include "proc.h"   // 包含 NPROC 和所有结构体定义

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
pagetable_t create_pagetable(void);
int map_page(pagetable_t pt, uint64 va, uint64 pa, int perm);
pte_t *walk_lookup(pagetable_t pt, uint64 va);

// trap.c
void trap_init(void);
void clock_init(void);
uint64 get_time(void);
uint64 get_interrupt_count(void);
uint64 get_ticks(void);
void* get_ticks_channel(void); // 修正: 添加新原型

// test.c
void assert(int condition);
void test_physical_memory(void);
void test_pagetable(void);
void test_virtual_memory(void);
void test_timer_interrupt(void);
void test_exception_handling(void);
void test_interrupt_overhead(void);
void run_all_tests(void);
void run_lab4_tests(void);
void run_lab5_tests(void);

// spinlock.c
void spinlock_init(struct spinlock *lk, char *name);
void acquire(struct spinlock *lk);
void release(struct spinlock *lk);
void push_off(void);
void pop_off(void);

// proc.c
// (所有原型已移至 proc.h)

// swtch.S
void swtch(struct context *old, struct context *new);

#endif // __DEFS_H__