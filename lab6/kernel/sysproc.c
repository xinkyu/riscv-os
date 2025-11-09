// kernel/sysproc.c
#include "defs.h"
#include "proc.h"

// 声明 syscall.c 中的 argint/argaddr
int argint(int n, int *ip);
int argaddr(int n, uint64 *ip);

// (你将在这里添加 sys_fork, sys_exit, sys_write 等)

// 一个最简单的系统调用：获取当前进程ID
uint64 sys_getpid(void) {
    struct proc *p = myproc();
    if (p) {
        return p->pid;
    }
    return 0; // 或者返回 -1
}