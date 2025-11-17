// kernel/trap.c (最终修正版 - 修复 unused variable)
#include "defs.h"
#include "riscv.h"

extern void kernelvec();

static volatile uint64 tick_counter = 0;
static volatile uint64 total_interrupt_count = 0;

// SBI 调用：设置下一次 timer 中断
static inline void sbi_set_timer(uint64 stime) {
    register uint64 a7 asm("a7") = 0;
    register uint64 a6 asm("a6") = 0;
    register uint64 a0 asm("a0") = stime;
    asm volatile("ecall" : "+r"(a0) : "r"(a6), "r"(a7) : "memory");
}

void trap_init(void) {
    w_stvec((uint64)kernelvec);
    printf("trap_init: stvec set to %p\n", kernelvec);
}

void clock_init(void) {
    uint64 next_timer = r_time() + 100000;
    sbi_set_timer(next_timer);
    w_sie(r_sie() | SIE_STIE);
}

uint64 get_time(void) {
    return r_time();
}

uint64 get_interrupt_count(void) {
    return total_interrupt_count;
}

uint64 get_ticks(void) {
    return tick_counter;
}

void* get_ticks_channel(void) {
    return (void*)&tick_counter;
}

void kerneltrap() {
    uint64 scause = r_scause();
    
    // 修正: 既然 'yield()' 被移除了, 'p' 也不再需要.
    // struct proc *p = myproc(); // <--- 注释掉这一行

    if (scause & (1L << 63)) {
        // 这是中断
        uint64 cause = scause & 0x7FFFFFFFFFFFFFFF;
        
        if (cause == 5) {
            // 时钟中断
            total_interrupt_count++;
            tick_counter++;

            // 唤醒睡眠在 'tick_counter' 地址上的进程
            wakeup((void*)&tick_counter);
            
            uint64 next_timer = r_time() + 100000;
            sbi_set_timer(next_timer);

            // 移除 yield() 以防止栈溢出
            /*
            if (p && p->state == RUNNING) {
                yield();
            }
            */
        }
    } else {
        // 这是异常
        printf("kerneltrap: exception scause %p, sepc %p\n", scause, r_sepc());
        while (1);
    }
}