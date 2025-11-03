// kernel/sysproc.c
#include "riscv.h"
#include "defs.h"
#include "proc.h"

// 返回当前进程的 PID
int sys_getpid(void)
{
    struct proc *p = myproc();
    if(p)
        return p->pid;
    return -1;
}

// 主动让出 CPU
int sys_yield(void)
{
    yield();
    return 0; // 成功
}

// --- 其他系统调用的存根 (Stub) ---
// 你需要逐步实现它们

int sys_fork(void)
{
    printf("sys_fork not implemented\n");
    return -1;
}

int sys_exit(void)
{
    printf("sys_exit not implemented\n");
    return -1;
}