// kernel/proc.c
#include "defs.h"
#include "riscv.h"
#include "proc.h"

struct proc proc[NPROC];
struct cpu cpus[1];

struct proc *initproc; // 第一个进程

static int nextpid = 1; // 用于分配PID 

// 声明 swtch.S 中的函数
void swtch(struct context *old, struct context *new);

// 声明一个辅助函数，用于 allocproc
void forkret(void);

// 初始化进程表 
void procinit(void) {
    for (int i = 0; i < NPROC; i++) {
        proc[i].state = UNUSED;
    }
    printf("procinit: process table initialized.\n");
}

// 分配一个新进程 
static struct proc* allocproc(void) {
    struct proc *p;

    for (p = proc; p < &proc[NPROC]; p++) {
        if (p->state == UNUSED) {
            goto found;
        }
    }
    return 0; // 没有空闲进程

found:
    p->pid = nextpid++; 
    p->state = USED;

    // 分配内核栈 
    if ((p->kstack = (uint64)kalloc()) == 0) {
        p->state = UNUSED;
        return 0;
    }

    // --- 实验六修改 ---
    // 将 trapframe 指向内核栈的顶部
    // (trapframe 将在 trap 时被放置在这里)
    p->trapframe = (struct trapframe *)(p->kstack + PGSIZE - sizeof(struct trapframe));

    // 初始化内核栈上的上下文 
    // 将 ra 指向 forkret, sp 指向内核栈顶
    p->context.ra = (uint64)forkret;
    // context 保存区域在 trapframe 下方
    p->context.sp = (uint64)p->trapframe; 

    return p;
}

// 释放进程资源（暂未实现完整，实验五只关注调度）
void freeproc(struct proc *p) {
    if (p->kstack)
        kfree((void*)p->kstack);
    p->state = UNUSED;
    p->chan = 0;
    p->pid = 0;
}

// 创建第一个进程 (initproc)
// 它在内核态运行，没有用户空间
void userinit(void) {
    struct proc *p = allocproc();
    if (p == 0) {
        printf("userinit: failed to alloc proc\n");
        return;
    }
    
    initproc = p;
    p->state = RUNNABLE; // 标记为可运行 

    printf("userinit: first process (initproc) created. PID: %d\n", p->pid);
}

// 调度器循环 
void scheduler(void) {
    struct proc *p;
    struct cpu *c = mycpu();
    
    c->proc = 0;
    printf("scheduler: starting scheduler loop...\n");
    
    for (;;) { 
        // --- 实验六/实验五 关键修改 ---
        // 必须在调度循环中 *关闭* 中断
        // intr_off(); // 如果你定义了
        w_sstatus(r_sstatus() & ~SSTATUS_SIE); // 手动关中断

        // 简单的轮转调度 (Round-Robin) 
        for (p = proc; p < &proc[NPROC]; p++) {
            if (p->state == RUNNABLE) { 
                p->state = RUNNING; 
                c->proc = p; 

                // --- 修改点 ---
                // 在切换到新进程 *之前* 重新开启中断
                // w_sstatus(r_sstatus() | SSTATUS_SIE);

                // 切换上下文到进程 p
                // swtch 会保存 c->context (此时中断是关的)
                // 并恢复 p->context (它恢复时的 sstatus 也是关的)
                swtch(&c->context, &p->context); 

                // 切换回来后 (从 sched() 返回)
                // c->proc 会被 yield() 中的 sched() 设置为 0
                c->proc = 0; 
            }
        }
        
        // 如果没有 RUNNABLE 的进程，打开中断并等待
        w_sstatus(r_sstatus() | SSTATUS_SIE);
        // asm volatile("wfi"); // (可选的)
    }
}

// 切换到调度器
void sched(void) {
    struct proc *p = myproc();

    // sched 必须在有当前进程的上下文中被调用
    if (!p) {
        printf("sched: no process\n");
        return;
    }
    
    // (我们甚至可以检查 p->state != RUNNABLE 和 p->state != SLEEPING
    //  来确保状态被正确设置了，但 lab5 的逻辑中，我们
    //  只需要确保不是 RUNNING 状态即可。
    //  但最简单的修复是：无条件切换)

    // 保存上下文并切换到调度器
    // 调用者 (yield 或 sleep) 已经设置好了进程状态
    swtch(&p->context, &mycpu()->context);
}

// 进程主动让出CPU 
void yield(void) {
    struct proc *p = myproc();
    if (p) {
        p->state = RUNNABLE;
        sched();
    }
}
// 声明 sys_getpid (它在 sysproc.c 中，但通过 defs.h 暴露)
extern uint64 sys_getpid(void);
// forkret: allocproc 设置的返回地址
void forkret(void) {
    // 这是一个占位符。在真正的 fork 实现中，这里会
    // 负责返回到用户空间。在实验五中，第一个进程
    // (initproc) 会进入这里，然后可以让他循环 yield。
    printf("initproc: starting... running in kernel mode.\n");
    // --- 实验六 验证代码 ---
    // 作为一个内核进程，我们直接调用内核函数
    uint64 pid = sys_getpid();
    printf("initproc: [Lab6 Test] sys_getpid() reports my pid is %d\n", pid);
    if(pid != 1) {
        printf("initproc: [Lab6 Test] FAILED! Expected pid 1\n");
    } else {
        printf("initproc: [Lab6 Test] SUCCESS! sys_getpid is linked.\n");
    }
    // --- 验证结束 ---
    w_sstatus(r_sstatus() | SSTATUS_SIE);
    while(1) {
        yield();
    }
}

// 进程睡眠 
void sleep(void *chan) {
    struct proc *p = myproc();
    if(p) {
        p->chan = chan;
        p->state = SLEEPING; 
        sched();
        // 被唤醒后
        p->chan = 0;
    }
}

// 唤醒在 chan 上睡眠的所有进程
void wakeup(void *chan) {
    struct proc *p;
    for (p = proc; p < &proc[NPROC]; p++) {
        if (p->state == SLEEPING && p->chan == chan) {
            p->state = RUNNABLE;
        }
    }
}