// kernel/defs.h (Lab6 修改版)
#ifndef __DEFS_H__
#define __DEFS_H__

#include "riscv.h"
#include "proc.h"

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
int mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm);
void uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free);
int uvmcopy(pagetable_t old, pagetable_t new, uint64 sz);
uint64 uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz);
int copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len);
int copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len);
int copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max);

// trap.c
void trap_init(void);
void usertrapinit(void);
void usertrap(void);
void usertrapret(void);
void clock_init(void);
uint64 get_time(void);
uint64 get_interrupt_count(void);
uint64 get_ticks(void);
void* get_ticks_channel(void);

// syscall.c
void syscall(void);
int argint(int n, int *ip);
int argaddr(int n, uint64 *ip);
int argstr(int n, char *buf, int max);
int fetchstr(uint64 addr, char *buf, int max);

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
void run_lab6_tests(void);

// spinlock.c
void spinlock_init(struct spinlock *lk, char *name);
void acquire(struct spinlock *lk);
void release(struct spinlock *lk);
void push_off(void);
void pop_off(void);

// swtch.S
void swtch(struct context *old, struct context *new);

// trampoline.S
void userret(uint64, uint64);

// string.c
void* memset(void*, int, uint64);
int strlen(const char*);

#endif // __DEFS_H__