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

    // 初始化内核栈上的上下文 
    // 将 ra 指向 forkret, sp 指向内核栈顶
    p->context.ra = (uint64)forkret;
    p->context.sp = p->kstack + PGSIZE;

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
        // 必须在调度循环中启用中断
        w_sstatus(r_sstatus() | SSTATUS_SIE);

        // 简单的轮转调度 (Round-Robin) 
        for (p = proc; p < &proc[NPROC]; p++) {
            if (p->state == RUNNABLE) { 
                p->state = RUNNING; 
                c->proc = p; 
                
                // 切换上下文到进程 p
                swtch(&c->context, &p->context); 

                // 切换回来后，重置 c->proc
                c->proc = 0; 
            }
        }
    }
}

// 切换到调度器
void sched(void) {
    struct proc *p = myproc();
    if (p->state == RUNNING) {
        // 保存上下文并切换到调度器
        swtch(&p->context, &mycpu()->context);
    } else {
        printf("sched: process not running\n");
    }
}

// 进程主动让出CPU 
void yield(void) {
    struct proc *p = myproc();
    if (p) {
        p->state = RUNNABLE;
        sched();
    }
}

// forkret: allocproc 设置的返回地址
// forkret: allocproc 设置的返回地址
void forkret(void) {
    printf("initproc: starting... running in kernel mode.\n");
    
    // 调用 Lab5 测试套件
    run_lab5_tests();
    
    // 不会到达这里
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