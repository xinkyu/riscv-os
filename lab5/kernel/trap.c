// lab4/kernel/trap.c
#include "defs.h"
#include "riscv.h"
#include "proc.h" // 引入 proc.h

// 在 kernelvec.S 中定义的汇编入口点
extern void kernelvec();

static uint64 tick_counter = 0;

// 初始化陷阱处理
void trap_init(void) {
    // 将 Supervisor 模式的陷阱处理程序地址设置为 kernelvec
    w_stvec((uint64)kernelvec);
    printf("trap_init: stvec set to %p\n", kernelvec);
}

// 时钟中断初始化
void clock_init(void) {
    // 开启 Supervisor 模式下的时钟中断
    w_sie(r_sie() | SIE_STIE);
    printf("clock_init: supervisor timer interrupt enabled.\n");
}

// 内核态的陷阱处理函数，由 kernelvec.S 调用
void kerneltrap() {
    uint64 scause = r_scause();
    uint64 sepc = r_sepc();

    // 判断是中断还是异常
    if (scause & (1L << 63)) { // 最高位为1，表示是中断
        uint64 interrupt_type = scause & 0x7FFFFFFFFFFFFFFF;
        
        // 我们只处理 Supervisor 模式的时钟中断
        if (interrupt_type == 5) {
            // 每100次中断打印一次信息
            if (++tick_counter % 100 == 0) {
                printf("tick: %d\n", tick_counter);
            }
            
            yield();
            // RISC-V M-mode OpenSBI 会自动为我们设置下一次时钟中断,
            // 所以我们不需要手动操作 mtimecmp。
            // 只需清除 sip 寄存器中的 STIP 位即可。
            asm volatile("csrc sip, %0" : : "r"(1L << 5));

        } else {
            printf("kerneltrap: unhandled supervisor interrupt type %d\n", interrupt_type);
        }
    } else { // 否则是异常
        printf("kerneltrap: supervisor exception.\n");
        printf("  scause: 0x%x\n", scause);
        printf("  sepc:   0x%x\n", sepc);
        
        // 异常发生，内核宕机
        while (1);
    }
}