// lab4/kernel/trap.c
#include "defs.h"
#include "riscv.h"
#include "proc.h" // 引入 proc.h


// 声明 syscall.c 中即将创建的函数
void syscall(struct trapframe *tf);

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
// --- 实验六修改：修改函数签名 ---
void kerneltrap(struct trapframe *tf) {
    uint64 scause = r_scause();
    
    // S-mode 陷阱总是会禁用中断，
    // 我们需要保存 sstatus 并在返回前恢复
    tf->sstatus = r_sstatus();
    tf->sepc = r_sepc();

    // 判断是中断还是异常
    if (scause & (1L << 63)) { // 异步中断
        uint64 interrupt_type = scause & 0x7FFFFFFFFFFFFFFF;
        
        if (interrupt_type == 5) { // S-mode 时钟中断
            if (++tick_counter % 100 == 0) {
                printf("tick: %d\n", tick_counter);
            }
            yield();
            asm volatile("csrc sip, %0" : : "r"(1L << 5));
        } else {
            printf("kerneltrap: unhandled interrupt type %d\n", interrupt_type);
        }
    } else { // 同步异常
        
        // --- 实验六新增：系统调用处理 ---
        if (scause == 8) { // 来自 U-mode 的 Ecall (系统调用)
            // 检查 sstatus 的 SPP 位，确保是从用户态来的
            if ((tf->sstatus & (1L << 8)) == 0) {
                printf("kerneltrap: ecall from S-mode?\n");
                goto panic;
            }

            // ecall 指令会将 pc + 4 保存到 sepc
            // 我们需要手动将 sepc 再次 + 4，
            // 这样 sret 返回时会执行 ecall 的 *下一条* 指令
            tf->sepc += 4;
            
            // 调用系统调用分发器
            syscall(tf);

        } else {
        panic:
            printf("kerneltrap: supervisor exception.\n");
            printf("  scause: 0x%x\n", scause);
            printf("  sepc:   0x%x (pid: %d)\n", tf->sepc, myproc() ? myproc()->pid : -1);
            while (1);
        }
    }

    // 恢复 sepc 和 sstatus
    w_sepc(tf->sepc);
    w_sstatus(tf->sstatus);
}