// kernel/syscall.c
#include "defs.h"
#include "proc.h"
#include "syscall.h"
#include "riscv.h"

// 声明 sysproc.c 中的函数
extern uint64 sys_getpid(void);
// (在这里声明所有其他的 sys_... 函数)
// extern uint64 sys_fork(void);
// extern uint64 sys_exit(void);
// extern uint64 sys_write(void);

// 系统调用表
static uint64 (*syscalls[])(void) = {
    [SYS_fork]    0, // (0 表示未实现)
    [SYS_exit]    0,
    [SYS_wait]    0,
    [SYS_pipe]    0,
    [SYS_read]    0,
    [SYS_kill]    0,
    [SYS_exec]    0,
    [SYS_fstat]   0,
    [SYS_chdir]   0,
    [SYS_dup]     0,
    [SYS_getpid]  sys_getpid, // 注册 sys_getpid
    [SYS_sbrk]    0,
    [SYS_sleep]   0,
    [SYS_uptime]  0,
    [SYS_open]    0,
    [SYS_write]   0,
    [SYS_mknod]   0,
    [SYS_unlink]  0,
    [SYS_link]    0,
    [SYS_mkdir]   0,
    [SYS_close]   0,
};

// 系统调用总分发函数
void syscall(struct trapframe *tf) {
    int num = tf->a7; // a7 寄存器存放系统调用号
    
    if (num > 0 && num < (sizeof(syscalls)/sizeof(syscalls[0])) && syscalls[num]) {
        // 调用对应的实现
        uint64 ret = syscalls[num]();
        
        // 将返回值存入 a0
        tf->a0 = ret;
    } else {
        printf("pid %d: unknown syscall %d\n", myproc()->pid, num);
        tf->a0 = -1; // 返回错误
    }
}

// ---------------------------------
// 参数获取辅助函数 (根据手册 [cite: 1428])
// ---------------------------------

// 从陷阱帧获取第 n 个整数参数
int argint(int n, int *ip) {
    struct proc *p = myproc();
    switch (n) {
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

// 从陷阱帧获取第 n 个 64 位地址参数
int argaddr(int n, uint64 *ip) {
    struct proc *p = myproc();
    switch (n) {
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

// (argstr 和 copyin/copyout 的实现比较复杂，
// 它们需要验证用户指针并逐页表遍历来复制内存，
// 你可以在实现了 sys_getpid 之后再挑战它们)