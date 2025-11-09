// lab5/kernel/main.c
#include "defs.h"
#include "riscv.h"
#include "proc.h" // 引入 proc.h

void kmain(void) {
    clear_screen();
    printf("===== Kernel Start =====\n");

    // 初始化物理内存分配器
    kinit();
    
    // 创建并初始化内核页表
    kvminit();
    
    // 启用虚拟内存
    kvminithart();

    // 初始化中断向量
    trap_init();

    // 初始化时钟中断
    clock_init();

    // --- Lab5 新增 ---
    // 初始化进程表
    procinit();

    // 创建第一个进程
    userinit();
    
    printf("\nInitialization complete. Starting scheduler.\n");
    
    // 开启 Supervisor 模式的全局中断
    //w_sstatus(r_sstatus() | SSTATUS_SIE);

    // 启动调度器 
    scheduler();
    
    // scheduler() 不应该返回，如果返回了说明有错误
    while (1);
}