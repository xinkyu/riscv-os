// kernel/proc.h 
#ifndef __PROC_H__
#define __PROC_H__

#include "spinlock.h" // For struct spinlock
#include "riscv.h"    // For pagetable_t, uint64


#define NPROC 64 // 最大进程数

//进程状态枚举
enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// 上下文切换, 保存寄存器状态
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


struct cpu {
    struct proc *proc;      // 当前运行的进程
    struct context context; // 调度器上下文
    int ncli;               // push_off 的嵌套层数
    int intena;             // push_off 前的中断状态
};

extern struct cpu cpus[1]; // 声明

//进程控制块 PCB
struct proc {
    struct spinlock lock;   // 保护进程结构的自旋锁
    enum procstate state;   // 进程状态
    int pid;                // 进程ID
    struct proc *parent;    // 父进程
    void *chan;             // 如果非0, 表示正在 sleep
    int killed;             // 如果非0, 表示被 killing
    int xstate;             // 退出状态 (供 wait 读取)
    uint64 kstack;          // 进程的内核栈
    pagetable_t pagetable;  // 用户页表
    struct context context; // 上下文
    void (*entry)(void);    // 内核线程的入口函数
    char name[16];          // 进程名 (用于调试)
};

extern struct proc proc[NPROC]; // 声明

// Function Prototypes
struct proc* myproc(void);
struct cpu* mycpu(void);
void procinit(void);
void scheduler(void) __attribute__((noreturn));
void sched(void);
void yield(void);
void sleep(void *chan, struct spinlock *lk);
void wakeup(void *chan);
int  create_process(void (*entry)(void));
void exit(int status);
int  wait(int *status);
void wait_process(int*); // 指南书测试使用的名称

#endif // __PROC_H__