// kernel/proc.h (Lab6 修改版)
#ifndef __PROC_H__
#define __PROC_H__

#include "spinlock.h"
#include "riscv.h"

#define NPROC 64
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))

enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// 陷阱帧 - 保存用户寄存器
struct trapframe {
    /*   0 */ uint64 kernel_satp;
    /*   8 */ uint64 kernel_sp;
    /*  16 */ uint64 kernel_trap;
    /*  24 */ uint64 epc;
    /*  32 */ uint64 kernel_hartid;
    /*  40 */ uint64 ra;
    /*  48 */ uint64 sp;
    /*  56 */ uint64 gp;
    /*  64 */ uint64 tp;
    /*  72 */ uint64 t0;
    /*  80 */ uint64 t1;
    /*  88 */ uint64 t2;
    /*  96 */ uint64 s0;
    /* 104 */ uint64 s1;
    /* 112 */ uint64 a0;
    /* 120 */ uint64 a1;
    /* 128 */ uint64 a2;
    /* 136 */ uint64 a3;
    /* 144 */ uint64 a4;
    /* 152 */ uint64 a5;
    /* 160 */ uint64 a6;
    /* 168 */ uint64 a7;
    /* 176 */ uint64 s2;
    /* 184 */ uint64 s3;
    /* 192 */ uint64 s4;
    /* 200 */ uint64 s5;
    /* 208 */ uint64 s6;
    /* 216 */ uint64 s7;
    /* 224 */ uint64 s8;
    /* 232 */ uint64 s9;
    /* 240 */ uint64 s10;
    /* 248 */ uint64 s11;
    /* 256 */ uint64 t3;
    /* 264 */ uint64 t4;
    /* 272 */ uint64 t5;
    /* 280 */ uint64 t6;
};

struct context {
    uint64 ra;
    uint64 sp;
    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
};

struct cpu {
    struct proc *proc;
    struct context context;
    int ncli;
    int intena;
};

struct proc {
    struct spinlock lock;
    enum procstate state;
    int pid;
    struct proc *parent;
    void *chan;
    int killed;
    int xstate;
    uint64 kstack;
    uint64 sz;                  // 用户内存大小
    pagetable_t pagetable;      // 用户页表
    struct trapframe *trapframe; // 用户陷阱帧
    struct context context;
    void (*entry)(void);
    char name[16];
    int is_user;                // 是否为用户进程
};

extern struct proc proc[NPROC];
extern struct cpu cpus[1];

struct proc* myproc(void);
struct cpu* mycpu(void);
void procinit(void);
void scheduler(void) __attribute__((noreturn));
void sched(void);
void yield(void);
void sleep(void *chan, struct spinlock *lk);
void wakeup(void *chan);
int create_process(void (*entry)(void));
int fork(void);
void exit(int status);
int wait(uint64);
void wait_process(int*);
pagetable_t proc_pagetable(struct proc *p);
void proc_freepagetable(pagetable_t pagetable, uint64 sz);
int userinit(void);

#endif // __PROC_H__