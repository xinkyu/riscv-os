#ifndef __ASSEMBLER__
#include "user/user.h"
int fork(void);
void exit(int);
int wait(int*);
int getpid(void);
int sleep(int);
uint64 uptime(void);
#else
#include "kernel/syscall.h"
.global fork
.global exit
.global wait
.global getpid
.global sleep
.global uptime

fork:
    li a7, SYS_fork
    ecall
    ret

exit:
    li a7, SYS_exit
    ecall
    ret

wait:
    li a7, SYS_wait
    ecall
    ret

getpid:
    li a7, SYS_getpid
    ecall
    ret

sleep:
    li a7, SYS_sleep
    ecall
    ret

uptime:
    li a7, SYS_uptime
    ecall
    ret
#endif