// kernel/sysfile.c
#include "defs.h"

// 简化的文件描述符（暂时只支持控制台输出）
#define FD_CONSOLE 1

uint64 sys_read(void) {
    int fd, n;
    uint64 p;
    
    if (argint(0, &fd) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
        return -1;
    
    // 暂不实现
    return -1;
}

uint64 sys_write(void) {
    int fd, n;
    uint64 p;
    
    if (argint(0, &fd) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
        return -1;
    
    if (fd != FD_CONSOLE)
        return -1;
    
    // 简单实现：直接输出到控制台
    struct proc *proc = myproc();
    char buf[128];
    int i;
    
    for (i = 0; i < n && i < sizeof(buf); i++) {
        if (copyin(proc->pagetable, &buf[i], p + i, 1) < 0)
            return -1;
        uart_putc(buf[i]);
    }
    
    return i;
}