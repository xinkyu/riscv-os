// kernel/sysproc.c
#include "defs.h"

int sys_getpid(void) {
    // [关键] 性能测试绝对不能包含 printf
    return myproc()->pid;
}

int sys_exit(void) {
    int n;
    if(argint(0, &n) < 0)
        return -1;
    exit(n);
    return 0; 
}

int sys_fork(void) {
    return fork();
}

int sys_wait(void) {
    uint64 p;
    if(argaddr(0, &p) < 0)
        return -1;
    return wait((int*)p);
}

int sys_kill(void) {
    int pid;
    if(argint(0, &pid) < 0)
        return -1;
    printf("sys_kill not implemented\n");
    return -1; 
}