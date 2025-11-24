// kernel/syscall.c
#include "defs.h"
#include "syscall.h"

extern int sys_fork(void);
extern int sys_exit(void);
extern int sys_wait(void);
extern int sys_kill(void);
extern int sys_getpid(void);
extern int sys_write(void);
extern int sys_read(void);
extern int sys_open(void);
extern int sys_close(void);

static int (*syscalls[32])(void) = {
    [SYS_fork]    sys_fork,
    [SYS_exit]    sys_exit,
    [SYS_wait]    sys_wait,
    [SYS_kill]    sys_kill,
    [SYS_getpid]  sys_getpid,
    [SYS_write]   sys_write,
    [SYS_read]    sys_read,
    [SYS_open]    sys_open,
    [SYS_close]   sys_close,
};

int argint(int n, int *ip) {
    struct proc *p = myproc();
    if (p == 0 || p->trapframe == 0) return -1;
    switch(n) {
        case 0: *ip = p->trapframe->a0; break;
        case 1: *ip = p->trapframe->a1; break;
        case 2: *ip = p->trapframe->a2; break;
        case 3: *ip = p->trapframe->a3; break;
        case 4: *ip = p->trapframe->a4; break;
        case 5: *ip = p->trapframe->a5; break;
        default: return -1;
    }
    return 0;
}

int argaddr(int n, uint64 *ip) {
    struct proc *p = myproc();
    if (p == 0 || p->trapframe == 0) return -1;
    switch(n) {
        case 0: *ip = p->trapframe->a0; break;
        case 1: *ip = p->trapframe->a1; break;
        case 2: *ip = p->trapframe->a2; break;
        case 3: *ip = p->trapframe->a3; break;
        case 4: *ip = p->trapframe->a4; break;
        case 5: *ip = p->trapframe->a5; break;
        default: return -1;
    }
    return 0;
}

int argstr(int n, char *buf, int max) {
    uint64 addr;
    if(argaddr(n, &addr) < 0) return -1;
    char *src = (char*)addr;
    for(int i = 0; i < max; i++){
        buf[i] = src[i];
        if(src[i] == 0) return i;
    }
    buf[max-1] = 0;
    return -1;
}

void syscall(void) {
    int num;
    struct proc *p = myproc();

    if (p == 0 || p->trapframe == 0) {
        // panic("syscall");
        while(1);
    }

    num = p->trapframe->a7;

    if(num > 0 && num < sizeof(syscalls)/sizeof(syscalls[0]) && syscalls[num]) {
        // [关键] 移除这里的 printf
        p->trapframe->a0 = syscalls[num]();
    } else {
        printf("pid %d %s: unknown sys call %d\n", p->pid, p->name, num);
        p->trapframe->a0 = -1;
    }
}