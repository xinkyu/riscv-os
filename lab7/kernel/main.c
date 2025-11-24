// kernel/main.c
#include "defs.h"
#include "riscv.h"

void main_task(void) {
    printf("===== main_task Started (PID %d) =====\n", myproc()->pid);
    run_lab6_tests();
    printf("\n===== All Labs Complete =====\n");
    printf("Press Ctrl-A then X to quit QEMU.\n");
    while (1);
}

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

volatile static int started = 0;

void kmain() {
    int cpuid_ = cpuid();
    if (cpuid_ == 0) {
        console_init();
        printfinit();
        printf("\n");
        printf("xv6 kernel is booting\n");
        printf("\n");

        kinit();         // 物理内存分配器
        kvminit();       // 内核页表
        kvminithart();   // 开启分页
        procinit();      // 进程表
        trap_init();     // 陷阱向量
        trap_inithart(); // 安装陷阱处理程序
        
        plic_init();     // set up PLIC priority <--- 必须在 virtio_disk_init 之前
        plic_inithart(); // ask PLIC for device interrupts
        
        binit();         // buffer cache
        iinit();         // inode cache
        fileinit();      // file table
        virtio_disk_init(); // emulated hard disk <--- 初始化磁盘
        
        // pinit();      // 如果有 pipe
        
        userinit();      // first user process
        __sync_synchronize();
        started = 1;
    } else {
        while (started == 0);
        __sync_synchronize();
        printf("hart %d starting\n", cpuid_);
        kvminithart();
        trap_inithart();
        plic_inithart();
    }

    scheduler();
}