// lab3/kernel/defs.h
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
pagetable_t create_pagetable(void);  
int map_page(pagetable_t pt, uint64 va, uint64 pa, int perm);  
pte_t *walk_lookup(pagetable_t pt, uint64 va);  

// test.c
void assert(int condition);  
void test_physical_memory(void); 
void test_pagetable(void);  
void test_virtual_memory(void);  
void run_all_tests(void);  