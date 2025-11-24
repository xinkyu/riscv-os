// kernel/trap.c
#include "defs.h"
#include "riscv.h"
#include "syscall.h"
#include "proc.h" 

extern void kernelvec();
extern struct proc proc[NPROC]; 
extern void restore_trapframe(struct trapframe *tf);

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

void fork_ret() {
    struct proc *p = myproc();
    release(&p->lock); 
    restore_trapframe(p->trapframe);
}

struct stack_regs {
    uint64 ra; uint64 sp; uint64 gp; uint64 tp;
    uint64 t0; uint64 t1; uint64 t2;
    uint64 s0; uint64 s1;
    uint64 a0; uint64 a1; uint64 a2; uint64 a3; uint64 a4; uint64 a5; uint64 a6; uint64 a7;
    uint64 s2; uint64 s3; uint64 s4; uint64 s5; uint64 s6; uint64 s7; uint64 s8; uint64 s9; uint64 s10; uint64 s11;
    uint64 t3; uint64 t4; uint64 t5; uint64 t6;
};

void kerneltrap(uint64 sp_val) {
    uint64 scause = r_scause();
    uint64 sepc = r_sepc();
    uint64 sstatus = r_sstatus(); // [修复1] 保存入口时的 sstatus

    if (scause & (1L << 63)) {
        uint64 cause = scause & 0x7FFFFFFFFFFFFFFF;
        if (cause == 5) {
            total_interrupt_count++;
            tick_counter++;
            wakeup((void*)&tick_counter);
            uint64 next_timer = r_time() + 100000;
            sbi_set_timer(next_timer);
        }
    } 
    else if (scause == 3) {
        struct proc *p = myproc();
        if (p == 0) {
            printf("kerneltrap: FATAL - no process\n");
            while(1);
        }

        struct stack_regs *regs = (struct stack_regs *)sp_val;

        if (p->trapframe) {
            // 复制寄存器到 trapframe ... (省略未变动代码)
            p->trapframe->ra = regs->ra;
            p->trapframe->sp = regs->sp + 256; 
            p->trapframe->gp = regs->gp;
            p->trapframe->tp = regs->tp;
            p->trapframe->t0 = regs->t0;
            p->trapframe->t1 = regs->t1;
            p->trapframe->t2 = regs->t2;
            p->trapframe->s0 = regs->s0;
            p->trapframe->s1 = regs->s1;
            p->trapframe->a0 = regs->a0;
            p->trapframe->a1 = regs->a1;
            p->trapframe->a2 = regs->a2;
            p->trapframe->a3 = regs->a3;
            p->trapframe->a4 = regs->a4;
            p->trapframe->a5 = regs->a5;
            p->trapframe->a6 = regs->a6;
            p->trapframe->a7 = regs->a7;
            p->trapframe->s2 = regs->s2;
            p->trapframe->s3 = regs->s3;
            p->trapframe->s4 = regs->s4;
            p->trapframe->s5 = regs->s5;
            p->trapframe->s6 = regs->s6;
            p->trapframe->s7 = regs->s7;
            p->trapframe->s8 = regs->s8;
            p->trapframe->s9 = regs->s9;
            p->trapframe->s10 = regs->s10;
            p->trapframe->s11 = regs->s11;
            p->trapframe->t3 = regs->t3;
            p->trapframe->t4 = regs->t4;
            p->trapframe->t5 = regs->t5;
            p->trapframe->t6 = regs->t6;
            p->trapframe->epc = sepc;
        }

        // [修复2] 不要在这里写寄存器，因为 intr_on 后的中断会覆盖它们
        // w_sepc(sepc + 4); 
        
        if(p->trapframe) p->trapframe->epc += 4;

        intr_on(); // 开启中断，危险区开始
        syscall();
        intr_off(); // 关闭中断，危险区结束
        
        // [修复3] 恢复被嵌套中断破坏的 CSR 寄存器
        // 必须恢复 sstatus，否则 sret 可能会错误地返回到 User 模式导致 Page Fault
        w_sstatus(sstatus); 
        w_sepc(sepc + 4);   // 恢复正确的返回地址 (跳过 ebreak 指令)

        regs->a0 = p->trapframe->a0;
    } 
    else {
        printf("kerneltrap: exception scause %p, sepc %p, stval %p\n", scause, sepc, r_stval());
        while(1);
    }
}