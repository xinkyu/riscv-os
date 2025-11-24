// kernel/trap.c
#include "defs.h"
#include "riscv.h"
#include "syscall.h"
#include "proc.h" // 必须包含，以便访问 proc 数组

extern void kernelvec();
extern struct proc proc[NPROC]; // 引用全局进程表

static volatile uint64 tick_counter = 0;
static volatile uint64 total_interrupt_count = 0;

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

uint64 get_time(void) { return r_time(); }
uint64 get_interrupt_count(void) { return total_interrupt_count; }
uint64 get_ticks(void) { return tick_counter; }
void* get_ticks_channel(void) { return (void*)&tick_counter; }

void kerneltrap() {
    uint64 scause = r_scause();
    uint64 sepc = r_sepc();

    // 处理中断
    if (scause & (1L << 63)) {
        uint64 cause = scause & 0x7FFFFFFFFFFFFFFF;
        if (cause == 5) { // Timer
            total_interrupt_count++;
            tick_counter++;
            wakeup((void*)&tick_counter);
            uint64 next_timer = r_time() + 100000;
            sbi_set_timer(next_timer);
        }
    } 
    // 处理系统调用 (ecall)
    else if (scause == 8 || scause == 9) {
        struct proc *p = myproc();
        
        // [修复] 如果 myproc() 丢失了进程，手动找回它！
        if (p == 0) {
            for(int i = 0; i < NPROC; i++) {
                if (proc[i].state == RUNNING) {
                    p = &proc[i];
                    // 顺便修复 CPU 记录，防止下次再丢
                    mycpu()->proc = p; 
                    break;
                }
            }
        }

        // 必须跳过 ecall 指令
        w_sepc(sepc + 4);

        if (p == 0) {
            printf("kerneltrap: FATAL - ecall from unknown process. No RUNNING proc found.\n");
            while(1);
        }

        // 保存 EPC，以备不时之需
        if (p->trapframe) {
            p->trapframe->epc = sepc + 4;
        } else {
             printf("kerneltrap: FATAL - proc %d has NULL trapframe\n", p->pid);
             while(1);
        }

        intr_on();
        syscall();
        
        if (p->killed) exit(-1);
    } 
    else {
        printf("kerneltrap: exception scause %p, sepc %p\n", scause, sepc);
        printf("stval: %p\n", r_stval());
        struct proc *p = myproc();
        printf("current process: %s (pid %d)\n", p ? p->name : "NULL", p ? p->pid : -1);
        while (1);
    }
}