// lab4/kernel/main.c
#include "defs.h"
#include "riscv.h"

void kmain(void) {
    clear_screen();
    printf("===== Kernel Start =====\n");

    // 初始化物理内存分配器
    kinit();
    
    // 创建并初始化内核页表
    kvminit();
    
    // 启用虚拟内存
    kvminithart();

    // --- Lab4 新增 ---
    // 初始化中断向量
    trap_init();

    // 初始化时钟中断
    clock_init();

    // 开启 Supervisor 模式的全局中断
    w_sstatus(r_sstatus() | SSTATUS_SIE);
    
    printf("\nInitialization complete. Interrupts enabled. Entering halt state.\n");
    
    // 内核初始化完成，进入死循环等待中断
    while (1);
}