// kernel/proc.h
#ifndef __PROC_H__
#define __PROC_H__

#include "spinlock.h"
#include "riscv.h"

#define NPROC 64

enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// 保存的寄存器，用于系统调用/中断处理
struct trapframe {
    /* 0 */ uint64 kernel_satp;   // kernel page table
    /* 8 */ uint64 kernel_sp;     // top of process's kernel stack
    /* 16 */ uint64 kernel_trap;  // usertrap()
    /* 24 */ uint64 epc;          // saved user program counter
    /* 32 */ uint64 kernel_hartid; // saved kernel tp
    /* 40 */ uint64 ra;
    /* 48 */ uint64 sp;
    /* 56 */ uint64 gp;
    /* 64 */ uint64 tp;
    /* 72 */ uint64 t0;
    /* 80 */ uint64 t1;
    /* 88 */ uint64 t2;
    /* 96 */ uint64 s0;
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

extern struct cpu cpus[1];

struct proc {
    struct spinlock lock;
    enum procstate state;
    int pid;
    struct proc *parent;
    void *chan;
    int killed;
    int xstate;
    
    uint64 kstack;               // 内核栈
    struct trapframe *trapframe; // 新增: 指向 trapframe 物理页
    pagetable_t pagetable;       // 用户页表
    struct context context;      // 进程上下文
    
    void (*entry)(void);
    char name[16];
    void *ofile[16];             // 打开文件表 (Lab 7预留)
};

extern struct proc proc[NPROC];

#endif // __PROC_H__