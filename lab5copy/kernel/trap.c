// kernel/trap.c (Lab6 修改版)
#include "defs.h"
#include "riscv.h"
#include "syscall.h"

extern void kernelvec();
extern void uservec();
extern void userret(uint64, uint64);

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

// 初始化用户陷阱处理
void usertrapinit(void) {
    printf("usertrapinit: setting up user traps\n");
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

// 用户态陷阱处理
void usertrap(void) {
    int which_dev = 0;
    struct proc *p = myproc();

    if((r_sstatus() & SSTATUS_SPP) != 0)
        printf("usertrap: not from user mode");

    // 设置内核陷阱向量
    w_stvec((uint64)kernelvec);

    p->trapframe->epc = r_sepc();
    
    uint64 scause = r_scause();
    if(scause == 8) {
        // 系统调用
        if(p->killed)
            exit(-1);

        p->trapframe->epc += 4;
        
        intr_on();
        
        syscall();
    } else if((which_dev = devintr()) != 0){
        // 设备中断
    } else {
        printf("usertrap: unexpected scause %p pid=%d\n", r_scause(), p->pid);
        printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
        p->killed = 1;
    }

    if(p->killed)
        exit(-1);

    if(which_dev == 2)
        yield();

    usertrapret();
}

// 返回用户态
void usertrapret(void) {
    struct proc *p = myproc();

    intr_off();

    // 设置用户陷阱向量 (简化版，直接用内核向量)
    w_stvec((uint64)kernelvec);

    p->trapframe->kernel_satp = r_satp();
    p->trapframe->kernel_sp = p->kstack + PGSIZE;
    p->trapframe->kernel_trap = (uint64)usertrap;
    p->trapframe->kernel_hartid = 0;

    unsigned long x = r_sstatus();
    x &= ~SSTATUS_SPP;
    x |= SSTATUS_SPIE;
    w_sstatus(x);

    w_sepc(p->trapframe->epc);

    uint64 satp = MAKE_SATP(p->pagetable);

    // 简化返回（不使用 trampoline）
    // 直接恢复寄存器并 sret
    uint64 fn = (uint64)userret;
    ((void (*)(uint64,uint64))fn)(satp, (uint64)p->trapframe);
}

// 检查是否是设备中断
int devintr() {
    uint64 scause = r_scause();

    if((scause & 0x8000000000000000L) &&
       (scause & 0xff) == 9){
        // 软件中断 - 忽略
        return 0;
    } else if(scause == 0x8000000000000005L){
        // 时钟中断
        total_interrupt_count++;
        tick_counter++;
        wakeup((void*)&tick_counter);
        sbi_set_timer(r_time() + 100000);
        return 2;
    } else {
        return 0;
    }
}

void kerneltrap() {
    uint64 scause = r_scause();

    if (scause & (1L << 63)) {
        uint64 cause = scause & 0x7FFFFFFFFFFFFFFF;
        
        if (cause == 5) {
            total_interrupt_count++;
            tick_counter++;
            wakeup((void*)&tick_counter);
            uint64 next_timer = r_time() + 100000;
            sbi_set_timer(next_timer);
        }
    } else {
        printf("kerneltrap: exception scause %p, sepc %p\n", scause, r_sepc());
        while (1);
    }
}