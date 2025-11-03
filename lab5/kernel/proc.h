// kernel/proc.h

#ifndef __PROC_H__
#define __PROC_H__

#include "riscv.h"

// 定义进程状态
enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// 进程的调度上下文，在 swtch.S 中使用
struct context {
    uint64 ra;
    uint64 sp;

    // callee-saved 寄存器
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

// 每个CPU的状态
struct cpu {
    struct proc *proc;      // 当前在此CPU上运行的进程
    struct context context; // 调度器上下文
};

extern struct cpu cpus[1]; // 假设单核
#define mycpu() (&cpus[0])
#define myproc() (mycpu()->proc)

// 进程控制块 (PCB)
struct proc {
    pagetable_t pagetable; // 进程页表
    uint64 kstack;         // 内核栈地址
    enum procstate state;  // 进程状态
    int pid;               // 进程ID
    struct context context; // 进程的调度上下文
    
    // -- sleep/wakeup --
    void *chan;            // 如果非零, 表示正在睡眠
};

#define NPROC 64 // 最大进程数

#endif // __PROC_H__