// lab4/kernel/defs.h

#ifndef __DEFS_H__
#define __DEFS_H__

#include "riscv.h"  // 提供 pagetable_t, pte_t, uint64 等类型定义

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
void set_print_ticks(int enable);  // 新增
void print_interrupt_stats(void);  // 新增
// test.c
void assert(int condition);
void test_physical_memory(void);
void test_pagetable(void);
void test_virtual_memory(void);
void test_timer_interrupt(void);
void test_exception_handling(void);
void test_interrupt_overhead(void);
void run_all_tests(void);
void run_lab4_tests(void);  // 添加这一行

#endif // __DEFS_H__