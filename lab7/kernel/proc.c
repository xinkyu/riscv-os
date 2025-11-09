// kernel/proc.c
#include "defs.h"
#include "riscv.h"
#include "proc.h"
#include "spinlock.h"  // <-- 确保包含
#include "sleeplock.h" // <-- 确保包含

struct proc proc[NPROC];
struct cpu cpus[1];

struct proc *initproc;

static int nextpid = 1;
struct spinlock pid_lock; // <-- 全局 PID 锁

extern void forkret(void);
void swtch(struct context *old, struct context *new);

// 初始化进程表 
void procinit(void) {
    initspinlock(&pid_lock, "nextpid"); // <-- 初始化 PID 锁
    for (int i = 0; i < NPROC; i++) {
        initspinlock(&proc[i].lock, "proc"); // <-- 初始化每个进程的锁
        proc[i].state = UNUSED;
    }
    printf("procinit: process table initialized.\n");
}

// 分配一个新进程 
static struct proc* allocproc(void) {
    struct proc *p;

    acquire(&pid_lock); // <-- 锁住 PID
    int pid = nextpid++;
    release(&pid_lock); // <-- 释放 PID 锁

    for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock); // <-- 锁住这个 proc
        if (p->state == UNUSED) {
            p->pid = pid; 
            p->state = USED;
            // (保持锁住，直到 userinit 设置为 RUNNABLE)
            goto found;
        }
        release(&p->lock); // <-- 解锁
    }
    return 0; // 没有空闲进程

found:
    if ((p->kstack = (uint64)kalloc()) == 0) {
        p->state = UNUSED;
        release(&p->lock); // <-- 失败时解锁
        return 0;
    }

    p->trapframe = (struct trapframe *)(p->kstack + PGSIZE - sizeof(struct trapframe));
    p->context.ra = (uint64)forkret;
    p->context.sp = (uint64)p->trapframe; 

    return p; // (返回时 p->lock 仍被持有)
}

// (freeproc 暂时跳过，它也需要锁)

// 创建第一个进程 (initproc)
void userinit(void) {
    struct proc *p = allocproc(); // (allocproc 返回时 p->lock 是锁住的)
    if (p == 0) {
        printf("userinit: failed to alloc proc\n");
        return;
    }
    
    initproc = p;
    p->state = RUNNABLE; // 标记为可运行 
    release(&p->lock); // <-- *现在*才解锁

    printf("userinit: first process (initproc) created. PID: %d\n", p->pid);
}

// 调度器循环 
void scheduler(void) {
    struct proc *p;
    struct cpu *c = mycpu();
    
    c->proc = 0;
    printf("scheduler: starting scheduler loop...\n");
    
    for (;;) { 
        // 关中断 (在 lab6 我们已经关了)
        intr_off();

        for (p = proc; p < &proc[NPROC]; p++) {
            acquire(&p->lock); // <-- 锁住 p
            if (p->state == RUNNABLE) { 
                p->state = RUNNING; 
                c->proc = p; 

                // 切换！
                // swtch 会保存 c->context (中断是关的)
                // 切换到 p->context (p->lock 是持有的)
                swtch(&c->context, &p->context); 

                // 切换回来后 (c->proc 已经被 yield 设为 0)
                c->proc = 0; 
            }
            release(&p->lock); // <-- 解锁 p
        }
        
        // 没有可运行的进程，开中断并等待
        intr_on();
    }
}

// 切换到调度器
// *必须* 在持有 p->lock 的情况下调用
void sched(void) {
    struct proc *p = myproc();
    struct cpu *c = mycpu();
    if (!p) return;
    if (p->state == RUNNING) {
        printf("sched: state RUNNING\n");
        while(1); // panic
    }
    if (holding(&p->lock) == 0) { // 检查我们是否持有锁
        printf("sched: no lock\n");
        while(1); // panic
    }

    // swtch 会保存 p->context, 切换到 c->context
    // (我们保持 p->lock 锁住状态)
    swtch(&p->context, &c->context);
    
    // (我们不会立即返回到这里)
}

// 进程主动让出CPU 
void yield(void) {
    struct proc *p = myproc();
    if (p) {
        acquire(&p->lock); // <-- 获取锁
        p->state = RUNNABLE;
        sched();
        release(&p->lock); // <-- 释放锁
    }
}

// 进程睡眠
// lk 是 sleeplock 的内部 spinlock
void sleep(void *chan, struct spinlock *lk) {
    struct proc *p = myproc();
    if(p == 0)
        return;

    // acquire p->lock
    acquire(&p->lock);

    // *原子性地* 释放 lk 并设置状态
    release(lk); 

    p->chan = chan;
    p->state = SLEEPING; 

    // 去调度
    sched(); // sched 会保持 p->lock 锁住状态并切换

    // 唤醒后
    p->chan = 0;

    // 释放 p->lock
    release(&p->lock);
    
    // 重新获取 sleeplock 的 lk
    acquire(lk);
}

// 唤醒在 chan 上睡眠的所有进程
void wakeup(void *chan) {
    struct proc *p;
    for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock); // <-- 锁住 p
        if (p->state == SLEEPING && p->chan == chan) {
            p->state = RUNNABLE;
        }
        release(&p->lock); // <-- 解锁 p
    }
}

// forkret: allocproc 设置的返回地址
void forkret(void) {
    // 第一次运行：swtch 返回到这里时，
    // 我们仍然持有 p->lock (由 scheduler 获取的)
    release(&myproc()->lock); // <-- 释放它

    // --- 实验六 验证代码 ---
    uint64 pid = sys_getpid();
    printf("initproc: [Lab6 Test] sys_getpid() reports my pid is %d\n", pid);
    if(pid != 1) {
        printf("initproc: [Lab6 Test] FAILED! Expected pid 1\n");
    } else {
        printf("initproc: [Lab6 Test] SUCCESS! sys_getpid is linked.\n");
    }
    // --- 验证结束 ---

    // 进程第一次运行时，在这里开启中断
    intr_on();
    
    while(1) {
        yield();
    }
}