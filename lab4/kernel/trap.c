// lab4/kernel/trap.c
#include "defs.h"
#include "riscv.h"

extern void kernelvec();

static volatile uint64 tick_counter = 0;
static volatile uint64 total_interrupt_count = 0;

// SBI 调用：设置下一次 timer 中断
// a7 = 0 (set_timer extension)
// a6 = 0 (set_timer function)
// a0 = 时间值
static inline void sbi_set_timer(uint64 stime) {
    register uint64 a7 asm("a7") = 0;  // EID: Timer Extension
    register uint64 a6 asm("a6") = 0;  // FID: set_timer
    register uint64 a0 asm("a0") = stime;
    asm volatile("ecall" : "+r"(a0) : "r"(a6), "r"(a7) : "memory");
}

void trap_init(void) {
    w_stvec((uint64)kernelvec);
    printf("trap_init: stvec set to %p\n", kernelvec);
}

void clock_init(void) {
    // 设置第一次 timer 中断（当前时间 + 一个时间间隔）
    // 这会清除待处理的中断
    uint64 next_timer = r_time() + 100000;  // 100000 cycles later
    sbi_set_timer(next_timer);
    
    // 然后启用 timer 中断
    w_sie(r_sie() | SIE_STIE);
}

uint64 get_time(void) {
    return r_time();
}

uint64 get_interrupt_count(void) {
    return total_interrupt_count;
}

void kerneltrap() {
    uint64 scause = r_scause();

    if (scause & (1L << 63)) {
        // 这是中断
        uint64 cause = scause & 0x7FFFFFFFFFFFFFFF;
        
        if (cause == 5) {
            // 时钟中断
            total_interrupt_count++;
            tick_counter++;
            
            // 设置下一次中断（这会自动清除 STIP）
            uint64 next_timer = r_time() + 100000;
            sbi_set_timer(next_timer);
        }
    } else {
        // 这是异常 - 死循环
        while (1);
    }
}