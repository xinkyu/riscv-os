// user/initcode.S - 第一个用户程序
#ifndef __ASSEMBLER__
void start(void);
#else
#include "kernel/syscall.h"
# exec(init, argv)
.globl start
start:
    la a0, init
    la a1, argv"kernel/syscall.h"
    li a7, SYS_exec.global fork
    ecallc 失败，退出
al wait
# 如果 exec 失败，退出_exit
exit:leepll
    li a7, SYS_exitme    jal exit
    ecall.global write
    jal exit

init:    li a7, SYS_fork
    .string "/init\0"p2align 2
    ret

exit:
    li a7, SYS_exitad init
    ecall    .quad 0
#endif