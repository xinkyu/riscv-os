// kernel/proc.h

#ifndef __PROC_H__
#define __PROC_H__

#include "riscv.h"
#include "spinlock.h"
// 匹配 kernelvec.S 中寄存器保存顺序的陷阱帧
struct trapframe {
    uint64 ra;   // 0
    uint64 sp;   // 8
    uint64 gp;   // 16
    uint64 tp;   // 24
    uint64 t0;   // 32
    uint64 t1;   // 40
    uint64 t2;   // 48
    uint64 s0;   // 56
    uint64 s1;   // 64
    uint64 a0;   // 72
    uint64 a1;   // 80
    uint64 a2;   // 88
    uint64 a3;   // 96
    uint64 a4;   // 104
    uint64 a5;   // 112
    uint64 a6;   // 120
    uint64 a7;   // 128  <-- 系统调用号将在这里
    uint64 s2;   // 136
    uint64 s3;   // 144
    uint64 s4;   // 152
    uint64 s5;   // 160
    uint64 s6;   // 168
    uint64 s7;   // 176
    uint64 s8;   // 184
    uint64 s9;   // 192
    uint64 s10;  // 200
    uint64 s11;  // 208
    uint64 t3;   // 216
    uint64 t4;   // 224
    uint64 t5;   // 232
    uint64 t6;   // 240
    // sepc 和 sstatus 由 kerneltrap 手动保存
    uint64 sepc;
    uint64 sstatus;
};

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
    struct spinlock lock;

    pagetable_t pagetable; // 进程页表
    uint64 kstack;         // 内核栈地址
    enum procstate state;  // 进程状态
    int pid;               // 进程ID
    struct context context; // 进程的调度上下文
    
    // --- 实验六新增 ---
    struct trapframe *trapframe; // 指向保存在内核栈上的陷阱帧

    // -- sleep/wakeup --
    void *chan;            // 如果非零, 表示正在睡眠
};

#define NPROC 64 // 最大进程数

#endif // __PROC_H__