// kernel/syscall.c
#include "defs.h"
#include "syscall.h"

// 系统调用函数声明
extern uint64 sys_fork(void);
extern uint64 sys_exit(void);
extern uint64 sys_wait(void);
extern uint64 sys_getpid(void);
extern uint64 sys_read(void);
extern uint64 sys_write(void);
extern uint64 sys_sleep(void);
extern uint64 sys_uptime(void);

// 系统调用表
static uint64 (*syscalls[])(void) = {
    [SYS_fork]    sys_fork,
    [SYS_exit]    sys_exit,
    [SYS_wait]    sys_wait,
    [SYS_getpid]  sys_getpid,
    [SYS_read]    sys_read,
    [SYS_write]   sys_write,
    [SYS_sleep]   sys_sleep,
    [SYS_uptime]  sys_uptime,
};

// 系统调用名称（用于调试）
static char *syscall_names[] = {
    [SYS_fork]    "fork",
    [SYS_exit]    "exit",
    [SYS_wait]    "wait",
    [SYS_getpid]  "getpid",
    [SYS_read]    "read",
    [SYS_write]   "write",
    [SYS_sleep]   "sleep",
    [SYS_uptime]  "uptime",
};

// 从用户空间获取第n个系统调用参数（整数）
int argint(int n, int *ip) {
    struct proc *p = myproc();
    if (n < 0 || n >= 6) return -1;
    
    switch(n) {
        case 0: *ip = p->trapframe->a0; break;
        case 1: *ip = p->trapframe->a1; break;
        case 2: *ip = p->trapframe->a2; break;
        case 3: *ip = p->trapframe->a3; break;
        case 4: *ip = p->trapframe->a4; break;
        case 5: *ip = p->trapframe->a5; break;
    }
    return 0;
}

// 从用户空间获取第n个系统调用参数（地址）
int argaddr(int n, uint64 *ip) {
    struct proc *p = myproc();
    if (n < 0 || n >= 6) return -1;
    
    switch(n) {
        case 0: *ip = p->trapframe->a0; break;
        case 1: *ip = p->trapframe->a1; break;
        case 2: *ip = p->trapframe->a2; break;
        case 3: *ip = p->trapframe->a3; break;
        case 4: *ip = p->trapframe->a4; break;
        case 5: *ip = p->trapframe->a5; break;
    }
    return 0;
}

// 从用户空间获取字符串参数
int argstr(int n, char *buf, int max) {
    uint64 addr;
    if (argaddr(n, &addr) < 0)
        return -1;
    return fetchstr(addr, buf, max);
}

// 从用户空间复制字符串到内核
int fetchstr(uint64 addr, char *buf, int max) {
    struct proc *p = myproc();
    int i;
    
    for (i = 0; i < max; i++) {
        if (copyin(p->pagetable, &buf[i], addr + i, 1) < 0)
            return -1;
        if (buf[i] == '\0')
            return i;
    }
    return -1;
}

// 系统调用分发器
void syscall(void) {
    int num;
    struct proc *p = myproc();
    
    num = p->trapframe->a7;  // 系统调用号在 a7 寄存器
    
    if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {
        // 调用系统调用函数，返回值存入 a0
        p->trapframe->a0 = syscalls[num]();
    } else {
        printf("%d %s: unknown sys call %d\n",
                p->pid, p->name, num);
        p->trapframe->a0 = -1;
    }
}