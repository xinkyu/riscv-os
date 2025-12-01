// kernel/sysproc.c
#include "defs.h"

uint64 sys_getpid(void) {
    // [关键] 性能测试绝对不能包含 printf
    return myproc()->pid;
}

uint64 sys_exit(void) {
    int n;
    if(argint(0, &n) < 0)
        return -1;
    exit(n);
    return 0; 
}

uint64 sys_fork(void) {
    return fork();
}

uint64 sys_wait(void) {
    uint64 p;
    if(argaddr(0, &p) < 0)
        return -1;
    return wait((int*)p);
}

uint64 sys_kill(void) {
    int pid;
    if(argint(0, &pid) < 0)
        return -1;
    printf("sys_kill not implemented\n");
    return -1; 
}

// sys_klog(char *buf, int len)
// 从内核日志缓冲区读取数据放入用户提供的 buf 中
uint64 sys_klog(void) {
    uint64 buf;
    int len;

    // 获取参数 0 (buf指针) 和 参数 1 (长度)
    if (argaddr(0, &buf) < 0 || argint(1, &len) < 0)
        return -1;

    return klog_read(buf, len);
}