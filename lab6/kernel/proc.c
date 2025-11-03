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
        goto fail;
    }

    // --- Lab6 新增：分配陷阱帧 ---
    if ((p->trapframe = (struct trapframe *)kalloc()) == 0) {
        goto fail;
    }
    
    // --- Lab6 新增：填充陷阱帧的内核信息 ---
    p->trapframe->kernel_sp = p->kstack + PGSIZE;
    p->trapframe->kernel_trap = (uint64)kerneltrap;
    p->trapframe->kernel_satp = MAKE_SATP(kernel_pagetable);


    // 初始化内核栈上的上下文 
    // 将 ra 指向 forkret, sp 指向内核栈顶
    p->context.ra = (uint64)forkret;
    p->context.sp = p->kstack + PGSIZE;

    return p;

fail:
    // 释放已分配的资源
    if (p->trapframe) {
        kfree((void*)p->trapframe);
        p->trapframe = 0;
    }
    if (p->kstack) {
        kfree((void*)p->kstack);
        p->kstack = 0;
    }
    p->state = UNUSED;
    return 0;
}

// 释放进程资源
void freeproc(struct proc *p) {
    // --- Lab6 新增：释放陷阱帧 ---
    if (p->trapframe) {
        kfree((void*)p->trapframe);
        p->trapframe = 0;
    }

    if (p->kstack)
        kfree((void*)p->kstack);
    
    // TODO: 释放用户页表
    
    p->kstack = 0;
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
    
    // 它是一个纯内核线程
    
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
void forkret(void) {
    // initproc (内核进程) 会进入这里
    printf("initproc: starting... running in kernel mode.\n");
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