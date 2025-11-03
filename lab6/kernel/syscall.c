// kernel/syscall.c
#include "riscv.h"
#include "defs.h"
#include "proc.h"
#include "syscall.h"

//
// 从陷阱帧中提取系统调用参数
//

// 提取第 n 个 32位 整数参数
int argint(int n, int *ip)
{
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

// 提取第 n 个 64位 地址参数
int argaddr(int n, uint64 *ip)
{
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
    // TODO: 在这里应添加检查 *ip 是否为合法的用户地址
    return 0;
}

//
// 系统调用分发
//

// 声明所有系统调用函数 (定义在 sysproc.c)
extern int sys_getpid(void);
extern int sys_yield(void);
extern int sys_fork(void);
extern int sys_exit(void);
// ... 声明其他 sys_ 函数 ...


// 系统调用函数指针数组
static int (*syscalls[])(void) = {
    [SYS_getpid] = sys_getpid,
    [SYS_yield]  = sys_yield,
    [SYS_fork]   = sys_fork,
    [SYS_exit]   = sys_exit,
    // ... 其他 ...
};

// 系统调用分发器
void syscall(void)
{
    struct proc *p = myproc();
    // 从陷阱帧的 a7 寄存器获取系统调用号
    int num = p->trapframe->a7;

    if (num > 0 && num < (sizeof(syscalls) / sizeof(syscalls[0])) && syscalls[num]) {
        // 调用对应的系统调用
        // 返回值存入 a0 寄存器
        p->trapframe->a0 = syscalls[num]();
    } else {
        printf("syscall: unknown sys_num %d\n", num);
        p->trapframe->a0 = -1; // 返回 -1 表示错误
    }
}