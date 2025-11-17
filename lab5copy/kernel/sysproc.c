// kernel/sysproc.c
#include "defs.h"

uint64 sys_exit(void) {
    int n;
    if (argint(0, &n) < 0)
        return -1;
    exit(n);
    return 0;  // not reached
}

uint64 sys_getpid(void) {
    return myproc()->pid;
}

uint64 sys_fork(void) {
    return fork();
}

uint64 sys_wait(void) {
    uint64 p;
    if (argaddr(0, &p) < 0)
        return -1;
    return wait(p);
}

uint64 sys_sleep(void) {
    int n;
    if (argint(0, &n) < 0)
        return -1;
    
    uint64 ticks0 = get_ticks();
    acquire(&myproc()->lock);
    while (get_ticks() - ticks0 < n) {
        if (myproc()->killed) {
            release(&myproc()->lock);
            return -1;
        }
        sleep(get_ticks_channel(), &myproc()->lock);
    }
    release(&myproc()->lock);
    return 0;
}

uint64 sys_uptime(void) {
    return get_ticks();
}